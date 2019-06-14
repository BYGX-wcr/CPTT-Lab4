#include "assemble.h"
#include "ircode.h"
#include "sparse.h"

/* Definitions of global variants*/

static FILE* ass_fp = NULL; //file pointer of assemble output

static const union MIPSRegs reg_set = //description of MIPS32 register set
{ "$0", "$1", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8",
  "$t9", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra" };

static struct FrameDesc stack = { NULL, NULL }; //description of the dynamic stack
static struct FrameDesc* cur_frame_ptr = &stack;
static struct VarDesc res_buffer = { NULL, 0, 0, NULL };

/* Assemble Functions */

//assemble main function
void assemble(char* filename) {
    //setup
    ass_fp = fopen(filename, "w");
    assemble_init();

    struct CodeListItem* code_ptr = begin_code();
    while (code_ptr != NULL) {
        instr_transform(code_ptr, ass_fp);

        code_ptr = next_code(code_ptr);
    }

    //clean
    fflush(ass_fp);
    fclose(ass_fp);
    ass_fp = NULL;
}

//initialization before assembling begins
void assemble_init() {
    //initialize global data&symbol in assemble output
    fprintf(ass_fp, ".data\n");
    fprintf(ass_fp, "_prompt: .asciiz \"Enter an integer:\"\n");
    fprintf(ass_fp, "_ret: .asciiz \"\\n\"\n");

    fprintf(ass_fp, ".globl main\n");
    fprintf(ass_fp, ".text\n");

    //add read() and write() functions
    fprintf(ass_fp, 
    "\nread:\n \
    li $v0, 4\n \
    la $a0, _prompt\n \
    syscall\n \
    li $v0, 5\n \
    syscall\n \
    jr $ra\n");

    fprintf(ass_fp, 
    "\nwrite:\n \
    li $v0, 1\n \
    syscall\n \
    li $v0, 4\n \
    la $a0, _ret\n \
    syscall\n \
    move $v0, $0\n \
    jr $ra\n");
}

//transform an intermediate instruction to an assemble instruction
void instr_transform(struct CodeListItem* ptr, FILE* output) {
    switch (ptr->opt)
    {
        case OT_LABEL: {
            fprintf(output, "%s: \n", ptr->left);
            break;
        }
        case OT_FUNC: {
            fprintf(output, "\n%s: \n", ptr->left);
            fprintf(output, "  move $fp, $sp\n");
            break;
        }
        case OT_ASSIGN: {
            int reg_x = get_reg(ptr->left, RES_OPT, output);
            if (is_imm(ptr->right)) {
                fprintf(output, "  li %s, %s \n", reg_set.reg[reg_x], (ptr->right + 1));
            }
            else {
                int reg_y = get_reg(ptr->right, RIGHT_OPT, output);
                fprintf(output, "  move %s, %s \n", reg_set.reg[reg_x], reg_set.reg[reg_y]);
            }
            spill_reg(output);
            break;
        }
        /* TODO: to be finished*/
        case OT_ADD: {
            int reg_x = get_reg(ptr->dst, RES_OPT, output);
            int reg_y = get_reg(ptr->left, LEFT_OPT, output);
            int reg_z = get_reg(ptr->right, RIGHT_OPT, output);
            fprintf(output, "  add %s, %s, %s \n", reg_set.reg[reg_x], reg_set.reg[reg_y], reg_set.reg[reg_z]);
            spill_reg(output);
            break;
        }
        case OT_SUB: {
            int reg_x = get_reg(ptr->dst, RES_OPT, output);
            int reg_y = get_reg(ptr->left, LEFT_OPT, output);
            int reg_z = get_reg(ptr->right, RIGHT_OPT, output);
            fprintf(output, "  sub %s, %s, %s \n", reg_set.reg[reg_x], reg_set.reg[reg_y], reg_set.reg[reg_z]);
            spill_reg(output);
            break;
        }
        case OT_MUL: {
            int reg_x = get_reg(ptr->dst, RES_OPT, output);
            int reg_y = get_reg(ptr->left, LEFT_OPT, output);
            int reg_z = get_reg(ptr->right, RIGHT_OPT, output);
            fprintf(output, "  mul %s, %s, %s \n", reg_set.reg[reg_x], reg_set.reg[reg_y], reg_set.reg[reg_z]);
            spill_reg(output);
            break;
        }
        case OT_DIV: {
            int reg_x = get_reg(ptr->dst, RES_OPT, output);
            int reg_y = get_reg(ptr->left, LEFT_OPT, output);
            int reg_z = get_reg(ptr->right, RIGHT_OPT, output);
            fprintf(output, "  div %s, %s \n", reg_set.reg[reg_y], reg_set.reg[reg_z]);
            fprintf(output, "  mflo %s", reg_set.reg[reg_x]);
            spill_reg(output);
            break;
        }
        case OT_GOTO: {
            fprintf(output, "  j %s \n", ptr->left);
            break;
        }
        case OT_RELOP: {
            int reg_x = get_reg(ptr->left, LEFT_OPT, output);
            int reg_y = get_reg(ptr->right, RIGHT_OPT, output);
            if (strcmp(ptr->extra, "==") == 0)
                fprintf(output, "  beq %s, %s, %s \n", reg_set.reg[reg_x], reg_set.reg[reg_y], ptr->dst);
            else if (strcmp(ptr->extra, "!=") == 0)
                fprintf(output, "  bne %s, %s, %s \n", reg_set.reg[reg_x], reg_set.reg[reg_y], ptr->dst);
            else if (strcmp(ptr->extra, ">") == 0)
                fprintf(output, "  bgt %s, %s, %s \n", reg_set.reg[reg_x], reg_set.reg[reg_y], ptr->dst);
            else if (strcmp(ptr->extra, "<") == 0)
                fprintf(output, "  blt %s, %s, %s \n", reg_set.reg[reg_x], reg_set.reg[reg_y], ptr->dst);
            else if (strcmp(ptr->extra, ">=") == 0)
                fprintf(output, "  bge %s, %s, %s \n", reg_set.reg[reg_x], reg_set.reg[reg_y], ptr->dst);
            else if (strcmp(ptr->extra, "<=") == 0)
                fprintf(output, "  ble %s, %s, %s \n", reg_set.reg[reg_x], reg_set.reg[reg_y], ptr->dst);
            else
                assert(0);
            break;
        }
        case OT_RET: {
            int reg_x = get_reg(ptr->left, LEFT_OPT, output);
            fprintf(output, "  move $v0, %s\n", reg_set.reg[reg_x]);
            fprintf(output, "  jr $ra\n");
            break;
        }
        case OT_DEC: {
            fprintf(output, "  addi $sp, $sp, -%d \n",  atoi(ptr->right));
            int offset = get_cur_offset();
            create_var(ptr->left, offset, atoi(ptr->right));
            break;
        }
        case OT_ARG: {
            fprintf(output, "  addi $sp, $sp, -4 \n");
            int reg_x = get_reg(ptr->left, LEFT_OPT, output);
            fprintf(output, "  sw %s, 0($sp)\n", reg_set.reg[reg_x]);
            break;
        }
        case OT_CALL: {
            //save return address & $fp
            fprintf(output, "  addi $sp, $sp, -4 \n");
            fprintf(output, "  sw $ra, 0($sp)\n");
            fprintf(output, "  addi $sp, $sp, -4 \n");
            fprintf(output, "  sw $fp, 0($sp)\n");

            fprintf(output, "  jal %s \n", ptr->right);

            //recover return address & $fp
            fprintf(output, "  move $sp, $fp\n");
            fprintf(output, "  lw $fp, 0($sp)\n");
            fprintf(output, "  addi $sp, $sp, 4 \n");
            fprintf(output, "  lw $ra, 0($sp)\n");
            fprintf(output, "  addi $sp, $sp, 4 \n");
            int reg_x = get_reg(ptr->left, RES_OPT, output);
            fprintf(output, "  move %s, $v0\n", reg_set.reg[reg_x]);
            spill_reg(output);
            break;
        }
        case OT_PARAM: {
            int offset = get_cur_arg_offset();
            create_var(ptr->left, offset, 4);
            break;
        }
        case OT_READ: {
            fprintf(output, "  addi $sp, $sp, -4 \n");
            fprintf(output, "  sw $ra, 0($sp)\n");
            fprintf(output, "  addi $sp, $sp, -4 \n");
            fprintf(output, "  sw $fp, 0($sp)\n");

            fprintf(output, "  jal read \n");

            fprintf(output, "  move $sp, $fp\n");
            fprintf(output, "  lw $fp, 0($sp)\n");
            fprintf(output, "  addi $sp, $sp, 4 \n");
            fprintf(output, "  lw $ra, 0($sp)\n");
            fprintf(output, "  addi $sp, $sp, 4 \n");
            int reg_x = get_reg(ptr->left, RES_OPT, output);
            fprintf(output, "  move %s, $v0\n", reg_set.reg[reg_x]);
            spill_reg(output);
            break;
        }
        case OT_WRITE: {
            fprintf(output, "  addi $sp, $sp, -4 \n");
            int reg_x = get_reg(ptr->left, LEFT_OPT, output);
            fprintf(output, "  sw %s, 0($sp)\n", reg_set.reg[reg_x]);

            fprintf(output, "  addi $sp, $sp, -4 \n");
            fprintf(output, "  sw $ra, 0($sp)\n");
            fprintf(output, "  addi $sp, $sp, -4 \n");
            fprintf(output, "  sw $fp, 0($sp)\n");

            fprintf(output, "  jal write \n");

            fprintf(output, "  move $sp, $fp\n");
            fprintf(output, "  lw $fp, 0($sp)\n");
            fprintf(output, "  addi $sp, $sp, 4 \n");
            fprintf(output, "  lw $ra, 0($sp)\n");
            fprintf(output, "  addi $sp, $sp, 4 \n");
            break;
        }
        default:
            printf("opt: %d\n", ptr->opt);
            assert(0);
            break;
    }
}

/* Operations for register allocation */

//allocate an register for arg:var, arg:flag denotes the used method
//return the string of allocated register
int get_reg(char* var, int flag, FILE* output) {
    int res = -1;
    int buffer = -1;

    struct VarDesc* var_ptr = search_var(var);
    if (flag != 0) {
        if (flag == LEFT_OPT) {
            res = REG_LEFT;
            buffer = REG_LEFT_BUFFER;
        }
        else if (flag == RIGHT_OPT) {
            res = REG_RIGHT;
            buffer = REG_RIGHT_BUFFER;
        }
        else
            panic("Undefined flag!");

        //corresponding to left or right operands
        if (is_imm(var)) {
            fprintf(output, "  li %s, %d\n", reg_set.reg[res], atoi(var + 1));
        }
        else {//non-intermediate number
            if (var_ptr == NULL) {
                panic("Use a variant before assigning a value to it!");
            }

            if (var[0] == '*') {// require dereference
                fprintf(output, "  lw %s, %d($fp)\n", reg_set.reg[buffer], var_ptr->mem_offset);
                fprintf(output, "  lw %s, 0(%s)\n", reg_set.reg[res], reg_set.reg[buffer]);
            }
            else if (var[0] == '&') {// require reference
                fprintf(output, "  addi %s, $fp, %d\n", reg_set.reg[res], var_ptr->mem_offset);
            }
            else {// normal
                fprintf(output, "  lw %s, %d($fp)\n", reg_set.reg[res], var_ptr->mem_offset);
            }
        }
    }
    else {
        if (var_ptr == NULL) {
            int offset = get_cur_offset();
            fprintf(output, "  addi $sp, $sp, -4\n");
            var_ptr = create_var(var, offset, 4);
        }

        res = REG_RES;
        buffer = REG_RES_BUFFER;
        //corresponding to destination
        if (var[0] == '*') {// require dereference
            fprintf(output, "  lw %s, %d($fp)\n", reg_set.reg[buffer], var_ptr->mem_offset);
        }
        else {// normal
            fprintf(output, "  addi %s, $fp, %d\n", reg_set.reg[buffer], var_ptr->mem_offset);
        }
    }
    return res;
}

//spill the value in the result register into memory
void spill_reg(FILE* output) {
    fprintf(output, "  sw %s, 0(%s)\n", reg_set.reg[REG_RES], reg_set.reg[REG_RES_BUFFER]);
}

/* Operations on stack */

void push_frame() {
    struct FrameDesc* temp = malloc(sizeof(struct FrameDesc));
    memset(temp, 0, sizeof(struct FrameDesc));
    temp->last = cur_frame_ptr;
    temp->next = NULL;
    cur_frame_ptr = temp;
}

void pop_frame() {
    if (cur_frame_ptr == &stack) return;

    struct FrameDesc* temp = cur_frame_ptr;
    cur_frame_ptr = temp->last;
    free(temp);
}

int get_cur_offset() {
    struct VarDesc* ptr = cur_frame_ptr->var_head.next;
    int res = BASE_OFFSET;
    while (ptr != NULL) {
        if (ptr->mem_offset < 0) {
            res -= ptr->size;
        }

        ptr = ptr->next;
    }

    return res;
}

int get_cur_arg_offset() {
    struct VarDesc* ptr = cur_frame_ptr->var_head.next;
    int res = ARG_OFFSET;
    while (ptr != NULL) {
        if (ptr->mem_offset >= 0) {
            res += ptr->size;
        }
        else {
            break;
        }

        ptr = ptr->next;
    }

    return res;
}

struct VarDesc* search_var(char* id) {
    struct VarDesc* ptr = cur_frame_ptr->var_head.next;
    while (ptr != NULL) {
        if (strcmp(ptr->id, id) == 0) {
            return ptr;
        }

        ptr = ptr->next;
    }

    return ptr;
}

struct VarDesc* create_var(char* id, int mem_offset, int size) {
    struct VarDesc* ptr = &cur_frame_ptr->var_head;
    while (ptr->next != NULL) {
        ptr = ptr->next;
    }

    struct VarDesc* new_var = malloc(sizeof(struct VarDesc));
    memset(new_var, 0, sizeof(struct VarDesc));
    copy_str(&new_var->id, id);
    new_var->mem_offset = mem_offset;
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