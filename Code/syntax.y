%{
    #include <stdio.h>
    #include "lex.yy.c"

    //#define YYERROR_VERBOSE

    //extern struct Node;

    extern struct Node* create_node(bool type, char* id, int lineno, const char* info);
    extern void insert(struct Node* dest, struct Node* src);
    extern void combine(struct Node* dest, int number, ...);

    void yyerror(const char *s);

    struct Node* syntax_tree = NULL;
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

/* priority declaration */
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

%%

/* High-level Definitions */
Program : ExtDefList {
    $$ = create_node(GRAM_U, "Program", @$.first_line, "");
    insert($$, $1);
    syntax_tree = $$;
}
    ;
ExtDefList : ExtDef ExtDefList {
    $$ = create_node(GRAM_U, "ExtDefList", @$.first_line, "");
    combine($$, 2, $1, $2);
}
    | {$$ = create_node(GRAM_U, "ExtDefList", @$.first_line, "");}
    ; 
ExtDef : Specifier ExtDecList SEMI {
    $$ = create_node(GRAM_U, "ExtDef", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    | Specifier SEMI {
    $$ = create_node(GRAM_U, "ExtDef", @$.first_line, "");
    combine($$, 2, $1, $2);
}
    | Specifier FunDec CompSt {
    $$ = create_node(GRAM_U, "ExtDef", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    | Specifier FunDec SEMI {
    $$ = create_node(GRAM_U, "ExtDef", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    ;
ExtDecList : VarDec {
    $$ = create_node(GRAM_U, "ExtDecList", @$.first_line, "");
    insert($$, $1);
}
    | VarDec COMMA ExtDecList {
    $$ = create_node(GRAM_U, "ExtDecList", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    ;

/* Specifiers */
Specifier : TYPE {
    $$ = create_node(GRAM_U, "Specifier", @$.first_line, "");
    insert($$, $1);
}
    | StructSpecifier {
    $$ = create_node(GRAM_U, "Specifier", @$.first_line, "");
    insert($$, $1);
}
    ;
StructSpecifier : STRUCT OptTag LC DefList RC {
    $$ = create_node(GRAM_U, "StructSpecifier", @$.first_line, "");
    combine($$, 5, $1, $2, $3, $4, $5);
}
    | STRUCT Tag {
    $$ = create_node(GRAM_U, "StructSpecifier", @$.first_line, "");
    combine($$, 2, $1, $2);
}
    ;
OptTag : ID {
    $$ = create_node(GRAM_U, "OptTag", @$.first_line, "");
    insert($$, $1);
}
    | {$$ = create_node(GRAM_U, "OptTag", @$.first_line, "");}
    ;
Tag : ID {
    $$ = create_node(GRAM_U, "Tag", @$.first_line, "");
    insert($$, $1);
}
    ;

/* Declarators */
VarDec : ID {
    $$ = create_node(GRAM_U, "VarDec", @$.first_line, "");
    insert($$, $1);
}
    | VarDec LB INT RB {
    $$ = create_node(GRAM_U, "VarDec", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    ;
FunDec : ID LP VarList RP {
    $$ = create_node(GRAM_U, "FunDec", @$.first_line, "");
    combine($$, 4, $1, $2, $3, $4);
}
    | ID LP RP {
    $$ = create_node(GRAM_U, "FunDec", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    ;
VarList : ParamDec COMMA VarList {
    $$ = create_node(GRAM_U, "VarList", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    | ParamDec {
    $$ = create_node(GRAM_U, "VarList", @$.first_line, "");
    insert($$, $1);
}
    ;
ParamDec : Specifier VarDec {
    $$ = create_node(GRAM_U, "ParamDec", @$.first_line, "");
    combine($$, 2, $1, $2);
}
    ;

/* Statements */
CompSt : LC DefList StmtList RC {
    $$ = create_node(GRAM_U, "CompSt", @$.first_line, "");
    combine($$, 4, $1, $2, $3, $4);
}
    ;
StmtList : Stmt StmtList {
    $$ = create_node(GRAM_U, "StmtList", @$.first_line, "");
    combine($$, 2, $1, $2);
}
    |  { $$ = create_node(GRAM_U, "StmtList", @$.first_line, ""); }
    ;
Stmt : Exp SEMI {
    $$ = create_node(GRAM_U, "Stmt", @$.first_line, "");
    combine($$, 2, $1, $2);
}
    | CompSt {
    $$ = create_node(GRAM_U, "Stmt", @$.first_line, "");
    insert($$, $1);
}
    | RETURN Exp SEMI {
    $$ = create_node(GRAM_U, "Stmt", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {
    $$ = create_node(GRAM_U, "Stmt", @$.first_line, "");
    combine($$, 5, $1, $2, $3, $4, $5);
}
    | IF LP Exp RP Stmt ELSE Stmt {
    $$ = create_node(GRAM_U, "Stmt", @$.first_line, "");
    combine($$, 7, $1, $2, $3, $4, $5, $6, $7);
}
    | WHILE LP Exp RP Stmt {
    $$ = create_node(GRAM_U, "Stmt", @$.first_line, "");
    combine($$, 5, $1, $2, $3, $4, $5);
}
    ;

/* Local Definitions */
DefList : Def DefList {
    $$ = create_node(GRAM_U, "DefList", @$.first_line, "");
    combine($$, 2, $1, $2);
}
    | { $$ = create_node(GRAM_U, "DefList", @$.first_line, ""); }
    ;
Def : Specifier DecList SEMI {
    $$ = create_node(GRAM_U, "Def", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    ;
DecList : Dec {
    $$ = create_node(GRAM_U, "DecList", @$.first_line, "");
    insert($$, $1);
}
    | Dec COMMA DecList {
    $$ = create_node(GRAM_U, "DecList", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    ;
Dec : VarDec {
    $$ = create_node(GRAM_U, "Dec", @$.first_line, "");
    insert($$, $1);
}
    | VarDec ASSIGNOP Exp {
    $$ = create_node(GRAM_U, "Dec", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    ;

/* Expressions */
Exp : ID {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    insert($$, $1);
}
    | INT {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    insert($$, $1);
}
    | FLOAT {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    insert($$, $1);
}
    | Exp ASSIGNOP Exp {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    | Exp AND Exp {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    | Exp OR Exp {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    | Exp RELOP Exp {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    | Exp PLUS Exp {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");    
    combine($$, 3, $1, $2, $3);
}
    | Exp MINUS Exp {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    | Exp STAR Exp {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    | Exp DIV Exp {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    | LP Exp RP {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    | MINUS Exp {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    combine($$, 2, $1, $2);
}
    | NOT Exp {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    combine($$, 2, $1, $2);
}
    | ID LP Args RP {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    combine($$, 4, $1, $2, $3, $4);
}
    | ID LP RP {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    | Exp LB Exp RB {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, ""); 
    combine($$, 4, $1, $2, $3, $4);
}
    | Exp DOT ID {
    $$ = create_node(GRAM_U, "Exp", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    ;
Args : Exp COMMA Args {
    $$ = create_node(GRAM_U, "Args", @$.first_line, "");
    combine($$, 3, $1, $2, $3);
}
    | Exp {
    $$ = create_node(GRAM_U, "Args", @$.first_line, "");
    insert($$, $1);
}
    ;

%%