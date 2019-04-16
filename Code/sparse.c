#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>

/* type and constant value definitions */

static const int LEXICAL_U = 1;
static const int GRAM_U = 0;
static const int INT = 0;
static const int FLOAT = 1;

static const unsigned int MAX_CHILDS = 8;
static const unsigned int MAX_ARGS = 10;
static const unsigned int TABLE_SIZE = 16;

#define NODE_SIZE sizeof(struct Node)
#define CHECK_ID(vertex, name) !strcmp(vertex->id, name)
#define INT_PTR &INT_T
#define FLOAT_PTR &FLOAT_T

enum MetaType { BASIC, ARRAY, STRUCTURE };
enum SymbolMetaType { VAR, PROC, USER_TYPE };

struct Node {
    bool type; //[Lexical Unit]:true, [Grammatical Unit]:false
    char* id; //[Lexical Unit]:token, [Grammatical Unit]:non-terminals
    int lineno; //line number
    char* info; //[Lexical Unit]:details, [Grammatical Unit]:undefined
    struct Node* childs[MAX_CHILDS]; //pointers to child nodes
};

struct FieldList {
    char* id;
    struct Type* type;
    struct FieldList* next;
};

struct Type {
    enum MetaType kind;
    union {
        int basic; //0-int, 1-float
        struct { struct Type* elem_type; int size; } array;
        struct FieldList* structure;
    };
};

struct Symbol {
    char* id;
    enum SymbolMetaType kind;
    bool defined;
    union {
        struct Type* type;
        struct {
            struct Type* ret_type;
            struct Type* argtype_list[MAX_ARGS];
        } proc_type;
    };
};

struct SymbolTableItem {
    struct Symbol* id;
    struct SymbolTableItem* next;
};

/* global variant definitions */

static struct SymbolTableItem* symbol_table[TABLE_SIZE]; //A hash table

const static struct Type INT_T = { BASIC, INT };
const static struct Type FLOAT_T = { BASIC, FLOAT };

/* function declarations */

void init();
void visit(struct Node* vertex);
void errorinfo(int type, int lineno, char* description);

void add(struct Symbol* newItem);
struct Symbol* search(char* name);
struct Symbol* create_symbol(char* id, int kind);

/* traverse functions */

void semantic_parse(struct Node* root) {
    init();
    visit(root);
}

void init() {

}

void panic(char* msg) {
    fprintf(stderr, "%s\n", msg);
    assert(0);
}

