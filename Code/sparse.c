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
#define SAFE_ID(vertex, name) \
        if (strcmp(vertex->id, name)) \
        {   \
            printf("When checking %s:",vertex->id); \
            panic("Node Unmatched!!"); \
        }

enum MetaType { BASIC, ARRAY, STRUCTURE };  
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

/* global variant definitions */

static struct SymbolTableItem* symbol_table[TABLE_SIZE]; //a hash table

const static struct Type INT_T = { BASIC, INT }; //initialize the constant type struct of int
const static struct Type FLOAT_T = { BASIC, FLOAT }; //initialize the constant type struct of float

static bool struct_def_flag = false; //set true when defining a struct type
static bool func_def_flag = false; //set true when defining a function

/* function declarations */

void init();
void visit(struct Node* vertex);
void final_check();
void panic(char* msg);
void errorinfo(int type, int lineno, char* description);
void output(struct Node* root);

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

/* traverse functions */

void semantic_parse(struct Node* root) {
    init();

    if (CHECK_ID(root, "Program"))
        visit(root);
    else
        panic("Unexpected semantic parsing object!");

    final_check();

    display_symbol();
    //output(root);
}

void init() {
    memset(symbol_table, 0, sizeof(struct SymbolTableItem *)*TABLE_SIZE);
}

void final_check() {

    //check function declarations
    for (int i = 0; i < TABLE_SIZE; ++i) {
        struct SymbolTableItem* ptr = symbol_table[i];
        while (ptr != NULL) {
            struct Symbol* id = ptr->id;
            if (id->kind == PROC && !id->defined)
                errorinfo(18, id->first_lineno, "Undefined function");

            ptr = ptr->next;
        }
    }
}

void panic(char* msg) {
    fprintf(stderr, "%s\n", msg);
    assert(0);
}

//use DFS to visit the node of syntax tree
void visit(struct Node* vertex) {     
    if (vertex == NULL)
        panic("Null Vertex Pointer");
                                                
    if (CHECK_ID(vertex, "ExtDef")) {       
        ExtDef(vertex);
    }
    else if (CHECK_ID(vertex, "DefList")) {
        struct FieldList* var_dec_list;
        DefList(vertex, &var_dec_list);
    }
    else if (CHECK_ID(vertex, "Exp")) {
        Exp(vertex);
    }
    else {
        int ptr = 0;
        while (ptr < MAX_CHILDS && vertex->childs[ptr] != NULL) {       
            visit(vertex->childs[ptr]);
            ++ptr;
        }
    }
}

void errorinfo(int type, int lineno, char* description) {
    printf("Error type %d at Line %d: %s\n", type, lineno, description);
}

/* semantic parse function */

void ExtDef(struct Node* vertex) {
    SAFE_ID(vertex,"ExtDef");
    struct Type* type_inh = Specifier(vertex->childs[0]);

    if (CHECK_ID(vertex->childs[1], "ExtDecList")) {
        ExtDecList(vertex->childs[1], type_inh);
    }
    else if (CHECK_ID(vertex->childs[1], "FunDec")) {           
        struct Symbol* func = FunDec(vertex->childs[1], type_inh);

        if (CHECK_ID(vertex->childs[2], "CompSt") && func != NULL) {
            struct Symbol* former = search_symbol(func->id);
            if (former->defined == false)
                func->defined = true;
            else
                errorinfo(4, vertex->childs[1]->lineno, "Redefined function");

            // function is defined here
            if (!CompSt(vertex->childs[2], type_inh)) {
                /* no return val */
            }
        }
    }
}

struct Type* Specifier(struct Node* vertex) {   
    SAFE_ID(vertex,"Specifier");
    if (CHECK_ID(vertex->childs[0], "TYPE")) {
        struct Type* basic_type;
        if (strcmp(vertex->childs[0]->info, "INT"))
            basic_type = INT_PTR;
        else
            basic_type = FLOAT_PTR;
        return basic_type;
    }
    else {
        return StructSpecifier(vertex->childs[0]);
    }
}

