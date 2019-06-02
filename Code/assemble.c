#include "ircode.h"
#include "sparse.h"
#include "assemble.h"

/* Definitions of global variants*/

static FILE* ass_fp = NULL; //file pointer of assemble output
static bool* codeblock_array = NULL; //block-split flags array of ir code list

static const union MIPSRegs reg_set = //description of MIPS32 register set
{ "$0", "$1", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8",
  "$t9", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra" };

static struct VarDesc var_list = { "HEAD", NULL, NULL, 0, NULL }; //the linked list of variant description
static struct VarDesc* reg_desc[REG_NUM]; //the array of occupation info of regs

/* Assemble Functions */

void assemble(char* filename) {
    //setup
    ass_fp = fopen(filename, "w");
    assemble_init();

    int block_begin = 0;
    int block_end = 0;
    int block_len = 0;
    int code_end = code_num();
    struct CodeListItem* block_ptr = begin_code();
    while (block_end != code_end) {
        block_end = block_begin + 1;
        block_len = block_end - block_begin;
        while (block_end < code_end && codeblock_array[block_end] != true) block_end++;

        //preprocess for the basic block
        struct CodeListItem* ptr = block_ptr;
        for (int i = block_begin; i < block_end; ++i) {
            //resolve data structure
            if (ptr->opt != OT_LABEL && ptr->opt != OT_GOTO) {
                if (strlen(ptr->right) != 0) {
                    struct VarDesc* var = search_var(ptr->right);
                    if (var == NULL) {
                        //create VarDesc for var
                        var = create_var(ptr->right, block_len);
                    }
                    assert(var->used);
                    var->used[i] = true;
                }

                if (strlen(ptr->left) != 0) {
                    struct VarDesc* var = search_var(ptr->left);
                    if (var == NULL) {
                        //create VarDesc for var
                        var = create_var(ptr->left, block_len);
                    }
                    assert(var->used);
                    var->used[i] = true;
                }
            }

            ptr = next_code(ptr);
        }

        //handle the basic block
        ptr = block_ptr;
        for (int i = block_begin; i < block_end; ++i) {
            //transform code
            assert(ptr);
            instr_transform(ptr, i, ass_fp);

            ptr = next_code(ptr);
        }

        //postprocess for the basic block
        block_ptr = ptr;
        block_begin = block_end;
        /* TODO: clear regs and var_list */
    }

    //clean
    fflush(ass_fp);
    fclose(ass_fp);
    ass_fp = NULL;
}

//initialization before assembling begins
void assemble_init() {
    //initialize global data in assemble output
    fprintf(ass_fp, ".data\n");
    fprintf(ass_fp, "_prompt: .asciiz \"Enter an integer:\"");
    fprintf(ass_fp, "_ret: .asciiz \"\\n\"");
    //add read() and write() functions
    fprintf(ass_fp, "\nread:\n \
                  li $v0, 4\n \
                  la $a0, _prompt\n \
                  syscall\n \
                  li $v0, 5\n \
                  syscall\n \
                  jr $ra\n");

    fprintf(ass_fp, "\nwrite:\n \
                li $v0, 1\n \
                syscall\n \
                li $v0, 4\n \
                la $a0, _ret\n \
                syscall\n \
                move $v0, $0\n \
                jr $ra\n");

    //split basic blocks
    split_blocks();

    //clear flags of registers
    clear_regs();
}

//split the ir code list to basic blocks
void split_blocks() {
    int len = code_num();
    if (len == 0) { panic("Cannot split blocks in empty ir code list!"); }

    if (codeblock_array != NULL) free(codeblock_array);
    codeblock_array = malloc(len);
    memset(codeblock_array, 0, len);

    struct CodeListItem* ptr = begin_code();
    int counter = 0;
    codeblock_array[0] = true;
    while (ptr != NULL) {
        if (ptr->opt == OT_LABEL) {
            codeblock_array[counter] = true;
        }
        else if ((ptr->opt == OT_RELOP || ptr->opt == OT_GOTO) && counter + 1 < len) {
            codeblock_array[counter + 1] = true;
        }

        ptr = next_code(ptr);
        counter++;
    }
}

//transform an intermediate instruction to an assemble instruction
void instr_transform(struct CodeListItem* ptr, int pos, FILE* output) {
    switch (ptr->opt)
    {
        case OT_LABEL: {
            fprintf(output, "  %s: \n", ptr->left);
            break;
        }
        case OT_FUNC: {
            fprintf(output, "\n%s: \n", ptr->left);
            break;
        }
        case OT_ASSIGN: {
            int reg_x = get_reg(ptr->left, pos, ALLOCATE_REG, output);
            if (is_imm(ptr->right)) {
                fprintf(output, "  li %s, %s \n", reg_set.reg[reg_x], ptr->right);
            }
            else {
                int reg_y = get_reg(ptr->right, pos, ENSURE_REG, output);
                fprintf(output, "  move %s, %s \n", reg_set.reg[reg_x], reg_set.reg[reg_y]);
            }
            break;
        }
        /* TODO: to be finished*/
        case OT_ADD: {
            fprintf(output, "  %s := %s + %s \n", ptr->dst, ptr->left, ptr->right);
            break;
        }
        case OT_SUB: {
            fprintf(output, "  %s := %s - %s \n", ptr->dst, ptr->left, ptr->right);
            break;
        }
        case OT_MUL: {
            fprintf(output, "  %s := %s * %s \n", ptr->dst, ptr->left, ptr->right);
            break;
        }
        case OT_DIV: {
            fprintf(output, "  %s := %s / %s \n", ptr->dst, ptr->left, ptr->right);
            break;
        }
        case OT_GOTO: {
            fprintf(output, "  GOTO %s \n", ptr->left);
            break;
        }
        case OT_RELOP: {
            fprintf(output, "  IF %s %s %s GOTO %s \n", ptr->left, ptr->extra, ptr->right, ptr->dst);
            break;
        }
        case OT_RET: {
            fprintf(output, "  RETURN %s \n", ptr->left);
            break;
        }
        case OT_DEC: {
            fprintf(output, "  DEC %s %s \n", ptr->left, ptr->right);
            break;
        }
        case OT_ARG: {
            fprintf(output, "  ARG %s \n", ptr->left);
            break;
        }
        case_OT_ARG_R: {
            fprintf(output, "ARG *%s \n", ptr->left);
            break;              
        }
        case OT_CALL: {
            fprintf(output, "%s := CALL %s \n", ptr->left, ptr->right);
            break;
        }
        case OT_PARAM: {
            fprintf(output, "PARAM %s \n", ptr->left);
            break;
        }
        case OT_READ: {
            fprintf(output, "READ %s \n", ptr->left);
            break;
        }
        case OT_WRITE: {
            fprintf(output, "WRITE %s \n", ptr->left);
            break;
        }
        default:
            assert(0);
            break;
    }
}

/* Operations for register allocation */

//allocate an register for arg:var, arg:flag denotes the used method
//return the string of allocated register
int get_reg(char* var, int pos, bool flag, FILE* output) {
    int res = -1;
    if (flag == ENSURE_REG) {
        //corresponding to ensure(var)
        if ((res = search_in_reg(var)) == -1) {
            res = get_reg(var, pos, ALLOCATE_REG, output);
            fprintf(output, "lw %s, %s\n", reg_set.reg[res], var);
        }
    }
    else {
        //corresponding to allocate(var)
        if ((res = search_empty_reg()) == -1) {
            res = search_best_reg(pos);
            spill_reg(res, output);
        }
    }
    return res;
}

//search an empty register
//return the index of empty register if found, otherwise -1
int search_empty_reg() {
    for (int i = 2; i < 26; ++i) {
        if (reg_desc[i] == NULL) {
            return i;
        }
    }

    return -1;
}

//search the register arg:id existing in
//return the index of target register if found, otherwise -1
int search_in_reg(char* id) {
    for (int i = 2; i < 26; ++i) {
        if (reg_desc[i] != NULL && strcmp(reg_desc[i]->id, id) == 0) {
            return i;
        }
    }

    return -1;
}

//search the best register in the context of arg:pos
//return the index of best register
int search_best_reg(int pos) {
    int max = -1;
    int maxReg = -1;
    for (int i = 2; i < 26; ++i) {
        if (reg_desc[i] == NULL) {
            return i;
        }
        else if (reg_desc[i] != NULL) {
            int offset = 0x7fffffff;
            for (int j = reg_desc[i]->block_len - 1; j >= 0; --j) {
                if (reg_desc[i]->used[j] && pos <= j) {
                    offset = j - pos;
                    break;
                }
                else if (pos > j) {
                    break;
                }
            }
            
            if (offset > max) {
                max = offset;
                maxReg = i;
            }
        }
    }

    assert(maxReg != -1);
    return maxReg;
}

//reset the flags array of registers
void clear_regs() {
    /* TODO: spill regs */
    memset(reg_desc, 0, REG_NUM * sizeof(struct VarDesc*));
}

//spill the value in register into memory
void spill_reg(int index, FILE* output) {
    //
}

/* Operations on VarDesc list*/

struct VarDesc* search_var(char* id) {
    struct VarDesc* ptr = var_list.next;
    while (ptr != NULL) {
        if (strcmp(ptr->id, id) == 0) {
            return ptr;
        }

        ptr = ptr->next;
    }

    return ptr;
}

struct VarDesc* create_var(char* id, int block_len) {
    struct VarDesc* ptr = &var_list;
    while (ptr->next != NULL) {
        ptr = ptr->next;
    }

    struct VarDesc* new_var = malloc(sizeof(struct VarDesc));
    copy_str(&new_var->id, id);
    new_var->reg = NULL;
    new_var->used = malloc(block_len);
    memset(new_var->used, 0, block_len);
    new_var->next = NULL;
    ptr->next = new_var;

    return new_var;
}

/* Tool functions */

//judge whether the operand is immediate number
//return true if operand is immediate number, otherwise return false
bool is_imm(char* operand) {
    if (operand[0] == '#')
        return true;
    else
        return false;
}