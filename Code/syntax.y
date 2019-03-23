%{
    #include <stdio.h>
    #include <memory.h>
    #include <string.h>
    #include <assert.h>
    #include "lex.yy.c"

    struct Node {
        bool type; //[Lexical Unit]:true, [Grammatical Unit]:false
        char* id; //[Lexical Unit]:token, [Grammatical Unit]:non-terminals
        int lineno; //line number
        char* info; //[Lexical Unit]:details, [Grammatical Unit]:undefined
        struct Node* childs[MAX_CHILDS]; //pointers to child nodes
    };

    #define YYSTYPE struct Node*

    #define MAX_CHILDS 8
    #define NODE_SIZE sizeof(struct Node)

    void yyerror(char* msg);
    void panic(char* msg);
    struct Node* create_node(bool type, char* id, int lineno, char* info);
    insert(struct Node* dest, struct Node* src);

    struct Node* syntax_tree = NULL;

    // struct Node* syntax_stack = NULL;
    // int length = 0;
    // int size = 0;
%}

%locations

/* declared tokens */
%token INT FLOAT ID
%token SEMI COMMA
%token ASSIGNOP
%token RELOP
%token PLUS MINUS STAR DIV
%token AND OR NOT
%token DOT
%token TYPE
%token LP RP LB RB LC RC
%token STRUCT RETURN IF ELSE WHILE

/* Priority declaration */
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%right ASSIGNOP 
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT            
%left DOT LP RP LB RB

/* declared non-terminals */

%%

/* High-level Definitions */
Program : ExtDefList {
    $$ = create_node(GRAM_U, "Program", @1.first_line, "");
    insert($$, $1);
    syntax_tree = $$;
}
    ;
ExtDefList : ExtDef ExtDefList {
    $$ = create_node(GRAM_U, "ExtDefList", @1.first_line, "");
    insert($$, $1);
    insert($$, $2);
}
    |
    ; 
ExtDef : Specifier ExtDecList SEMI
    | Specifier SEMI
    | Specifier FunDec CompSt
    ;
ExtDecList : VarDec
    | VarDec COMMA ExtDecList
    ;

/* Specifiers */
Specifier : TYPE
    | StructSpecifier
    ;
StructSpecifier : STRUCT OptTag LC DefList RC
    | STRUCT Tag
    ;
OptTag : ID
    |
    ; 
Tag : ID
    ;

/* Declarators */
VarDec : ID
    | VarDec LB INT RB
    ;
FunDec : ID LP VarList RP
    | ID LP RP
    ;
VarList : ParamDec COMMA VarList
    | ParamDec
    ;
ParamDec : Specifier VarDec
    ;

/* Statements */
CompSt : LC DefList StmtList RC
    ;
StmtList : Stmt StmtList
    |
    ;
Stmt : Exp SEMI
    | CompSt
    | RETURN Exp SEMI
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
    | IF LP Exp RP Stmt ELSE Stmt
    | WHILE LP Exp RP Stmt
    ;

/* Local Definitions */
DefList : Def DefList
    |
    ;
Def : Specifier DecList SEMI
    ;
DecList : Dec
    | Dec COMMA DecList
    ;
Dec : VarDec
    | VarDec ASSIGNOP Exp
    ;

/* Expressions */
Exp : Exp ASSIGNOP Exp
    | Exp AND Exp
    | Exp OR Exp
    | Exp RELOP Exp
    | Exp PLUS Exp
    | Exp MINUS Exp
    | Exp STAR Exp
    | Exp DIV Exp
    | LP Exp RP
    | MINUS Exp
    | NOT Exp
    | ID LP Args RP
    | ID LP RP
    | Exp LB Exp RB
    | Exp DOT ID
    | ID {
        $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
        insert(exp, $1);
    }
    | INT
    | FLOAT
    ;
Args : Exp COMMA Args
    | Exp
    ;

%%

void yyerror(char* msg) {
    fprintf(stderr, "syntax error: %s\n", msg);
}

void panic(char* msg) {
    fprintf(stderr, msg);
    assert(0);
}

/* operations on tree nodes*/

struct Node* create_node(bool type, char* id, int lineno, char* info) {
    struct Node* res = malloc(NODE_SIZE);
    memset(res, 0, NODE_SIZE);

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

void remove(struct Node* root) {
    free(root->id);
    free(root->info);

    int pos = 0;
    while (pos < MAX_CHILDS && root->childs[pos] != NULL) {
        remove(root->childs[pos]);
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

/* operations on syntax stack */

// void expand() {
//     //allocate new space
//     if (size == 0)
//         size = 8;
//     else
//         size <<= 1;
//     struct Node* new_space = malloc(size * sizeof(void *));
//     memset(new_space, 0, size * sizeof(void *));

//     //copy data & release space
//     memcpy(new_space, syntax_stack, length * sizeof(void *));
//     free(syntax_stack);
// }

// void push(struct Node* ptr) {
//     if (syntax_stack == NULL) {
//         panic("syntax stack uninitiated!\n");
//     }
//     else if (length == size) {
//         expand(); // enlarge the size of stack
//     }

//     syntax_stack[length] = ptr;
//     length++;
// }

// struct Node* pop() {
//     if (syntax_stack == NULL) {
//         panic("syntax stack uninitiated!\n");
//     }
//     else if (length == 0) {
//         panic("syntax stack underflow!\n");
//     }

//     length--;
//     struct Node* res = syntax_stack[length];

//     return res;
// }