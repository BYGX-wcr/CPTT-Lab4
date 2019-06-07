#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>

/* type and constant value definitions */

#define LEXICAL_U 1
#define GRAM_U 0
#define INT 0
#define FLOAT 1

#define MAX_CHILDS 8
#define MAX_ARGS 10
#define TABLE_SIZE 6

#define NODE_SIZE sizeof(struct Node)
#define CHECK_ID(vertex, name) ((vertex != NULL) ? !strcmp(vertex->id, name) : false)
#define INT_PTR &INT_T
#define FLOAT_PTR &FLOAT_T
#define INVALID_TYPE &INVALID_T
#define SAFE_ID(vertex, name) \
        if (strcmp(vertex->id, name) != 0) \
        {   \
            printf("When checking %s:\n",vertex->id); \
            panic("Node Unmatched!!"); \
        }
 
enum MetaType { BASIC, ARRAY, STRUCTURE, INVALID };  
enum SymbolMetaType { VAR, PROC, USER_TYPE };  // var, function, structure

struct Node {
    bool type;                          //[Lexical Unit]:true, [Grammatical Unit]:false
    char* id;                           //[Lexical Unit]:token, [Grammatical Unit]:non-terminals
    int lineno;                         //line number
    char* info;                         //[Lexical Unit]:details, [Grammatical Unit]:undefined
    struct Node* childs[MAX_CHILDS];    //pointers to child nodes
};

struct FieldList {                  
    char* id;                           //name of id
    struct Type* type;                  //type of id
    struct FieldList* next;
};

struct Type {
    enum MetaType kind;                 //meta type of the Type : BASIC, ARRAY, STRUCTURE
    union {
        int basic;                      //0-int, 1-float
        struct { struct Type* elem_type; int size; } array;
        struct {
            char* struct_id;
            struct FieldList* structure;    //head ptr for FieldList
        };
    };
};

struct ExpType {
    struct Type* type;                  //type of expression
    bool lvalue;                        //flag for whether it is lvalue
};

struct Symbol {
    char* id;                           //name of the symbol, id in PL
    enum SymbolMetaType kind;           //meta type of the symbol
    int first_lineno;                   //lineno where the symbol is declared firstly
    bool defined;                       //flag for define status of the symbol, only used by functions
    int var_num;                            //record temporary var
    union {
        struct Type* type;
        struct {
            struct Type* ret_type;      //return type
            struct Type* argtype_list[MAX_ARGS];    //args_list type
        } proc_type;                    //type definition for function
    };
};

struct SymbolTableItem {
    struct Symbol* id;                  //the symbol carried by the item
    struct SymbolTableItem* next;
};

/* function declarations */

void init();
void visit(struct Node* vertex);
void final_check();
void panic(char* msg);
void errorinfo(int type, int lineno, char* description);
void output(struct Node* root);
unsigned int hash(char *str);

void add_symbol(struct Symbol* newItem);
struct Symbol* search_symbol(char* name);
struct Symbol* create_symbol(char* id, int kind, int first_lineno);
struct Type* create_type(int kind);
struct FieldList* create_field(char* id, struct Type* type);
bool comp_type(struct Type* ltype, struct Type* rtype);
void display_symbol();
bool check_fields(char* id, struct FieldList* fl);

void ExtDef(struct Node* vertex);
struct Type* Specifier(struct Node* vertex);
struct Type* StructSpecifier(struct Node* vertex);
struct ExpType Exp(struct Node* vertex);
void ExtDecList(struct Node* vertex, struct Type* type_inh);
struct Symbol* VarDec(struct Node* vertex, struct Type* type_inh);
struct Symbol* FunDec(struct Node* vertex, struct Type* type_inh);
struct Symbol* Dec(struct Node* vertex, struct Type* type_inh);
struct Symbol* ParamDec(struct Node* vertex);
void VarList(struct Node* vertex, struct Symbol* func, int pos);
void DefList(struct Node* vertex, struct FieldList** fl_inh);
struct FieldList* Def(struct Node* vertex);
struct FieldList* DecList(struct Node* vertex, struct Type* type_inh);
bool CompSt(struct Node* vertex, struct Type* type_inh);
bool StmtList(struct Node* vertex, struct Type* type_inh);
bool Stmt(struct Node* vertex, struct Type* type_inh);
struct FieldList* Args(struct Node* vertex);