struct Type* StructSpecifier(struct Node* vertex) {
    SAFE_ID(vertex,"StructSpecifier");

    if (CHECK_ID(vertex->childs[1], "Tag")) { //struct def reference
        struct Symbol* id = search_symbol(vertex->childs[1]->childs[0]->info);
        if (id->kind != USER_TYPE)
            errorinfo(17, vertex->childs[1]->lineno, "Undefined struct type");

        struct Type* type = id->type;
        return type;
    }
    else { //struct definition
        /* TODO: Anonymous struct*/
        struct Symbol* id = create_symbol(vertex->childs[1]->childs[0]->info, USER_TYPE, vertex->childs[1]->childs[0]->lineno);
        struct Type* type = create_type(STRUCTURE);
        type->struct_id = id->id;

        struct_def_flag = true;
        DefList(vertex->childs[3], &type->structure);
        struct_def_flag = false;

        if (id == NULL) {
            errorinfo(16, vertex->childs[1]->lineno, "Redefined struct identifier");
        }
        else
            id->type = type;

        return type;
    }
}

void ExtDecList(struct Node* vertex, struct Type* type_inh) {
    SAFE_ID(vertex,"ExtDecList");
    VarDec(vertex->childs[0], type_inh);

    if (vertex->childs[2] != NULL) {
        ExtDecList(vertex->childs[2], type_inh);
    }
}

struct Symbol* VarDec(struct Node* vertex, struct Type* type_inh) {
    SAFE_ID(vertex,"VarDec");
    if (CHECK_ID(vertex->childs[0], "ID")) {                                                            
        struct Symbol* var = create_symbol(vertex->childs[0]->info, VAR, vertex->childs[0]->lineno);
        struct Type* type = type_inh;

        if (var == NULL) {
            errorinfo(3, vertex->childs[0]->lineno, "Redefined variant");
            //create new symbol for further check
            var = malloc(sizeof(struct Symbol));
            var->kind = VAR;
            var->id = malloc(strlen(vertex->childs[0]->info) + 1);
            strcpy(var->id, vertex->childs[0]->info);
            var->first_lineno = vertex->childs[0]->lineno;
        }

        var->type = type;

        return var;
    }
    else {
        struct Type* type = create_type(ARRAY);
        type->array.elem_type = type_inh;
        type->array.size = atoi(vertex->childs[2]->info);
        return VarDec(vertex->childs[0], type);
    }
}

struct Symbol* FunDec(struct Node* vertex, struct Type* type_inh) {  
    SAFE_ID(vertex, "FunDec");                                          
    struct Symbol* func = search_symbol(vertex->childs[0]->info);                              
    struct Symbol* former = NULL;

    if (func == NULL) { //first appear                
        func = create_symbol(vertex->childs[0]->info, PROC, vertex->childs[0]->lineno);        
    }
    else { //appear again, need to check       
        former = func;

        func = malloc(sizeof(struct Symbol));
        memset(func, 0, sizeof(struct Symbol));
        char *id = vertex->childs[0]->info;

        func->id = malloc(strlen(id) + 1);
        strcpy(func->id, id);
        func->kind = PROC;
        func->first_lineno = vertex->childs[0]->lineno;
    }

    func->proc_type.ret_type = type_inh;
    if (CHECK_ID(vertex->childs[2], "VarList")) {
        func_def_flag = true;
        VarList(vertex->childs[2], func, 0);
        func_def_flag = false;
    }

    if (former != NULL) {
        //check
        bool flag = comp_type(func->proc_type.ret_type, former->proc_type.ret_type);
        for (int i = 0; i < MAX_ARGS; ++i) {
            if (func->proc_type.argtype_list[i] != NULL && former->proc_type.argtype_list[i] != NULL)
                flag = flag && comp_type(func->proc_type.argtype_list[i], former->proc_type.argtype_list[i]);
            else if (!(func->proc_type.argtype_list[i] == NULL && former->proc_type.argtype_list[i] == NULL))
                flag = false;
        }

        if (flag == false)
            errorinfo(19, func->first_lineno, "Function declarations clash");
    }

