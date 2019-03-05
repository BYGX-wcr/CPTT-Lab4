%{
    int int_func();
    int float_func();
    int id_func();
    int str_func();
    int error_func();
%}

%option  yylineno

/* regular definitions */
delim       [ \t\r\n]
ws          {delim}+
letter      [a-zA-Z]
digit       [0-9]
id          (_|{letter})(_|{letter}|{digit})*           
str         \"(.)*\"
lcomment    \/\/.*$
bcomment    \/\*(.|\n)*\*\/
comment     {lcomment}|{bcomment}

/* Tokens definitions  */
INT         (0|[1-9]{digit}*)    
FLOAT       {INT}\.{digit}+

%%
";"         {printf("%s\n",yytext);}
","         {printf("%s\n",yytext);}
"="         {printf("%s\n",yytext);}
">"         {printf("%s\n",yytext);}
"<"         {printf("%s\n",yytext);}
">="        {printf("%s\n",yytext);}
"<="        {printf("%s\n",yytext);}
"=="        {printf("%s\n",yytext);}
"!="        {printf("%s\n",yytext);}
"+"         {printf("%s\n",yytext);}
"-"         {printf("%s\n",yytext);}
"/"         {printf("%s\n",yytext);}
"&&"        {printf("%s\n",yytext);}
"||"        {printf("%s\n",yytext);}
"."         {printf("%s\n",yytext);}
"!"         {printf("%s\n",yytext);}
"("         {printf("%s\n",yytext);}
")"         {printf("%s\n",yytext);}
"["         {printf("%s\n",yytext);}
"]"         {printf("%s\n",yytext);}
"{"         {printf("%s\n",yytext);}
"}"         {printf("%s\n",yytext);}
"float"     {printf("float\n");}
"int"       {printf("int\n");}
"struct"    {printf("struct\n");}
"return"    {printf("return\n");}
"if"        {printf("if\n");}
"else"      {printf("else\n");}
"while"     {printf("while\n");}
{INT}       {int_func();}
{FLOAT}     {float_func();}
{id}        {id_func();}
{ws}        {}
{str}       {str_func();}
{lcomment}  {printf("line comment %s\n", yytext);}
{bcomment}  {printf("block comment %s\n", yytext);}
.           {error_func();}

%%

int int_func()
{
    printf("INT %d\n", atoi(yytext));
    return 1;
}

int float_func()
{
    printf("FLOAT %s\n", yytext);
    return 1;
}

int id_func()
{
    printf("ID %s\n", yytext);
    return 1;
}

int str_func()
{
    printf("STR %s \n", yytext);
    return 1;
}

int error_func()
{
    printf("Error type A at Line %d: Mysterious character \"%s\"\n", yylineno, yytext);
    return 1;
}