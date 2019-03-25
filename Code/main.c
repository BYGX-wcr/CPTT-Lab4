#include <stdio.h>

extern struct Node;
extern FILE* yyin;
extern int yylineno;
extern int yyrestart();
extern int yyparse();
extern void output(struct Node* root);
extern struct Node* syntax_tree;

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
    if(yyparse() == 0){
        if(syntax_tree != NULL){
            output(syntax_tree);
        }
    }
    return 0;
}