
%{
    /**********
        João Paulo Paixão Rocha - 156408
        Maria Clara Couto Lorena - 163941
    ************/
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "lexer.h"
    #include "parse.h"   
    #define YYSTYPE struct ParseTree*
    #define SCOPE_NAME_SIZE 100
    #include "symbol_table.h"

    int error_num = 0;
    int last_syntax_error_line = -1;

    void yyerror(char* s);
    extern struct ParseTree* sintTree;
    extern int yylex(void);
    extern FILE* yyin;
    extern char* yytext;
    extern int lineno;

    int ident_level = 0;

    char scope[SCOPE_NAME_SIZE] = "global";
    char* temp_name_buffer;
    char* erro;

    HashTable* table;
%}
%start command
%token IF ELSE WHILE RETURN INT VOID ASSIGN EQ LTE LT GTE GT DIFF PLUS MINUS TIMES OVER LPAREN RPAREN COMMA SEMI LCOLCH RCOLCH LCHAVE RCHAVE NUM ID FIM ERROR
%nonassoc ELSE

%%

command : programa {$$ = $1;
                    if(search_item(table, "main", "global") == NULL){
                        printf("ERRO SEMÂNTICO: main não declarada\n");
                        error_num++;
                    }
                    if(error_num == 0){
		        sintTree = $$;
                        print_tree(sintTree, 0);
                        print_table(table);
                        printf("\nBem sucedido\n");
                    }
                    };

programa: declaracao-lista {$$ = $1;};

declaracao-lista: declaracao-lista declaracao {
                        struct ParseTree* temp = $1;
			if(temp != NULL){
			  while(temp->sibling != NULL) temp = temp->sibling;

			  temp->sibling = $2;
			}
			
                        

                        
                    }
                  | declaracao {$$ = $1;}
                  
                  | error { char* erro = strdup(yytext); } token_qualquer SEMI{
                    // MESMA LOGICA AQUI
                    if (lineno > last_syntax_error_line) {
                        printf("ERRO SINTÁTICO: token %s. LINHA: %d\n", erro, lineno);
                        last_syntax_error_line = lineno;
                        error_num++;
                    }
                    yyclearin; // Descarta token
                    yyerrok;   // Continua parse
                    $$ = NULL;
                }

                  ;

declaracao: var-declaracao { $$ = $1;};
            | fun-declaracao {$$ = $1;};

var-declaracao: tipo-especificador  ID SEMI {
                    $$ = create_var_node(create_id_node(token_string), NULL);
                    if($1->node_value.op_value == VOID){
                        insert_item(table, $$->children[0]->node_value.id_name, scope, $$->children[0]->line_num, VAR, VOID_EXP, 0);
                    } else {
                        insert_item(table, $$->children[0]->node_value.id_name, scope, $$->children[0]->line_num, VAR, INT_EXP, 0);
                    }
                }
                | tipo-especificador ID LCOLCH NUM RCOLCH SEMI {
                    $$ = create_var_node(create_id_node(token_string), create_num_node(token_num));
                    if($1->node_value.op_value == VOID){
                        insert_item(table, $$->children[0]->node_value.id_name, scope, $$->children[0]->line_num, VAR, VOID_EXP, 0);
                    } else {
                        insert_item(table, $$->children[0]->node_value.id_name, scope, $$->children[0]->line_num, VAR, INT_EXP, 0);
                    }
                }
                
                | ERROR { error_num++;$$ = create_error_node();}
                /* | error {temp_name_buffer = strdup(token_string);} token_qualquer {
                    printf("ERRO SINTÁTICO: token %s. LINHA: %d\n", temp_name_buffer, lineno);
                    error_num++; 
                    yyerrok; 
                    yyclearin;
                    $$ = create_error_node();
                }; */
                |error { erro = strdup(yytext); } token_qualquer SEMI{
                    // MESMA LOGICA AQUI
                    if (lineno > last_syntax_error_line) {
                        printf("ERRO SINTÁTICO: token %s. LINHA: %d\n", erro, lineno);
                        last_syntax_error_line = lineno;
                        error_num++;
                    }
                    yyclearin; // Descarta token
                    yyerrok;   // Continua parse
                    $$ = create_error_node();
                }

