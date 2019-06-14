#ifndef ASS_H
#define ASS_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>

#define LEFT_OPT 1
#define RIGHT_OPT 2
#define RES_OPT 0

#define REG_NUM 32
#define REG_LEFT 8
#define REG_LEFT_BUFFER 9
#define REG_RIGHT 10
#define REG_RIGHT_BUFFER 11
#define REG_RES 12
#define REG_RES_BUFFER 13

#define ARG_OFFSET 8
#define BASE_OFFSET 0

extern struct CodeListItem;

union MIPSRegs { /* !!! the order of regs has been tuned */
    struct {
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
    }; //named by usage
    char* reg[REG_NUM]; //named by order
};

struct VarDesc {
    char* id;
    int mem_offset;
    int size;
    struct VarDesc* next;
};

struct FrameDesc {
    struct FrameDesc* next;
    struct FrameDesc* last;
    struct VarDesc var_head;
};

void assemble(char* filename);
void assemble_init();
void instr_transform(struct CodeListItem* ptr, FILE* output);

int get_reg(char* var, int flag, FILE* output);
void spill_reg(FILE* output);

void push_frame();
void pop_frame();
int get_cur_offset();
int get_cur_arg_offset();
struct VarDesc* search_var(char* id);
struct VarDesc* create_var(char* id, int mem_offset, int size);

bool is_imm(char* operand);

#endif