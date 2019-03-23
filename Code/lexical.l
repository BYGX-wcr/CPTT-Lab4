%{
    #include "syntax.tab.h"

    int yycolumn = 1;
    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;

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
";"         { printf("%s\n", yytext); }
","         { printf("%s\n", yytext); }
"="         { printf("%s\n", yytext); }
">"         { printf("%s\n", yytext); }
"<"         { printf("%s\n", yytext); }
">="        { printf("%s\n", yytext); }
"<="        { printf("%s\n", yytext); }
"=="        { printf("%s\n", yytext); }
"!="        { printf("%s\n", yytext); }
"+"         { printf("%s\n", yytext); return PLUS; }
"-"         { printf("%s\n", yytext); return MINUS; }
"*"         { printf("%s\n", yytext); return STAR; }
"/"         { printf("%s\n", yytext); return DIV; }
"&&"        { printf("%s\n", yytext); }
"||"        { printf("%s\n", yytext); }
"."         { printf("%s\n", yytext); }
"!"         { printf("%s\n", yytext); }
"("         { printf("%s\n", yytext); }
")"         { printf("%s\n", yytext); }
"["         { printf("%s\n", yytext); }
"]"         { printf("%s\n", yytext); }
"{"         { printf("%s\n", yytext); }
"}"         { printf("%s\n", yytext); }
"float"     { printf("float\n"); return TYPE; }
"int"       { printf("int\n"); return TYPE; }
"struct"    { printf("struct\n"); }
"return"    { printf("return\n"); }
"if"        { printf("if\n"); }
"else"      { printf("else\n"); }
"while"     { printf("while\n"); }
{inum}      { int_func(); return INT; }
{fnum}      { float_func(); return FLOAT; }
{id}        { id_func(); }
{ws}        {}
{str}       { str_func(); }
{lcomment}  { printf("line comment %s\n", yytext); }
{bcomment}  { printf("block comment %s\n", yytext); }
.           { error_func(); }

%%

int int_func()
{
    yylval.type_int = atoi(yytext);
    printf("integer %d\n", atoi(yytext));
    return 1;
}

int float_func()
{
    yylval.type_float = atof(yytext);
    printf("float %s\n", yytext);
    return 1;
}

int id_func()
{
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