token_qualquer: RCHAVE token_qualquer| RCOLCH token_qualquer| RPAREN token_qualquer | COMMA token_qualquer | ID token_qualquer | PLUS token_qualquer | MINUS token_qualquer | LT token_qualquer | LTE token_qualquer | GT token_qualquer | GTE token_qualquer | IF token_qualquer | ELSE token_qualquer | WHILE token_qualquer | RETURN token_qualquer | INT token_qualquer | VOID token_qualquer | ASSIGN token_qualquer | EQ token_qualquer | DIFF token_qualquer | TIMES token_qualquer | OVER token_qualquer | NUM token_qualquer | LCHAVE token_qualquer | LCOLCH token_qualquer | LPAREN token_qualquer| ;
tipo-especificador: INT {$$ = create_op_terminal(INT);}
                    | VOID {$$ = create_op_terminal(VOID);}; 

fun-declaracao: tipo-especificador ID {
                    $2 = create_id_node(token_string);
                    strcpy(scope, $2->node_value.id_name);
                } LPAREN params RPAREN {
                    int qnt = 0;
                    if($5 != NULL){
                        qnt = procura_params($5);
                    }
                    if($1->node_value.op_value == VOID){
                        insert_item(table, $2->node_value.id_name, "global", $2->line_num, FUNC, VOID_EXP, qnt);
                    } else {
                        insert_item(table, $2->node_value.id_name, "global", $2->line_num, FUNC, INT_EXP, qnt);
                    }
                } composto-decl{ //AQUI
                    $$ = create_func_node($2, $5, $8);
                    strcpy(scope,"global");
                };

params: param-lista {$$ = create_param_node($1);}
        | VOID;

param-lista: param-lista COMMA param {
                struct ParseTree* temp = $1;
                while(temp->sibling != NULL) temp = temp->sibling;

                temp->sibling = $3;
            }
            | param{$$ = $1;};

param:  tipo-especificador ID {$$ = create_var_node(create_id_node(token_string), NULL);
            if($1->node_value.op_value == VOID){
                insert_item(table, $$->children[0]->node_value.id_name, scope, $$->children[0]->line_num, VAR, VOID_EXP, 0);
            } else {
                insert_item(table, $$->children[0]->node_value.id_name, scope, $$->children[0]->line_num, VAR, INT_EXP, 0);
            }
        }
        | tipo-especificador ID LCOLCH RCOLCH{$$ = create_var_node(create_id_node(token_string), NULL);
            if($1->node_value.op_value == VOID){
                insert_item(table, $$->children[0]->node_value.id_name, scope, $$->children[0]->line_num, VAR, VOID_EXP, 0);
            } else {
                insert_item(table, $$->children[0]->node_value.id_name, scope, $$->children[0]->line_num, VAR, INT_EXP, 0);
            }
        };

composto-decl: LCHAVE local-declaracoes statement-lista RCHAVE{

        $$ = create_decl_list_node($2, $3);
    };

local-declaracoes: local-declaracoes var-declaracao{
                    struct ParseTree* temp = $1;
                    if(temp == NULL){
                        $$ = $2;
                    }
                    else{ 
                        while(temp->sibling != NULL) temp = temp->sibling;
                        temp->sibling = $2;
                        $$ = $1;
                    }
                }
                | /* VAZIO */ {$$ = NULL;}
                
                ;

statement-lista: statement-lista statement {
                    struct ParseTree* temp = $1;
                    if(temp == NULL){
                        $$ = $2;
                    }
                    else{ 
                        while(temp->sibling != NULL) temp = temp->sibling;
                        temp->sibling = $2;
                        $$ = $1;
                    }
                }
                 | /* VAZIO */  {$$ = NULL;};

statement: expressao-decl  {$$ = $1;}
           | composto-decl {$$ = $1;}
           | selecao-decl  {$$ = $1;}
           | iteracao-decl {$$ = $1;}
           | retorno-decl  {$$ = $1;}
           | ERROR { error_num++;$$ = create_error_node();}
           /* | error {temp_name_buffer = strdup(token_string);} token_qualquer {
                    printf("ERRO SINTÁTICO: token %s. LINHA: %d\n", temp_name_buffer, lineno);
                    error_num++; 
                    yyerrok; 
                    yyclearin;
                    $$ = create_error_node();
                }; */
                |error { char* erro = strdup(yytext); } token_qualquer SEMI{
                    // MESMA LOGICA AQUI
                    if (lineno > last_syntax_error_line) {
                        printf("ERRO SINTÁTICO: token %s. LINHA: %d\n", erro, lineno);
                        last_syntax_error_line = lineno;
                        error_num++;
                    }
                    yyclearin; // Descarta token
                    yyerrok;   // Continua parse
                    $$ = NULL;
                }

expressao-decl: expressao SEMI {$$ = $1;}
                | SEMI ;

