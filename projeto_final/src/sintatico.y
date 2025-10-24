%{
    #include <stdio.h>
    #include <stdlib.h>
    #include "lexer.h"
    #include "parse.h"    

    #define YYSTYPE struct ParseTree*
    void yyerror(char* s);

    extern int yylex(void);
    extern FILE* yyin;
    extern char* yytext;
    int ident_level = 0;
%}
%start command
%token IF ELSE WHILE RETURN INT VOID ASSIGN EQ LTE LT GTE GT DIFF PLUS MINUS TIMES OVER LPAREN RPAREN COLON SEMI LCOLCH RCOLCH LCHAVE RCHAVE NUM ID FIM ERROR

%%

command : fator {print_tree($1);} ;

fator : termo PLUS fator {
                $$ = create_new_node('+', OP_NODE);
                $$->children[0] = $1;
                $$->children[1] = $3;
        }
        | termo MINUS fator {
                $$ = create_new_node('-', OP_NODE);
                $$->children[0] = $1;
                $$->children[1] = $3;
        } ;
        | termo {$$ = $1;};
termo : NUM {

                $$ = create_new_node(atoi(yytext), NUM_NODE);


            } ;

%%

void print_tree(struct ParseTree* tree){
    /* while(tree != NULL){ */
        ident_level = ident_level + 1;
        if(tree == NULL) return;
        for(int i=0; i<ident_level; i++) printf(" ");
        if(tree->node_type == NUM_NODE){
            printf("%d", tree->node_value.num_value);
        }
        else{
            printf("%c", tree->node_value.op_value);
            printf("\n");
        }
    
    for(int i=0; i<2; i++){
        print_tree(tree->children[i]);
    }
    ident_level--;
    /* } */
}

struct ParseTree* create_new_node(int value, NodeType type){
    struct ParseTree* node = (struct ParseTree*) malloc(sizeof(struct ParseTree));

    if(node == NULL){
        printf("ERRO\n");
        return NULL;
    }

    if(type == NUM_NODE){
        node->node_type = type;
        node->node_value.num_value = value;
        for(int i=0; i<2;i++) node->children[i] = NULL;

    }
    else{
        node->node_type = type;
        node->node_value.op_value = value;
        for(int i=0; i<2;i++) node->children[i] = NULL;

    }
    return node;
}

int main(int argc, char* argv[]){

    if(argc < 2){
        printf("ERRO\n");
        return -1;
    }
    FILE *file = fopen(argv[1], "r");
    yyin = file;

    return yyparse();
}

void yyerror(char* s){
    extern char* yytext;
    printf("%s (%s)\n", s, yytext);
}