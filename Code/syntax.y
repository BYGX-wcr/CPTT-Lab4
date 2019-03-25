%{
    #include <stdio.h>
    #include <memory.h>
    #include <string.h>
    #include <assert.h>
    #include <stdarg.h>
    #include "lex.yy.c"

    #define MAX_CHILDS 8
    #define NODE_SIZE sizeof(struct Node)

    struct Node {
        bool type; //[Lexical Unit]:true, [Grammatical Unit]:false
        char* id; //[Lexical Unit]:token, [Grammatical Unit]:non-terminals
        int lineno; //line number
        char* info; //[Lexical Unit]:details, [Grammatical Unit]:undefined
        struct Node* childs[MAX_CHILDS]; //pointers to child nodes
    };

    void yyerror(char* msg);
    void panic(char* msg);
    struct Node* create_node(bool type, char* id, int lineno, const char* info);
    void insert(struct Node* dest, struct Node* src);
    void combine(struct Node* dest,int number,...);
    void destroy_tree(struct Node* root);
    void output(struct Node* root);


    struct Node* syntax_tree = NULL;

    // struct Node* syntax_stack = NULL;
    // int length = 0;
    // int size = 0;
%}

%locations

%define api.value.type { struct Node * }

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
    combine($$,2,$1,$2);
}
    |
    ; 
