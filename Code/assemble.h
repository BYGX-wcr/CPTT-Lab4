#ifndef ASS_H
#define ASS_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>
#include "ircode.h"

#define ENSURE_REG true
#define ALLOCATE_REG false

#define REG_NUM 32
#define REG_V 2
#define REG_V_NUM 2
#define REG_A 4
#define REG_A_NUM 4
#define REG_T 8
#define REG_T_NUM 10
#define REG_S 18
#define REG_S_NUM 8
#define AVA_REG 8
#define AVA_REG_NUM 18

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
    char* reg;
    bool* used;
    int block_len;
    int mem_offset;
    struct VarDesc* next;
};

void assemble(char* filename);
void assemble_init();
void split_blocks();
void instr_transform(struct CodeListItem* ptr, int pos, FILE* output);

int get_reg(char* var, int pos, bool flag, FILE* output);
int search_empty_reg();
int search_in_reg(char* id);
int search_best_reg(int pos);
void clear_regs();
void spill_reg(int index, FILE* output);

struct VarDesc* search_var(char* id);
struct VarDesc* create_var(char* id, int block_len, int mem_offset);

bool is_imm(char* operand);

#endif