void visit(struct Node* vertex) {
    if (vertex == NULL)
        panic("Null Vertex Pointer");

    if (CHECK_ID(vertex, "ExtDef")) {
        ExtDef(vertex);
    }
    else if (CHECK_ID(vertex, "DefList")) {
        DefList(vertex);
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
    struct Type* type_inh = Specifier(vertex->childs[0]);

    if (CHECK_ID(vertex->childs[1], "ExtDecList")) {
        ExtDecList(vertex->childs[1], type_inh);
    }
    else if (CHECK_ID(vertex->childs[1], "FunDec")) {
        struct Symbol* func = FuncDec(vertex->childs[1], type_inh);

        if (CHECK_ID(vertex->childs[2], "CompSt") && func != NULL) {
            func->defined = true;
        }
    }
}

struct Type* Specifier(struct Node* vertex) {
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
    if (CHECK_ID(vertex->childs[1], "Tag")) { //struct def reference
        struct Symbol* id = search(vertex->childs[1]->childs[0]);
        if (id->kind != USER_TYPE)
            errorinfo(17, vertex->childs[1]->lineno, "Invalid struct type");

        struct Type* type = id->type;
        return type;
    }
    else { //struct definition
        struct Symbol* id = create_symbol(vertex->childs[1]->childs[0]->info, USER_TYPE);
        struct Type* type = create_type(STRUCTURE);
        type->structure = DefList(vertex->childs[3]);

        if (id == NULL) {
            errorinfo(16, vertex->childs[1]->lineno, "Redefined struct identifier");
        }
        else
            id->type = type;

        return type;
    }
}

void ExtDecList(struct Node* vertex, struct Type* type_inh) {
    VarDec(vertex->childs[0], type_inh);

    if (vertex->childs[2] != NULL) {
        ExtDecList(vertex->childs[2], type_inh);
    }
}

struct Symbol* VarDec(struct Node* vertex, struct Type* type_inh) {
    if (CHECK_ID(vertex->childs[0], "ID")) {
        struct Symbol* id = create_symbol(vertex->childs[0]->info, VAR);
        struct Type* type = type_inh;

        if (id == NULL) {
            errorinfo(3, vertex->childs[0]->lineno, "Redefined variant");
        }
        else
            id->type = type;

        return id;
    }
    else {
        struct Type* type = create_type(ARRAY);
        type->array.elem_type = type_inh;
        type->array.size = atoi(vertex->childs[2]->info);
        return VarDec(vertex->childs[0], type);
    }
}

struct Symbol* FuncDec(struct Node* vertex, struct Type* type_inh) {
    struct Symbol* func = create_symbol(vertex->childs[0]->info, PROC);
    if (func == NULL) {
        errorinfo(4, vertex->childs[0]->lineno, "Redefined function");
        return func;
    }

    func->proc_type.ret_type = type_inh;
    if (CHECK_ID(vertex->childs[2], "RP")) {
        return func;
    }
    else {
        VarList(vertex->childs[2], func, 0);
        return func;
    }
}

void VarList(struct Node* vertex, struct Symbol* func, int pos) {
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
    struct Type* type_inh = Specifier(vertex->childs[0]);

    return VarDec(vertex, type_inh);
}

struct FieldList* DefList(struct Node* vertex) {
    struct FieldList* fl_syn = NULL;
    if (vertex->childs[0] != NULL) {
        fl_syn = Def(vertex->childs[0]);
        struct FieldList* tail = fl_syn;
        while (tail->next != NULL) tail = tail->next;

        //connect sublist to the tail
        tail->next = DefList(vertex->childs[1]);
    }
    return fl_syn;
}

struct FieldList* Def(struct Node* vertex) {
    struct Type* type_inh = Specifier(vertex->childs[0]);

    return DecList(vertex->childs[1], type_inh);
}

struct FieldList* DecList(struct Node* vertex, struct Type* type_inh) {
    struct Symbol* var = Dec(vertex->childs[0], type_inh);
    struct FieldList* fl_syn = create_field(var->id, var->type);

    if (vertex->childs[2] != NULL) {
        fl_syn->next = DecList(vertex->childs[2], type_inh);
    }

    return fl_syn;
}

struct Symbol* Dec(struct Node* vertex, struct Type* type_inh) {
    struct Symbol* var = VarDec(vertex->childs[0], type_inh);

    /* TODO: check = Exp */

    return var;
}

/* operations on symbol table */

unsigned int hash(char *str) {
    unsigned int val = 0, i = 0;
    while (*str != 0) {
        val = (val << 2) + *str;
        if (i = val & ~TABLE_SIZE) val = (val ^ (i >> 12)) & TABLE_SIZE;
    }

    return val;
}

void add(struct Symbol* newItem) {
    if (newItem == NULL)
        panic("Null new item");

    int pos = hash(newItem->id);
    struct SymbolTableItem* head = symbol_table[pos];

    //insert into the top pos
    if (head == NULL) {
        head = malloc(sizeof(struct SymbolTableItem));
        head->id = newItem;
        head->next = NULL;
    }
    else {
        struct SymbolTableItem* temp = malloc(sizeof(struct SymbolTableItem));
        temp->id = newItem;
        temp->next = head;
        symbol_table[pos] = temp;
    }
}

struct Symbol* search(char* name) {
    int pos = hash(name);
    struct SymbolTableItem* head = symbol_table[pos];

    while (head != NULL) {
        if (CHECK_ID(head->id, name))
            return head->id;

        head = head->next;
    }

    return NULL;
}

struct Symbol* create_symbol(char* id, int kind) {
    struct Symbol* temp = search(id);
    if (temp != NULL)
        return NULL;

    temp = malloc(sizeof(struct Symbol));
    memset(temp, 0, sizeof(struct Symbol));

    temp->id = malloc(strlen(id) + 1);
    strcpy(temp->id, id);
    temp->kind = kind;

    add(temp);
    return temp;
}

struct Type* create_type(int kind) {
    struct Type* temp = malloc(sizeof(struct Type));
    memset(temp, 0, sizeof(struct Type));

    temp->kind = kind;
    return temp;
}

struct FieldList* create_field(char* id, struct Type* type) {
    struct FieldList* temp = malloc(sizeof(struct FieldList));
    memset(temp, 0, sizeof(struct FieldList));

    temp->id = malloc(strlen(id) + 1);
    strcpy(temp->id, id);
    temp->type = type;

    return temp;
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