ExtDef : Specifier ExtDecList SEMI {
    $$ = create_node(GRAM_U,"ExtDef",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    | Specifier SEMI {
    $$ = create_node(GRAM_U,"ExtDef",@1.first_line,"");
    combine($$,2,$1,$2);
}
    | Specifier FunDec CompSt {
    $$ = create_node(GRAM_U,"ExtDef",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    ;
ExtDecList : VarDec {
    $$ = create_node(GRAM_U,"ExtDecList",@1.first_line,"");
    insert($$,$1);
}
    | VarDec COMMA ExtDecList {
    $$ = create_node(GRAM_U,"ExtDecList",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    ;

/* Specifiers */
Specifier : TYPE {
    $$ = create_node(GRAM_U,"Specifier",@1.first_line,"");
    insert($$,$1);
}
    | StructSpecifier {
    $$ = create_node(GRAM_U,"Specifier",@1.first_line,"");
    insert($$,$1);
}
    ;
StructSpecifier : STRUCT OptTag LC DefList RC {
    $$ = create_node(GRAM_U,"StructSpecifier",@1.first_line,"");
    combine($$,5,$1,$2,$3,$4,$5);
}
    | STRUCT Tag {
    $$ = create_node(GRAM_U,"StructSpecifier",@1.first_line,"");
    combine($$,2,$1,$2);
}
    ;
OptTag : ID {
    $$ = create_node(GRAM_U,"OptTag",@1.first_line,"");
    insert($$,$1);
}
    |
    ; 
Tag : ID {
    $$ = create_node(GRAM_U,"Tag",@1.first_line,"");
    insert($$,$1);
}
    ;

/* Declarators */
VarDec : ID {
    $$ = create_node(GRAM_U,"VarDec",@1.first_line,"");
    insert($$,$1);
}
    | VarDec LB INT RB {
    $$ = create_node(GRAM_U,"VarDec",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    ;
FunDec : ID LP VarList RP {
    $$ = create_node(GRAM_U,"FunDec",@1.first_line,"");
    combine($$,4,$1,$2,$3,$4);
}
    | ID LP RP {
    $$ = create_node(GRAM_U,"FunDec",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    ;
VarList : ParamDec COMMA VarList {
    $$ = create_node(GRAM_U,"VarList",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    | ParamDec {
    $$ = create_node(GRAM_U,"VarList",@1.first_line,"");
    insert($$,$1);
}
    ;
ParamDec : Specifier VarDec {
    $$ = create_node(GRAM_U,"ParamDec",@1.first_line,"");
    combine($$,2,$1,$2);
}
    ;

/* Statements */
CompSt : LC DefList StmtList RC {
    $$ = create_node(GRAM_U,"CompSt",@1.first_line,"");
    combine($$,4,$1,$2,$3,$4);
}
    ;
StmtList : Stmt StmtList {
    $$ = create_node(GRAM_U,"StmtList",@1.first_line,"");
    combine($$,2,$1,$2);
}
    |
    ;
Stmt : Exp SEMI {
    $$ = create_node(GRAM_U,"Stmt",@1.first_line,"");
    combine($$,2,$1,$2);
}
    | CompSt {
    $$ = create_node(GRAM_U,"Stmt",@1.first_line,"");
    insert($$,$1);
}
    | RETURN Exp SEMI {
    $$ = create_node(GRAM_U,"Stmt",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {
    $$ = create_node(GRAM_U,"Stmt",@1.first_line,"");
    combine($$,5,$1,$2,$3,$4,$5);
}
    | IF LP Exp RP Stmt ELSE Stmt {
    $$ = create_node(GRAM_U,"Stmt",@1.first_line,"");
    combine($$,7,$1,$2,$3,$4,$5,$6,$7);
}
    | WHILE LP Exp RP Stmt {
    $$ = create_node(GRAM_U,"Stmt",@1.first_line,"");
    combine($$,5,$1,$2,$3,$4,$5);
}
    ;

/* Local Definitions */
DefList : Def DefList {
    $$ = create_node(GRAM_U,"DefList",@1.first_line,"");
    combine($$,2,$1,$2);
}
    |
    ;
Def : Specifier DecList SEMI {
    $$ = create_node(GRAM_U,"Def",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    ;
DecList : Dec {
    $$ = create_node(GRAM_U,"DecList",@1.first_line,"");
    insert($$,$1);
}
    | Dec COMMA DecList {
    $$ = create_node(GRAM_U,"DecList",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    ;
Dec : VarDec {
    $$ = create_node(GRAM_U,"Dec",@1.first_line,"");
    insert($$,$1);
}
    | VarDec ASSIGNOP Exp {
    $$ = create_node(GRAM_U,"Dec",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    ;

/* Expressions */
Exp : Exp ASSIGNOP Exp {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    | Exp AND Exp {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    | Exp OR Exp {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    | Exp RELOP Exp {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    | Exp PLUS Exp {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");    
    combine($$,3,$1,$2,$3);
}
    | Exp MINUS Exp {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    | Exp STAR Exp {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    | Exp DIV Exp {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    | LP Exp RP {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    | MINUS Exp {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");
    combine($$,2,$1,$2);
}
    | NOT Exp {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");
    {
        combine($$,2,$1,$2);
    }
}
    | ID LP Args RP {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");
    combine($$,4,$1,$2,$3,$4);
}
    | ID LP RP {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    | Exp LB Exp RB {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,""); 
    combine($$,4,$1,$2,$3,$4);
}
    | Exp DOT ID {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    | ID {
        $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
        insert($$, $1);
}
    | INT {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");
    insert($$,$1);
}
    | FLOAT {
    $$ = create_node(GRAM_U,"Exp",@1.first_line,"");
    insert($$,$1);
}
    ;
Args : Exp COMMA Args {
    $$ = create_node(GRAM_U,"Args",@1.first_line,"");
    combine($$,3,$1,$2,$3);
}
    | Exp {
    $$ = create_node(GRAM_U,"Args",@1.first_line,"");
    insert($$,$1);    
}
    ;

%%

void yyerror(char* msg) {
    fprintf(stderr, "Error type B  %s\n",msg);
}

void panic(char* msg) {
    fprintf(stderr,"%s \n", msg);
    assert(0);
}

/* operations on tree nodes*/

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
        insert(dest,src);
    }
    va_end(ap);
}

void output(struct Node* root) {
    if(root->type == LEXICAL_U) {
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
    else {
        for(int i = 0;(i < MAX_CHILDS) && (root->childs[i] != NULL);i++) {
            output(root->childs[i]);
            struct Node *p = root->childs[i];
            printf("%s \n",p->id);
        }
    }
}

/*
void Sum(int Num, ...) {
	char *T = NULL;
	va_list ap;
	va_start(ap, Num);
	for (int i = 0; i < Num; i++)
	{
		T = va_arg(ap, char *);
		printf("%s \n", T);
	}
	va_end(ap);
}
*/


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