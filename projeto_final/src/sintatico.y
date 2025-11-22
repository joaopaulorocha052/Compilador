%{
    /**********
        João Paulo Paixão Rocha - 156408
        Maria Clara Couto Lorena - 163941
    ************/  
    #include <stdio.h>
    #include <string.h>
    #include "lexer.h"
    #include "parse.h"
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

command : programa {$$ = $1;print_tree_prefix($$, 0, "");printf("\nBem sucedido\n");};

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

fun-declaracao: tipo-especificador ID {$2 = create_id_node(token_string);} LPAREN params RPAREN composto-decl{
                    $$ = create_func_node($2, $5, $7);
                };

params: param-lista {$$ = create_param_node($1);}
        | VOID;

param-lista: param-lista COMMA param {
                struct ParseTree* temp = $1;
                while(temp->sibling != NULL) temp = temp->sibling;

                temp->sibling = $3;
            }
            | param{$$ = $1;};

param:  tipo-especificador ID {$$ = create_var_node(create_id_node(token_string), NULL);}
        | tipo-especificador ID LCOLCH RCOLCH{$$ = create_var_node(create_id_node(token_string), NULL);}; // OLHAR O VETOR DEPOIS

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
                | /* VAZIO */ {$$ = NULL;};

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
           | retorno-decl  {$$ = $1;};

expressao-decl: expressao SEMI {$$ = $1;}
                | SEMI ;

selecao-decl: IF LPAREN expressao RPAREN statement {
                    $$ =create_if_node($3, $5, NULL);
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
            }
          | simples-expressao { $$ = $1; };

var: ID {
            $$ = create_var_node(create_id_node(token_string), NULL);
    }
     | ID {temp_name_buffer = strdup(token_string);} LCOLCH expressao RCOLCH {
        $$ = create_var_node(create_id_node(temp_name_buffer), $4);
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
      | NUM { $$ = create_num_node(token_num); }; 

ativacao: ID {$1 = create_id_node(token_string);} LPAREN args RPAREN {
            $$ = create_func_ativacao($1, $4);
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

void yyerror(char* s){
    extern char* yytext;
    printf("%s (%s) linha: %d\n", s, yytext, lineno);
}