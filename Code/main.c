#include <stdio.h>

extern FILE* yyin;
extern int yylineno;
extern int yylex();

// main function for flex
int main(int argc, char** argv) {
    if (argc > 1) {
        if (!(yyin = fopen(argv[1], "r"))) {
            perror(argv[1]);
            return 1;
        }
    }
    /* start token analysis */
    yylineno = 1;
    while (yylex() != 0);
    return 0;
}