selecao-decl: IF LPAREN expressao RPAREN statement {
                    $$ = create_if_node($3, $5, NULL);
                }
              | IF LPAREN expressao RPAREN statement ELSE statement {
                    $$ = create_if_node($3, $5, $7);
                };

iteracao-decl: WHILE LPAREN expressao RPAREN statement{
                    $$ = create_while_node($3, $5);
                };

retorno-decl: RETURN SEMI { $$ = create_return_node(NULL); }
              | RETURN expressao SEMI { $$ = create_return_node($2);};

expressao: var ASSIGN expressao {
                $$ = create_assign_node($1, $3);
                procura_func_act($3,0);
            }
          | simples-expressao { $$ = $1; };

var: ID {
        $$ = create_var_node(create_id_node(token_string), NULL);
        HashItem* item = search_item(table,  $$->children[0]->node_value.id_name, scope);
        if(item == NULL){
            item = search_item(table,  $$->children[0]->node_value.id_name, "global");
            if(item == NULL){
                printf("ERRO SEMÂNTICO: Variável Não Declarada: %s - Linha: %d\n", $$->children[0]->node_value.id_name, $$->children[0]->line_num );
                error_num++;
            } else {
                insert_line(item, $$->children[0]->line_num);
            }
        }
        else {
            insert_line(item, $$->children[0]->line_num);
        }
    }
     | ID {temp_name_buffer = strdup(token_string);} LCOLCH expressao RCOLCH {
        $$ = create_var_node(create_id_node(temp_name_buffer), $4);
        HashItem* item = search_item(table,  $$->children[0]->node_value.id_name, scope);
        if(item == NULL){
            item = search_item(table,  $$->children[0]->node_value.id_name, "global");
            if(item == NULL){
                printf("\nVariável Não Declarada: %s - Linha: %d\n\n", $$->children[0]->node_value.id_name, $$->children[0]->line_num );
            } else {
                insert_line(item, $$->children[0]->line_num);
            }
        }
        else {
            insert_line(item, $$->children[0]->line_num);
        }
     };

simples-expressao: soma-expressao relacional soma-expressao {
                        $$ = create_op_node($1, $2, $3);
                    }
                   | soma-expressao { $$ = $1; };

relacional:   LTE  {$$ = create_op_terminal(LTE);}
            | LT   {$$ = create_op_terminal(LT);}
            | GT   {$$ = create_op_terminal(GT);}
            | GTE  {$$ = create_op_terminal(GTE);}
            | EQ   {$$ = create_op_terminal(EQ);}
            | DIFF {$$ = create_op_terminal(DIFF);};

soma-expressao: soma-expressao soma termo {
                    $$ = create_op_node($1, $2, $3);
                }
                | termo { $$ = $1; };

soma: PLUS {$$ = create_op_terminal(PLUS);}
      | MINUS {$$ = create_op_terminal(MINUS);};
      
termo: termo mult fator {
            $$ = create_op_node($1, $2, $3);
        }
       | fator { $$ = $1; };

mult: TIMES {$$ = create_op_terminal(TIMES);}
      | OVER {$$ = create_op_terminal(OVER);};
      
fator: LPAREN expressao RPAREN { $$ = $2; }
      | var { $$ = $1; }
      | ativacao { $$ = $1; }
      | NUM { $$ = create_num_node(token_num);}
      | ERROR { $$ = create_error_node(); error_num++;}
      /* | error {temp_name_buffer = strdup(token_string);} token_qualquer {
                    printf("ERRO SINTÁTICO: token %s. LINHA: %d\n", temp_name_buffer, lineno);
                    error_num++; 
                    yyerrok; 
                    yyclearin;
                    $$ = create_error_node();
                }; */
                |error {
                    // MESMA LOGICA AQUI
                    if (lineno > last_syntax_error_line) {
                        printf("ERRO SINTÁTICO: token %s. LINHA: %d\n", yytext, lineno);
                        last_syntax_error_line = lineno;
                        error_num++;
                    }
                    yyclearin; // Descarta token
                    yyerrok;   // Continua parse
                    $$ = NULL;
                }


ativacao: ID {
            $1 = create_id_node(token_string);
            HashItem* item = search_item(table, $1->node_value.id_name, "global");
            if(item == NULL){
                printf("ERRO SEMÂNTICO: Função não declarada %s - Linha: %d\n", $1->node_value.id_name, $1->line_num );
                error_num++;
            } else {
                insert_line(item, $1->line_num);
            }
            } LPAREN args RPAREN {
            procura_func_act($4,0);
            $$ = create_func_ativacao($1, $4);
            fun_conf_param($$); 
        };

