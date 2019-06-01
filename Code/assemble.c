#include "ircode.h"
#include "sparse.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>

#define ENSURE_REG true
#define ALLOCATE_REG false
#define REG_NUM 32

/* Definitions of global variants*/

static struct FILE* ass_fp = NULL; //file pointer of assemble output
static bool* codeblock_array = NULL; //block-split flags array of ir code list
static const struct MIPSRegs {
    //named by usage
    char* zero;
    char* at;
    char* v[2];
    char* a[4];
    char* t[10];
    char* s[8];
    char* k[2];
    char* gp;
    char* sp;
    char* fp;
    char* ra;
} reg_set = { "$0", "$1", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8",
              "$t9", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra" };
static bool reg_flags[REG_NUM];

/* Assemble Functions */

void assemble(char* filename) {
    //setup
    ass_fp = fopen(filename, "w");
    assemble_init();

    int block_begin = 0;
    int block_end = 0;
    int code_end = code_num();
    struct CodeListItem* block_ptr = begin_code();
    while (block_end != code_end) {
        block_end = block_begin + 1;
        while (block_end < code_end && codeblock_array[block_end] != true) block_end++;

        //preprocess for the basic block
        struct CodeListItem* ptr = block_ptr;
        for (int i = block_begin; i < block_end; ++i) {
            //resolve data structure
        }

        //handle the basic block
        ptr = block_ptr;
        for (int i = block_begin; i < block_end; ++i) {
            //transform code
            instr_transform(ptr, output);
        }

        //postprocess for the basic block
        block_ptr = ptr;
        block_begin = block_end;
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
void instr_transform(struct CodeListItem* ptr, struct FILE* output) {
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
            char* reg_x = get_reg(ptr->left, ALLOCATE_REG, output);
            if (is_imm(ptr->right)) {
                fprintf(output, "  li %s, %s \n", reg_x, ptr->right);
            }
            else {
                char* reg_y = get_reg(ptr->right, ENSURE_REG, output);
                fprintf(output, "  move %s, %s \n", reg_x, reg_y);
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
char* get_reg(char* var, bool flag, struct FILE* output) {
    char* res = NULL;
    if (flag == ENSURE_REG) {
        //corresponding to ensure(var)
        if ((res = search_in_reg(var)) == NULL) {
            res = get_reg(var, ALLOCATE_REG, output);
            fprintf(output, "lw %s, %s\n", res, var);
        }
    }
    else {
        //corresponding to allocate(var)
        if ((res = search_empty_reg()) == NULL) {
            res = search_best_reg();
            spill_reg(res, output);
        }
    }
    return res;
}

//reset the flags array of registers
void clear_regs() {
    memset(reg_flags, 0, REG_NUM);
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