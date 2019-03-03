%{
    int num_func();
    int id_func();
    int str_func();
    int error_func();
%}

/* regular definitions */
delim       [ \n\t]
ws          {delim}+
letter      [a-zA-Z]
digit       [0-9]
id          (_|{letter})(_|{letter}|{digit})*
num         [1-9]{digit}*
str         \"(.)*\"
lcomment    \/\/.*$
bcomment    \/\*(.|\n)*\*\/
comment     {lcomment}|{bcomment}

%%

{num}       {num_func();}
{id}        {id_func();}
{ws}        {}
{str}       {str_func();}
{lcomment}  {printf("Find a line comment %s\n", yytext);}
{bcomment}  {printf("Find a block comment %s\n", yytext);}
.           {error_func();}

%%

int num_func()
{
    printf("Find a number %d\n", atoi(yytext));
    return 1;
}

int id_func()
{
    printf("Find a identifier %s\n", yytext);
    return 1;
}

int str_func()
{
    printf("Find a string %s\n", yytext);
    return 1;
}

int error_func()
{
    return 1;
}