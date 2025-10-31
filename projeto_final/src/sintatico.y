%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "lexer.h"
    #include "parse.h"    
    #define YYSTYPE struct ParseTree*
    void yyerror(char* s);

    extern int yylex(void);
    extern FILE* yyin;
    extern char* yytext;
    extern int lineno;

    int ident_level = 0;

    char* temp_name_buffer;


%}
%start command
%token IF ELSE WHILE RETURN INT VOID ASSIGN EQ LTE LT GTE GT DIFF PLUS MINUS TIMES OVER LPAREN RPAREN COMMA SEMI LCOLCH RCOLCH LCHAVE RCHAVE NUM ID FIM ERROR
%nonassoc ELSE

%%

command : programa {$$ = $1;print_tree($$, 0);printf("\nBem sucedido\n");};

programa: declaracao-lista {$$ = $1;};

declaracao-lista: declaracao-lista declaracao {
                        struct ParseTree* temp = $1;
                        while(temp->sibling != NULL) temp = temp->sibling;

                        temp->sibling = $2;
                    }
                  | declaracao {$$ = $1;};

declaracao: var-declaracao { $$ = $1;};
            | fun-declaracao {$$ = $1;};

var-declaracao: tipo-especificador  ID SEMI {

                $$ = create_var_node(create_id_node(token_string), NULL);
                }
                | tipo-especificador ID LCOLCH NUM RCOLCH SEMI {
                    $$ = create_var_node(create_id_node(token_string), create_num_node(token_num));
                };

tipo-especificador: INT 
                    | VOID; 

fun-declaracao: tipo-especificador ID {temp_name_buffer = strdup(token_string);} LPAREN params RPAREN composto-decl{
                    $$ = create_func_node(create_id_node(temp_name_buffer), $5, $7);
                };

params: param-lista {$$ = $1;}
        | VOID;

param-lista: param-lista COMMA param {
                struct ParseTree* temp = $1;
                while(temp->sibling != NULL) temp = temp->sibling;

                temp->sibling = $3;
            }
            | param{$$ = $1;};

param:  tipo-especificador ID {$$ = create_var_node(create_id_node(token_string), NULL);}
        | tipo-especificador ID LCOLCH RCOLCH{$$ = create_var_node(create_id_node(token_string), create_num_node(token_num));};

composto-decl: LCHAVE local-declaracoes statement-lista RCHAVE{
        // $2->sibling = $3;
        // $$->children[0] = $2;
    };

local-declaracoes: local-declaracoes var-declaracao{
                // struct ParseTree* temp = $1;
                // while(temp->sibling != NULL) temp = temp->sibling;

                // temp->sibling = $2;

                // $$ = $1;
                }
                   | {};

statement-lista: statement-lista statement
                 | {};

statement: expressao-decl
           | composto-decl
           | selecao-decl 
           | iteracao-decl
           | retorno-decl;

expressao-decl: expressao SEMI
                | SEMI ;

selecao-decl: IF LPAREN expressao RPAREN statement
              | IF LPAREN expressao RPAREN statement ELSE statement;

iteracao-decl: WHILE LPAREN expressao RPAREN statement;

retorno-decl: RETURN SEMI
              | RETURN expressao SEMI;

expressao: var ASSIGN expressao
          | simples-expressao;

var: ID {
            printf("\nPassei Aqui\n");
            $$ = create_new_node(0, ID_NODE, yytext, NULL, NULL, NULL);
        }
     | ID LCOLCH expressao RCOLCH;

simples-expressao: soma-expressao relacional soma-expressao
                   | soma-expressao;

relacional: LTE
            | LT
            | GT
            | GTE
            | EQ
            | DIFF;

soma-expressao: soma-expressao soma termo
                | termo;

soma: PLUS
      | MINUS;
      
termo: termo mult fator
       | fator;

mult: TIMES
      | OVER;
      
fator: LPAREN expressao RPAREN
      | var
      | ativacao
      | NUM;

ativacao: ID LPAREN args RPAREN;

args: arg-lista
     | ;

arg-lista: arg-lista COMMA expressao
           | expressao;


%%


void print_tree(struct ParseTree* tree, int level){
    
    if(tree == NULL) return;

    int next_level = level;
    for(int i=0; i<level; i++) printf("\t");

    switch(tree->node_type){
        case NUM_NODE:
            printf("NUM_NODE: %d\n", tree->node_value.num_value);
            break;
        case VAR_NODE:
            printf("VAR_NODE\n");
            break;
    
        case ID_NODE:
            printf("ID_NODE: %s\n", tree->node_value.id_name);
            break;

        case OP_NODE:
            printf("OP_NODE: %c\n", tree->node_value.op_value);
            break;
        case FUNC_NODE:
            printf("FUNC_NODE\n");
            break;
        default:
            printf("Unknown Node: %d\n", tree->node_type);
            break;
    }
    
    next_level++;
    for(int i=0; i<NUM_CHILDREN; i++){
        print_tree(tree->children[i], next_level);
    }

    print_tree(tree->sibling, level);

    free_node(tree);
}

struct ParseTree* create_func_node(struct ParseTree* first_child, struct ParseTree* second_child, struct ParseTree* last_child){
    struct ParseTree* node = allocate_node(FUNC_NODE);

    node->children[0] = first_child;
    node->children[1] = second_child;
    node->children[2] = last_child;

    return node;
}

static void free_node(struct ParseTree* node){
    if(node->node_type == ID_NODE) free(node->node_value.id_name);

    free(node);
}
static struct ParseTree* allocate_node(NodeType type){
    struct ParseTree* node = (struct ParseTree*) malloc(sizeof(struct ParseTree));

    if(node == NULL){
        printf("Erro ao alocar memória para o nó %d\n", type);
        return NULL;
    }

    node->node_type = type;
    node->sibling = NULL;
    for(int i=0; i<NUM_CHILDREN; i++) node->children[i] = NULL;

    return node;

}

struct ParseTree* create_num_node(int value){
    struct ParseTree* node = allocate_node(NUM_NODE);

    node->node_value.num_value = value;

    return node;
}

struct ParseTree* create_id_node(char* id_name){
    struct ParseTree* node = allocate_node(ID_NODE);

    node->node_value.id_name = strdup(id_name);

    return node;
}

struct ParseTree* create_var_node(struct ParseTree* first_child, struct ParseTree* second_child){
    // first child - variable id | second child - vector size (NULL if integer)
    struct ParseTree* node = allocate_node(VAR_NODE);

    node->children[0] = first_child;
    node->children[1] = second_child;

    return node;
}
struct ParseTree* create_new_node(int value, NodeType type, char* name, struct ParseTree* first_child, struct ParseTree* second_child, struct ParseTree* last_child){
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
    else if(type == VAR_NODE){
        node->node_type = type;
        for(int i=1; i<NUM_CHILDREN;i++) node->children[i] = NULL;
        node->children[0] = first_child;
        node->children[1] = second_child;
        
    }
    else if(type == ID_NODE){
        node->node_type = type;
        node->node_value.id_name = strdup(name);
        for(int i=0; i<NUM_CHILDREN;i++) node->children[i] = NULL;
    }
    
    else{
        node->node_type = type;
        node->node_value.op_value = value;
        for(int i=0; i<NUM_CHILDREN;i++) node->children[i] = NULL;

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
    printf("%s (%s) linha: %d\n", s, yytext, lineno);
}