    return func;
}

void VarList(struct Node* vertex, struct Symbol* func, int pos) {
    SAFE_ID(vertex, "VarList");               
    if (func->kind != PROC) {
        panic("Unexpected Non process function id");
    }
    else if (pos >= MAX_ARGS) {
        panic("Too many arguments for a function");
    }

    func->proc_type.argtype_list[pos] = ParamDec(vertex->childs[0])->type; 
    if (vertex->childs[1] != NULL) {
        VarList(vertex->childs[2], func, pos + 1);
    }
}

struct Symbol* ParamDec(struct Node* vertex) {
    SAFE_ID(vertex,"ParamDec");
    struct Type* type_inh = Specifier(vertex->childs[0]);

    return VarDec(vertex->childs[1], type_inh);
}

void DefList(struct Node* vertex, struct FieldList** fl_inh) {   
    SAFE_ID(vertex,"DefList");
    if (vertex->childs[0] != NULL) {
        struct FieldList* fl_syn = Def(vertex->childs[0]);

        if (*fl_inh == NULL) //set as the head
            *fl_inh = fl_syn;
        else { //insert to the tail
            struct FieldList* tail = *fl_inh;
            while (tail->next != NULL) {
                if (struct_def_flag && check_fields(tail->id, fl_syn)) {
                    errorinfo(15, vertex->childs[0]->lineno, "Redefined field");
                }

                tail = tail->next;
            }

            if (struct_def_flag && check_fields(tail->id, fl_syn)) {
                errorinfo(15, vertex->childs[0]->lineno, "Redefined field");
            }
            tail->next = fl_syn;
        }

        //connect sublist to the tail
        DefList(vertex->childs[1], fl_inh);
    }
}

struct FieldList* Def(struct Node* vertex) {
    SAFE_ID(vertex,"Def");
    struct Type* type_inh = Specifier(vertex->childs[0]);

    return DecList(vertex->childs[1], type_inh);
}

struct FieldList* DecList(struct Node* vertex, struct Type* type_inh) {
    SAFE_ID(vertex,"DecList");
    struct Symbol* var = Dec(vertex->childs[0], type_inh);
    struct FieldList* fl_syn = create_field(var->id, var->type);

    if (vertex->childs[2] != NULL) {
        fl_syn->next = DecList(vertex->childs[2], type_inh);
    }

    return fl_syn;
}

struct Symbol* Dec(struct Node* vertex, struct Type* type_inh) {   
    SAFE_ID(vertex,"Dec");
    struct Symbol* var = VarDec(vertex->childs[0], type_inh);

    if (CHECK_ID(vertex->childs[1], "ASSIGNOP")) {
        if (struct_def_flag) {
            errorinfo(15, vertex->lineno, "Cannot initialize field when defining struct type");
        }
        if (comp_type(var->type, Exp(vertex->childs[2]).type)) {
            errorinfo(5, vertex->lineno, "Type of expression is unmatched to the type of variant!");
        }
    }

    return var;
}

bool CompSt(struct Node* vertex, struct Type* type_inh) {
    SAFE_ID(vertex,"CompSt");
    
    struct FieldList* var_def_list;
    DefList(vertex->childs[1], &var_def_list);
    return StmtList(vertex->childs[2], type_inh);
}

bool StmtList(struct Node* vertex, struct Type* type_inh) {
    SAFE_ID(vertex, "StmtList");

    if (vertex->childs[0] == NULL) {
        return false;
    }
    else {
        bool flag1 = Stmt(vertex->childs[0], type_inh);
        bool flag2 = StmtList(vertex->childs[1], type_inh);
        return flag1 || flag2;
    }
}

