#include <stdio.h>

extern FILE* yyin;

// main function for flex
int main(int argc, char** argv) {
    if (argc > 1) {
        if (!(yyin = fopen(argv[1], "r"))) {
            perror(argv[1]);
            return 1;
        }
    }

    /* start token analysis */
    while (yylex() != 0);
    return 0;
}