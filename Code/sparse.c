#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>

#define LEXICAL_U 1
#define GRAM_U 0

#define MAX_CHILDS 8
#define NODE_SIZE sizeof(struct Node)

#define CHECK(vertex, name) strcmp(vertex->id, name)

struct Node {
    bool type; //[Lexical Unit]:true, [Grammatical Unit]:false
    char* id; //[Lexical Unit]:token, [Grammatical Unit]:non-terminals
    int lineno; //line number
    char* info; //[Lexical Unit]:details, [Grammatical Unit]:undefined
    struct Node* childs[MAX_CHILDS]; //pointers to child nodes
};

void init();
void visit(struct Node* vertex);
void errorinfo(int type, int lineno, char* description);

void panic(char* msg) {
    fprintf(stderr, "%s\n", msg);
    assert(0);
}

void semantic_parse(struct Node* root) {

    init();
    visit(root);
}

void init() {

}

void visit(struct Node* vertex) {
    if (CHECK(vertex, "VarDec")) {
        VarDec(vertex);
    }
    else if (CHECK(vertex, "Exp")) {
        Exp(vertex);
    }
    else
        visit(vertex);
}

void errorinfo(int type, int lineno, char* description) {
    printf("Error type %d at Line %d: %s\n", type, lineno, description);
}

/* operations on tree nodes*/

struct Node* create_node(bool type, char* id, int lineno, const char* info) {
    struct Node* res = malloc(NODE_SIZE);
    memset(res, 0, NODE_SIZE);             // Nodes initialized

    res->type = type;
    res->lineno = lineno;
    
    if (strlen(id) == 0)
        panic("Invalid Id\n");
    res->id = malloc(strlen(id) + 1);
    strcpy(res->id, id);

    res->info = malloc(strlen(info) + 1);
    strcpy(res->info, info);

    return res;
}

void destroy_tree(struct Node* root) {
    free(root->id);
    free(root->info);

    int pos = 0;
    while (pos < MAX_CHILDS && root->childs[pos] != NULL) {
        destroy_tree(root->childs[pos]);
        pos++;
    }
    free(root);
}

void insert(struct Node* dest, struct Node* src) {
    //find a empty position to insert
    int pos = 0;
    while (pos < MAX_CHILDS && dest->childs[pos] != NULL) pos++;

    if (pos == MAX_CHILDS) {
        panic("childs are too many!");
    }
    else {
        dest->childs[pos] = src;
    }
}

void combine(struct Node *dest,int number,...){
    struct Node* src = NULL;
    va_list ap;
    va_start(ap,number); // start behind number
    for(int i = 0; i < number; i++){
        src = va_arg(ap,struct Node *);
        insert(dest,src);
    }
    va_end(ap);
}

void display(struct Node* root,int space) {
    if(root->type == LEXICAL_U) {                // lexical
        for(int k = 0;k < space;k++) {          // printf space
            printf(" ");
        }
        if(strcmp(root->id,"ID") == 0) {          // ID
            printf("%s: %s\n",root->id,root->info);
        }      
        else if(strcmp(root->id,"TYPE") == 0) {   // TYPE
            printf("%s: %s\n",root->id,root->info);
        }
        else if(strcmp(root->id,"INT") == 0) {   // INT
            printf("%s: %d\n",root->id,atoi(root->info));
        }
        else if(strcmp(root->id,"FLOAT") == 0) {   // FLOAT
            printf("%s: %f\n",root->id,atof(root->info));
        }
        else {
            printf("%s\n",root->id);
        }
    }
    else {                                // programmer 
        if(root->childs[0] != NULL) {     // not empty 
            for(int k = 0;k < space;k++) {  // printf space
                printf(" ");
            }
            printf("%s (%d)\n",root->id,root->lineno); 
            space += 2;                  // space add 2
            for(int i = 0;(i < MAX_CHILDS) && (root->childs[i] != NULL);i++) {
                display(root->childs[i],space);
            }
        }
    }
}

void output(struct Node* root) {
    if(root == NULL){
        panic("this tree is empty !\n");
        return;
    }
    int space = 0;
    display(root, space);
}