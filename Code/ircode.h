#ifndef IRCODE_H
#define IRCODE_H

#define CODE_LIST_ITEM_SIZE sizeof(struct CodeListItem)

enum OPERATOR_TYPE { // Definitions of intermediate code operators, according to table 1 in project3.pdf
    OT_LABEL,
    OT_FUNC,
    OT_ASSIGN,
    OT_ADD,
    OT_ADD_R,
    OT_ADD_L,
    OT_ADD_B,
    OT_ADD_REF_L,
    OT_SUB,
    OT_SUB_R,
    OT_SUB_L,
    OT_SUB_B,
    OT_MUL,
    OT_MUL_R,
    OT_MUL_L,
    OT_MUL_B,
    OT_DIV,
    OT_DIV_R,
    OT_DIV_L,
    OT_DIV_B,
    OT_REF,
    OT_DEREF_R,
    OT_DEREF_L,
    OT_DEFRE_B,
    OT_GOTO,
    OT_RELOP,
    OT_RET,
    OT_RET_DEFEF,
    OT_DEC,
    OT_ARG,
    OT_ARG_R,
    OT_CALL,
    OT_PARAM,
    OT_READ,
    OT_WRITE,
    OT_WRITE_DEREF,
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

struct CodeListItem* add_code(enum OPERATOR_TYPE opt, char* left, char* right, char* dst, char* extra);
struct CodeListItem* rm_code(struct CodeListItem* target);
struct CodeListItem* replace_code(struct CodeListItem* target, enum OPERATOR_TYPE opt, char* left, char* right, char* dst, char* extra);
struct CodeListItem* last_code(struct CodeListItem* target);
struct CodeListItem* next_code(struct CodeListItem* target);
void export_code(FILE* output);

#endif