bool Stmt(struct Node* vertex, struct Type* type_inh) {
    SAFE_ID(vertex, "Stmt");

    if (CHECK_ID(vertex->childs[0], "RETURN")) {
        if (!comp_type(type_inh, Exp(vertex->childs[1]).type)) {
            errorinfo(8, vertex->childs[0]->lineno, "Return type is unmatched to function definition");
        }
        return true;
    }
    else if (CHECK_ID(vertex->childs[0], "CompSt")) {
        return CompSt(vertex->childs[0], type_inh);
    }
    else if (CHECK_ID(vertex->childs[2], "Exp")) { //common pattern of control flow stmts
        if (!comp_type(Exp(vertex->childs[2]).type, INT_PTR)) {
            errorinfo(7, vertex->childs[2]->lineno, "Use non integer expression as judgement condition");
        }
        bool flag = Stmt(vertex->childs[4], type_inh);

        if (CHECK_ID(vertex->childs[6], "Stmt")) {
            flag = flag || Stmt(vertex->childs[6], type_inh);
        }
        return flag;
    }
    else {
        Exp(vertex->childs[0]);
        return false;
    }
}

struct ExpType Exp(struct Node* vertex) {
    SAFE_ID(vertex, "Exp");
    struct ExpType type_syn;
    type_syn.type = NULL;
    type_syn.lvalue = false;

