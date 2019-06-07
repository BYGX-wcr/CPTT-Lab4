#include <stdio.h>
#include "sparse.h"
#include "assemble.h"
#include "ircode.h"

extern FILE* yyin;
extern int yylineno;
extern struct Node* syntax_tree;

extern int yyrestart();
extern int yyparse();
extern void translate_semantic(struct Node *root);

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
    yyparse();
    semantic_parse(syntax_tree);
    translate_semantic(syntax_tree);
    if (argc > 2)
        assemble(argv[2]);
    return 0;
}