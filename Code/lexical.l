%{
    #include <stdio.h>
    #include <stdbool.h>
    #include "syntax.tab.h"

    int yycolumn = 1;
    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;

    /* defined in syntax.y */
    #define LEXICAL_U 1
    #define GRAM_U 0

    int int_func();
    int float_func();
    int id_func();
    int str_func();
    int error_func();

    extern int error_flag;
    extern struct Node* create_node(bool type, char* id, int lineno, const char* info);
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
bcomment    \/\*([^"*"]|("*"[^"/"]))*\*\/
comment     {lcomment}|{bcomment}
inum        (0|[1-9]{digit}*)    
fnum        {inum}\.{digit}+

%%
"\n"        { yycolumn = 1; }
";"         { yylval = create_node(LEXICAL_U,"SEMI",yylineno,yytext); return SEMI; }
","         { yylval = create_node(LEXICAL_U,"COMMA",yylineno,yytext); return COMMA; }
"="         { yylval = create_node(LEXICAL_U,"ASSIGNOP",yylineno,yytext); return ASSIGNOP; }
">"         { yylval = create_node(LEXICAL_U,"RELOP",yylineno,yytext); return RELOP; }
"<"         { yylval = create_node(LEXICAL_U,"RELOP",yylineno,yytext); return RELOP; }
">="        { yylval = create_node(LEXICAL_U,"RELOP",yylineno,yytext); return RELOP; }
"<="        { yylval = create_node(LEXICAL_U,"RELOP",yylineno,yytext); return RELOP; }
"=="        { yylval = create_node(LEXICAL_U,"RELOP",yylineno,yytext); return RELOP; }
"!="        { yylval = create_node(LEXICAL_U,"RELOP",yylineno,yytext); return RELOP; }
"+"         { yylval = create_node(LEXICAL_U,"PLUS",yylineno,yytext); return PLUS; }
"-"         { yylval = create_node(LEXICAL_U,"MINUS",yylineno,yytext); return MINUS; }
"*"         { yylval = create_node(LEXICAL_U,"STAR",yylineno,yytext); return STAR; }
"/"         { yylval = create_node(LEXICAL_U,"DIV",yylineno,yytext); return DIV; }
"&&"        { yylval = create_node(LEXICAL_U,"AND",yylineno,yytext); return AND; }
"||"        { yylval = create_node(LEXICAL_U,"OR",yylineno,yytext); return OR; }
"."         { yylval = create_node(LEXICAL_U,"DOT",yylineno,yytext); return DOT; }
"!"         { yylval = create_node(LEXICAL_U,"NOT",yylineno,yytext); return NOT; }
"("         { yylval = create_node(LEXICAL_U,"LP",yylineno,yytext); return LP; }
")"         { yylval = create_node(LEXICAL_U,"RP",yylineno,yytext); return RP; }
"["         { yylval = create_node(LEXICAL_U,"LB",yylineno,yytext); return LB; }
"]"         { yylval = create_node(LEXICAL_U,"RB",yylineno,yytext); return RB; }
"{"         { yylval = create_node(LEXICAL_U,"LC",yylineno,yytext); return LC; }
"}"         { yylval = create_node(LEXICAL_U,"RC",yylineno,yytext); return RC; }
"float"     { yylval = create_node(LEXICAL_U,"TYPE",yylineno,yytext); return TYPE; }
"int"       { yylval = create_node(LEXICAL_U,"TYPE",yylineno,yytext); return TYPE; }
"struct"    { yylval = create_node(LEXICAL_U,"STRUCT",yylineno,yytext); return STRUCT; }
"return"    { yylval = create_node(LEXICAL_U,"RETURN",yylineno,yytext); return RETURN; }
"if"        { yylval = create_node(LEXICAL_U,"IF",yylineno,yytext); return IF; }
"else"      { yylval = create_node(LEXICAL_U,"ELSE",yylineno,yytext); return ELSE; }
"while"     { yylval = create_node(LEXICAL_U,"WHILE",yylineno,yytext); return WHILE; }
{inum}      { int_func(); return INT; }
{fnum}      { float_func(); return FLOAT; }
{id}        { id_func(); return ID; }
{ws}        {}
{str}       { str_func(); }
{lcomment}  {}
{bcomment}  {}
.           { error_func(); }

%%

int int_func()
{
    yylval = create_node(LEXICAL_U, "INT", yylineno, yytext);
    return 1;
}

int float_func()
{
    yylval = create_node(LEXICAL_U, "FLOAT", yylineno, yytext);
    return 1;
}

int id_func()
{
    yylval = create_node(LEXICAL_U, "ID", yylineno, yytext);
    return 1;
}

int str_func()
{
    printf("this str %s \n", yytext);
    return 1;
}

int error_func()
{
    printf("Error type A at Line %d: Mysterious character \"%s\"\n", yylineno, yytext);
    error_flag = 1;
    return 1;
}