args: arg-lista { $$ = create_args_node($1); }
     | {$$ = NULL;};

arg-lista: arg-lista COMMA expressao {
                struct ParseTree* temp = $1;
                while(temp->sibling != NULL) temp = temp->sibling;

                temp->sibling = $3;
            }
           | expressao { $$ = $1; };


%%

//RETURN_NODE, IF_NODE, WHILE_NODE
void print_tree(struct ParseTree* tree, int level){
    
    if(tree == NULL) return;

    int next_level = level;
    for(int i=0; i<level; i++) printf("\t");

    char str[3];
    switch(tree->node_type){
        case OP_TERMINAL_NODE:
            // trocar depois por printToken
            switch(tree->node_value.op_value){
                case PLUS:
                    strcpy(str, "+");
                    break;
                case MINUS:
                    strcpy(str, "-");
                    break;
                case TIMES:
                    strcpy(str, "*");
                    break;
                case OVER:
                    strcpy(str, "/");
                    break;
                case LTE :
                    strcpy(str, "<=");
                    break;
                case LT  :
                    strcpy(str, "<");
                    break;
                case GT  :
                    strcpy(str, ">");
                    break;
                case GTE :
                    strcpy(str, ">=");
                    break;
                case EQ  :
                    strcpy(str, "==");
                    break;
                case DIFF:
                    strcpy(str, "!=");
                    break;
                default:
                    printf("Operação não reconhecida!!!");
                    break;
            }
            // trocar depois por printToken
            printf("OP_TERMINAL_NODE: %s\n", str);
            break;
        case OP_NODE:
            printf("OP_NODE\n");
            break;
        case NUM_NODE:
            printf("NUM_NODE: %d\n", tree->node_value.num_value);
            break;
        case VAR_NODE:
            printf("VAR_NODE\n");
            break;
        case ARGS_NODE:
            printf("ARGS_NODE\n");
            break;
        case ID_NODE:
            printf("ID_NODE: %s\n", tree->node_value.id_name);
            break;
        case FUNC_PARAM_NODE:
            printf("FUNC_PARAM_NODE\n");
            break;
        case FUNC_NODE:
            printf("FUNC_NODE\n");
            break;
        case DECL_LIST_NODE:
            printf("DECL_LIST_NODE\n");
            break;
        case ASSIGN_NODE:
            printf("ASSIGN_NODE:\n");
            break;
        case FUNC_ACTV_NODE:
            printf("FUNC_ACTV_NODE:\n");
            break;
        case RETURN_NODE:
            printf("RETURN_NODE:\n");
            break;
        case IF_NODE:
            printf("IF_NODE:\n");
            break;
        case WHILE_NODE:
            printf("WHILE_NODE:\n");
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

}

void procura_func_act(struct ParseTree* tree, int level){
    
    if(tree == NULL) return;

    int next_level = level;


    switch(tree->node_type){
        case FUNC_ACTV_NODE:
            HashItem* node_func = search_item(table, tree->children[0]->node_value.id_name, "global");
            if(node_func->type == 1){ //VOID_EXP
                printf("ERRO SEMÂNTICO: Parâmetro VOID %s - Linha: %d\n", node_func->value, lineno);
                error_num++;
                return; 
            }
            break;
        default:
            break;
    }
    
    next_level++;
    for(int i=0; i<NUM_CHILDREN; i++){
        procura_func_act(tree->children[i], next_level);
    }

    procura_func_act(tree->sibling, level);
}

void fun_conf_param(struct ParseTree* tree){
    if (tree == NULL) return;
    HashItem* item = search_item(table, tree->children[0]->node_value.id_name, "global");
    struct ParseTree* temp = tree->children[1];
    if(temp == NULL) {
        if(item->qnt_param == 0) return;
        printf("ERRO SEMÂNTICO: Menos Parâmetros %s - Linha: %d\n", item->value, lineno);
        error_num++;
    }
    temp = temp->children[0];
    int qnt = 0;
    while(temp != NULL){
        qnt++;
        temp = temp->sibling;
    }
    if(qnt != item->qnt_param) {
        printf("ERRO SEMÂNTICO Numero Errado de Parâmetros: %s - Linha: %d\n", item->value, lineno);
        error_num++;
    }
    return;
}

int procura_params(struct ParseTree* tree){
    if (tree == NULL) return 0;
    struct ParseTree* temp = tree->children[0];
    int qnt = 0;
    while(temp != NULL){
        HashItem* item = search_item(table, temp->children[0]->node_value.id_name, scope);
        if(item->type == 0){
            qnt++;
        }
        else{
            printf("ERRO SEMÂNTICO: Parametro void na declaração %s - Linha: %d\n", item->value, lineno);
            error_num++;
        }

        temp = temp->sibling;
    }

    return qnt;

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

struct ParseTree* create_op_terminal(TokenType op) {
    struct ParseTree* node = allocate_node(OP_TERMINAL_NODE);
    if(node == NULL){
        printf("Erro ao alocar memória para o nó %d\n", op);
        return NULL;
    }
    node->node_value.op_value = op;

    return node;
}

struct ParseTree* create_error_node() {
    struct ParseTree* node = allocate_node(ERROR_NODE);
    return node;
}

struct ParseTree* create_func_node(struct ParseTree* first_child, struct ParseTree* second_child, struct ParseTree* last_child){
    struct ParseTree* node = allocate_node(FUNC_NODE);

    node->children[0] = first_child;
    node->children[1] = second_child;
    node->children[2] = last_child;

    return node;
}

struct ParseTree* create_func_ativacao(struct ParseTree* first_child, struct ParseTree* second_child){
    struct ParseTree* node = allocate_node(FUNC_ACTV_NODE);

    node->children[0] = first_child;
    node->children[1] = second_child;

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
    node->line_num = lineno;

    return node;
}

struct ParseTree* create_var_node(struct ParseTree* first_child, struct ParseTree* second_child){
    // first child - variable id | second child - vector size (NULL if integer)
    struct ParseTree* node = allocate_node(VAR_NODE);

    node->children[0] = first_child;
    node->children[1] = second_child;

    return node;
}

struct ParseTree* create_param_node(struct ParseTree* first_child){
    // first child - variable id | second child - vector size (NULL if integer)
    struct ParseTree* node = allocate_node(FUNC_PARAM_NODE);

    node->children[0] = first_child;

    return node;
}

struct ParseTree* create_args_node(struct ParseTree* first_child){
    // first child - variable id | second child - vector size (NULL if integer)
    struct ParseTree* node = allocate_node(ARGS_NODE);

    node->children[0] = first_child;

    return node;
}

struct ParseTree* create_decl_list_node(struct ParseTree* first_child, struct ParseTree* second_child){
    struct ParseTree* node = allocate_node(DECL_LIST_NODE);

    node->children[0] = first_child;
    node->children[1] = second_child;

    return node;
}
struct ParseTree* create_op_node(struct ParseTree* first_child, struct ParseTree* second_child, struct ParseTree* last_child){
    // first child - op | second child 
    struct ParseTree* node = allocate_node(OP_NODE);
    

    node->children[0] = first_child;
    node->children[1] = second_child;
    node->children[2] = last_child;

    return node;
}

struct ParseTree* create_return_node(struct ParseTree* first_child){
    // first child - expressao
    struct ParseTree* node = allocate_node(RETURN_NODE);

    node->children[0] = first_child;

    return node;
}

struct ParseTree* create_if_node(struct ParseTree* first_child, struct ParseTree* second_child, struct ParseTree* last_child){
    struct ParseTree* node = allocate_node(IF_NODE);

    node->children[0] = first_child;
    node->children[1] = second_child;
    node->children[2] = last_child;

    return node;
}

struct ParseTree* create_while_node(struct ParseTree* first_child, struct ParseTree* second_child){
    // first child - expressao | second child - statement
    struct ParseTree* node = allocate_node(WHILE_NODE);

    node->children[0] = first_child;
    node->children[1] = second_child;

    return node;
}

struct ParseTree* create_assign_node(struct ParseTree* first_child, struct ParseTree* second_child){
    // first child - variable id | second child - expressão
    struct ParseTree* node = allocate_node(ASSIGN_NODE);


    node->children[0] = first_child;
    node->children[1] = second_child;

    return node;
}




/* int main(int argc, char* argv[]){
    if(argc < 2){
        printf("ERRO\n");
        return -1;
    }
    FILE *file = fopen(argv[1], "r");
    yyin = file;

    table = create_table();

    yyparse();

    fclose(file);
    free(temp_name_buffer);
    free(token_string);
    return 0;
} */

void yyerror(char* s){
    /* extern char* yytext;
    error_num++;
    printf("%c\n", yychar);
    printf("%s (%s) linha: %d\n", s, yytext, lineno); */
    return;
}