    if (CHECK_ID(vertex->childs[0], "ID") && !CHECK_ID(vertex->childs[1], "LP")) { //Var reference
        struct Symbol* var = search_symbol(vertex->childs[0]->info);
        if (var != NULL) {
            type_syn.type = var->type;
            type_syn.lvalue = true;
        }
        else {
            errorinfo(1, vertex->childs[0]->lineno, "Use undefined variant");
        }
    }
    else if (CHECK_ID(vertex->childs[0], "INT")) {
        type_syn.type = INT_PTR;
    }
    else if (CHECK_ID(vertex->childs[0], "FLOAT")) {
        type_syn.type = FLOAT_PTR;
    }
    else if (CHECK_ID(vertex->childs[1], "ASSIGNOP")) {
        struct ExpType ltype = Exp(vertex->childs[0]);
        struct ExpType rtype = Exp(vertex->childs[2]);

        if (!comp_type(ltype.type, rtype.type)) {
            errorinfo(5, vertex->childs[1]->lineno, "Types of variants besides the '=' are unmatched");
        }
        if (!ltype.lvalue) {
            errorinfo(6, vertex->childs[1]->lineno, "The left operand is not lvalue");
        }
        else {
            type_syn.lvalue = true;
        }
        type_syn.type = ltype.type;
    }
    else if (CHECK_ID(vertex->childs[0], "AND") || CHECK_ID(vertex->childs[0], "OR")) {
        struct ExpType ltype = Exp(vertex->childs[0]);
        struct ExpType rtype = Exp(vertex->childs[2]);

        if (!(comp_type(ltype.type, INT_PTR) && comp_type(rtype.type, INT_PTR))) {
            errorinfo(7, vertex->childs[1]->lineno, "Use non integer expression for logical operation");
        }
        type_syn.type = INT_PTR;
    }
    else if (CHECK_ID(vertex->childs[1], "RELOP")) {
        struct ExpType ltype = Exp(vertex->childs[0]);
        struct ExpType rtype = Exp(vertex->childs[2]);

        if (!comp_type(ltype.type, rtype.type)) {
            errorinfo(7, vertex->childs[1]->lineno, "Types of variants besides the operator are unmatched");
        }
        type_syn.type = INT_PTR;
    }
    else if (CHECK_ID(vertex->childs[1], "PLUS") || CHECK_ID(vertex->childs[1], "MINUS") || CHECK_ID(vertex->childs[1], "STAR") || CHECK_ID(vertex->childs[1], "DIV")) {
        struct ExpType ltype = Exp(vertex->childs[0]);
        struct ExpType rtype = Exp(vertex->childs[2]);
        struct Type* res = NULL;

        if (!comp_type(ltype.type, rtype.type)) {
            errorinfo(7, vertex->childs[1]->lineno, "Types of variants besides the operator are unmatched");
        }

        if (ltype.type->kind != BASIC) {
            errorinfo(7,vertex->childs[0]->lineno, "Type of left operand is invalid, use non basic type expression for arithmetical operation");
        }
        else {
            res = ltype.type;
        }
        if (rtype.type->kind != BASIC) {
            errorinfo(7,vertex->childs[0]->lineno, "Type of right operand is invalid, use non basic type expression for arithmetical operation");
        }
        else if (res == NULL) {
            res = rtype.type;
        }

        if (res == NULL) {
            res = ltype.type;
        }
        type_syn.type = res;
    }
    else if (CHECK_ID(vertex->childs[0], "MINUS")) {
        struct ExpType rtype = Exp(vertex->childs[1]);

        if (rtype.type->kind != BASIC) {
            errorinfo(7, vertex->childs[1]->lineno, "Use non basic type expression for arithmetical operation");
        }
        type_syn.type = rtype.type;
    }
    else if (CHECK_ID(vertex->childs[0], "NOT")) {
        struct ExpType rtype = Exp(vertex->childs[1]);

        if (!(comp_type(rtype.type, INT_PTR))) {
            errorinfo(7, vertex->childs[1]->lineno, "Use non integer expression for logical operation");
        }
        type_syn.type = INT_PTR;
    }
    else if (CHECK_ID(vertex->childs[0], "ID") && CHECK_ID(vertex->childs[1], "LP")) {//function invoking
        struct Symbol* func = search_symbol(vertex->childs[0]->info);
        if (func == NULL) {
            errorinfo(2, vertex->childs[0]->lineno, "Use undefined function");
        }
        else if (func->kind != PROC) {
            errorinfo(11, vertex->childs[0]->lineno, "The identifier is not a function");
        }
        else {
            bool flag = true;

            //check args list
            if (CHECK_ID(vertex->childs[2], "Args")) {
                struct FieldList* argslist = Args(vertex->childs[2]);
                int pos = 0;
                while (argslist != NULL && pos < MAX_ARGS) {
                    struct Type* arg = argslist->type;
                    if (!comp_type(arg, func->proc_type.argtype_list[pos])) {
                        flag = false;
                    }

                    pos++;
                    argslist = argslist->next;
                }

                if (pos == MAX_ARGS && argslist != NULL) {
                    panic("Too many arguments");
                }
                else if (argslist == NULL && pos < MAX_ARGS && func->proc_type.argtype_list[pos] != NULL) {
                    flag = false;
                }
            }
            else if (func->proc_type.argtype_list[0] != NULL) {
                flag = false;
            }

            if (flag == false) {
                errorinfo(9, vertex->childs[2]->lineno, "Arguments are unmatched to the definition of function");
            }
            type_syn.type = func->proc_type.ret_type;
        }
    }
    else if (CHECK_ID(vertex->childs[1], "LB")) {
        struct ExpType id_type = Exp(vertex->childs[0]);
        struct ExpType index_type = Exp(vertex->childs[2]);

        if (!(index_type.type->kind == BASIC && index_type.type->basic == INT)) {
            errorinfo(12, vertex->childs[2]->lineno, "Use non integer expression as array index");
        }

        if (id_type.type->kind != ARRAY) {
            errorinfo(10, vertex->childs[0]->lineno, "The identifier is not an array");
        }
        else {
            type_syn.lvalue = true;
            type_syn.type = id_type.type->array.elem_type;
        }
    }
    else if (CHECK_ID(vertex->childs[1], "DOT")) {
        struct ExpType id_type = Exp(vertex->childs[0]);

        if (id_type.type->kind != STRUCTURE) {
            errorinfo(13, vertex->childs[0]->lineno, "The identifier is not a struct var");
        }
        else {
            struct FieldList* fl = id_type.type->structure;

            bool flag = false;
            while (fl != NULL) {
                if (!strcmp(fl->id, vertex->childs[2]->info)) {   // is equal
                    flag = true;
                    type_syn.lvalue = true;
                    type_syn.type = fl->type;
                    break;
                }

                fl = fl->next;
            }

            if (flag == false) {
                errorinfo(14, vertex->childs[2], "Use undefined field of struct var");
            }
        }
    }

