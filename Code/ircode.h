#ifndef IRCODE_H
#define IRCODE_H

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>

#define CODE_LIST_ITEM_SIZE sizeof(struct CodeListItem)

enum OPERATOR_TYPE { // Definitions of intermediate code operators, according to table 1 in project3.pdf
    OT_LABEL,
    OT_FUNC,
    OT_ASSIGN,
    OT_ADD,
    OT_SUB,
    OT_MUL,
    OT_DIV,
    OT_GOTO,
    OT_RELOP,
    OT_RET,
    OT_DEC,
    OT_ARG,
    OT_CALL,
    OT_PARAM,
    OT_READ,
    OT_WRITE,
    OT_FLAG
};

struct CodeListItem { // Definition of items of bidirected-cyclic ir code list
    struct CodeListItem* last;

    enum OPERATOR_TYPE opt;
    char* left;
    char* right;
    char* dst;
    char* extra;

    struct CodeListItem* next;
};

void copy_str(char** dst, const char* src);

struct CodeListItem* add_code(enum OPERATOR_TYPE opt, char* left, char* right, char* dst, char* extra);
struct CodeListItem* rm_code(struct CodeListItem* target);
struct CodeListItem* replace_code(struct CodeListItem* target, enum OPERATOR_TYPE opt, char* left, char* right, char* dst, char* extra);
struct CodeListItem* last_code(struct CodeListItem* target);
struct CodeListItem* next_code(struct CodeListItem* target);
struct CodeListItem* begin_code();
struct CodeListItem* end_code();
void export_code(FILE* output);

#endif