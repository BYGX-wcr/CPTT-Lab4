#include <stdio.h>

extern struct Node;
extern FILE* yyin;
extern int yylineno;
extern struct Node* syntax_tree;
extern int error_flag;
//extern int yydebug;

extern int yyrestart();
extern int yyparse();
extern void output(struct Node* root);

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
    yyrestart(yyin);
    //yydebug = 0;
    if(yyparse() == 0 && !error_flag){
        output(syntax_tree);
    }
    return 0;
}