    return type_syn;
}

struct FieldList* Args(struct Node* vertex) {
    SAFE_ID(vertex, "Args");
    struct Type* type = Exp(vertex->childs[0]).type;
    struct FieldList* res = create_field("arg", type);

    if (vertex->childs[2] != NULL && CHECK_ID(vertex->childs[2], "Args")) {
        struct FieldList* types = Args(vertex->childs[2]);
        res->next = types;
    }

    return res;
}

/* operations on data structure for semantic parsing */

// hash function used by symbol table
/*unsigned int hash(char *str) {
    unsigned int val = 0, i = 0;
    for(; *str; ++str) {
        val = (val << 2) + *str;
        if (i = val & ~TABLE_SIZE) val = (val ^ (i >> 12)) & TABLE_SIZE;
    }
    printf("%d\n",val);
    return val;
}*/

// BKDR Hash Function used for symbol table
unsigned int hash(char *str) {
    unsigned int val = 5381;
    while(*str) {
        val += (val << 5) + (*str++);
    }
    return ((val & 0x7fffffff)%TABLE_SIZE);
}

// insert a symbol into the symbol table
void add_symbol(struct Symbol* newItem) {
    if (newItem == NULL)
        panic("Null new item");

    int pos = hash(newItem->id);
    struct SymbolTableItem* head = symbol_table[pos];

    //insert into the top pos
    if (head == NULL) {             
        head = malloc(sizeof(struct SymbolTableItem));
        head->id = newItem;
        head->next = NULL;
        symbol_table[pos] = head;
    }
    else {
        struct SymbolTableItem* temp = malloc(sizeof(struct SymbolTableItem));
        temp->id = newItem;
        temp->next = head;
        symbol_table[pos] = temp;
    }
}

// search the symbol in the symbol table, return NULL, if not found
struct Symbol* search_symbol(char* name) {         
    int pos = hash(name);                                  
    struct SymbolTableItem* head = symbol_table[pos];           

    while (head != NULL) {
        if (CHECK_ID(head->id, name))
            return head->id;

        head = head->next;
    }

    return NULL;
}

// create a Symbol structure variant, return NULL, if the symbol exists
struct Symbol* create_symbol(char* id, int kind, int first_lineno) {
    struct Symbol* temp;
    if (!(struct_def_flag || func_def_flag)) {
        temp = search_symbol(id);
        if (temp != NULL)
            return NULL;
    }

    temp = malloc(sizeof(struct Symbol));
    memset(temp, 0, sizeof(struct Symbol));

    temp->id = malloc(strlen(id) + 1);
    strcpy(temp->id, id);
    temp->kind = kind;
    temp->first_lineno = first_lineno;

    if (!(struct_def_flag || func_def_flag))
        add_symbol(temp);                   
    return temp;
}

// create a Type structure variant
struct Type* create_type(int kind) {
    struct Type* temp = malloc(sizeof(struct Type));
    memset(temp, 0, sizeof(struct Type));

    temp->kind = kind;
    return temp;
}

// create a FieldList structure variant
struct FieldList* create_field(char* id, struct Type* type) {
    struct FieldList* temp = malloc(sizeof(struct FieldList));
    memset(temp, 0, sizeof(struct FieldList));

    temp->id = malloc(strlen(id) + 1);
    strcpy(temp->id, id);
    temp->type = type;

    return temp;
}

void display_symbol() {
    // symbol_table[TABLE_SIZE]
    for(int i = 0; i < TABLE_SIZE; i++) {   
        if(symbol_table[i] != NULL) {                       
            struct SymbolTableItem *p = symbol_table[i];
            while(p != NULL) {
                printf("ID is %s \n",p->id->id);  
                p = p->next;        
            }
        }
    }
}

