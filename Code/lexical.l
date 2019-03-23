%{
    #include "syntax.tab.h"

    int yycolumn = 1;
    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;

    /* defined in syntax.y */
    #define LEXICAL_U true
    #define GRAMM_U false

    extern struct Node;
    extern struct Node* create_node(bool type, char* id, int lineno, char* info);

    int int_func();
    int float_func();
    int id_func();
    int str_func();
    int error_func();
%}

%option  yylineno

/* regular definitions */
delim       [ \t\r]
ws          {delim}+
letter      [a-zA-Z]
digit       [0-9]
id          (_|{letter})(_|{letter}|{digit})*           
str         \"(.)*\"
lcomment    \/\/.*$
bcomment    \/\*(.|\n)*\*\/
comment     {lcomment}|{bcomment}

/* Tokens definitions  */
inum        (0|[1-9]{digit}*)    
fnum        {inum}\.{digit}+

%%
"\n"        { yycolumn = 1; }
";"         { printf("%s\n", yytext); return SEMI; }
","         { printf("%s\n", yytext); return COMMA; }
"="         { printf("%s\n", yytext); return ASSIGNOP; }
">"         { printf("%s\n", yytext); return RELOP; }
"<"         { printf("%s\n", yytext); return RELOP; }
">="        { printf("%s\n", yytext); return RELOP; }
"<="        { printf("%s\n", yytext); return RELOP; }
"=="        { printf("%s\n", yytext); return RELOP; }
"!="        { printf("%s\n", yytext); return RELOP; }
"+"         { printf("%s\n", yytext); return PLUS; }
"-"         { printf("%s\n", yytext); return MINUS; }
"*"         { printf("%s\n", yytext); return STAR; }
"/"         { printf("%s\n", yytext); return DIV; }
"&&"        { printf("%s\n", yytext); return AND; }
"||"        { printf("%s\n", yytext); return OR; }
"."         { printf("%s\n", yytext); return DOT; }
"!"         { printf("%s\n", yytext); return NOT; }
"("         { printf("%s\n", yytext); return LP; }
")"         { printf("%s\n", yytext); return RP; }
"["         { printf("%s\n", yytext); return LB; }
"]"         { printf("%s\n", yytext); return RB; }
"{"         { printf("%s\n", yytext); return LC; }
"}"         { printf("%s\n", yytext); return RC; }
"float"     { printf("float \n"); return TYPE; }
"int"       { printf("int \n"); return TYPE; }
"struct"    { printf("struct \n"); return STRUCT; }
"return"    { printf("return \n"); return RETURN; }
"if"        { printf("if \n"); return IF; }
"else"      { printf("else \n"); return ELSE; }
"while"     { printf("while \n"); return WHILE; }
{inum}      { int_func(); return INT; }
{fnum}      { float_func(); return FLOAT; }
{id}        { id_func(); return ID; }
{ws}        {}
{str}       { str_func(); }
{lcomment}  { printf("line comment %s\n", yytext); }
{bcomment}  { printf("block comment %s\n", yytext); }
.           { error_func(); }

%%

int int_func()
{
    yylval = create_node(LEXICAL_U, "INT", lineno, yytext);
    printf("integer %d\n", atoi(yytext));
    return 1;
}

int float_func()
{
    yylval = create_node(LEXICAL_U, "FLOAT", lineno, yytext);
    printf("float %f\n", atof(yytext));
    return 1;
}

int id_func()
{
    yylval = create_node(LEXICAL_U, "ID", lineno, yytext);
    printf("id %s\n", yytext);
    return 1;
}

int str_func()
{
    printf("str %s \n", yytext);
    return 1;
}

int error_func()
{
    printf("Error type A at Line %d: Mysterious character \"%s\"\n", yylineno, yytext);
    return 1;
}