// compare two Type structure, return true, if they are equal
bool comp_type(struct Type* ltype, struct Type* rtype) {
    if (ltype->kind != rtype->kind)
        return false;

    if (ltype->kind == BASIC) {
        return ltype->basic == rtype->basic;
    }
    else if (ltype->kind = ARRAY) {
        return comp_type(ltype->array.elem_type, rtype->array.elem_type);
    }
    else {
        struct FieldList* lptr = ltype->structure;
        struct FieldList* rptr = rtype->structure;
        while (lptr != NULL && rptr != NULL) { //check each field
            if (!comp_type(lptr->type, rptr->type)) {
                return false;
            }

            lptr = lptr->next;
            rptr = rptr->next;
        }

        if (!(lptr == NULL && rptr == NULL)) { //check number of field
            return false;
        }
    }
}

// check whether the id exists in the field list or not, return true if id exists
bool check_fields(char* id, struct FieldList* head) {
    while (head != NULL) {
        if (strcmp(id, head->id)) {
            return true;
        }

        head = head->next;
    }

    return false;
}

/* operations on syntax tree nodes*/

struct Node* create_node(bool type, char* id, int lineno, const char* info) {
    struct Node* res = malloc(NODE_SIZE);
    memset(res, 0, NODE_SIZE);             // Nodes initialized

    res->type = type;
    res->lineno = lineno;
    
    if (strlen(id) == 0)
        panic("Invalid Id\n");
    res->id = malloc(strlen(id) + 1);
    strcpy(res->id, id);

    res->info = malloc(strlen(info) + 1);
    strcpy(res->info, info);

    return res;
}

void destroy_tree(struct Node* root) {
    free(root->id);
    free(root->info);

    int pos = 0;
    while (pos < MAX_CHILDS && root->childs[pos] != NULL) {
        destroy_tree(root->childs[pos]);
        pos++;
    }
    free(root);
}

void insert(struct Node* dest, struct Node* src) {
    //find a empty position to insert
    int pos = 0;
    while (pos < MAX_CHILDS && dest->childs[pos] != NULL) pos++;

    if (pos == MAX_CHILDS) {
        panic("childs are too many!");
    }
    else {
        dest->childs[pos] = src;
    }
}

void combine(struct Node *dest,int number,...){
    struct Node* src = NULL;
    va_list ap;
    va_start(ap,number); // start behind number
    for(int i = 0; i < number; i++){
        src = va_arg(ap,struct Node *);
        insert(dest, src);
    }
    va_end(ap);
}

void display(struct Node* root,int space) {
    if(root->type == LEXICAL_U) {                // lexical
        for(int k = 0;k < space;k++) {          // printf space
            printf(" ");
        }
        if(strcmp(root->id,"ID") == 0) {          // ID
            printf("%s: %s\n",root->id,root->info);
        }      
        else if(strcmp(root->id,"TYPE") == 0) {   // TYPE
            printf("%s: %s\n",root->id,root->info);
        }
        else if(strcmp(root->id,"INT") == 0) {   // INT
            printf("%s: %d\n",root->id,atoi(root->info));
        }
        else if(strcmp(root->id,"FLOAT") == 0) {   // FLOAT
            printf("%s: %f\n",root->id,atof(root->info));
        }
        else {
            printf("%s\n",root->id);
        }
    }
    else {                                // programmer 
        if(root->childs[0] != NULL) {     // not empty 
            for(int k = 0;k < space;k++) {  // printf space
                printf(" ");
            }
            printf("%s (%d)\n",root->id,root->lineno); 
            space += 2;                  // space add 2
            for(int i = 0;(i < MAX_CHILDS) && (root->childs[i] != NULL);i++) {
                display(root->childs[i],space);
            }
        }
    }
}

void output(struct Node* root) {
    if(root == NULL){
        panic("this tree is empty !\n");
        return;
    }
    int space = 0;
    display(root, space);
}