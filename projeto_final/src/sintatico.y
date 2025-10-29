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
%token IF ELSE WHILE RETURN INT VOID ASSIGN EQ LTE LT GTE GT DIFF PLUS MINUS TIMES OVER LPAREN RPAREN COMMA SEMI LCOLCH RCOLCH LCHAVE RCHAVE NUM ID FIM ERROR
%nonassoc ELSE

%%

command : programa {printf("Bem sucedido");};

programa: declaracao-lista;

declaracao-lista: declaracao-lista declaracao
                  | declaracao;

declaracao: var-declaracao
            | fun-declaracao;

var-declaracao: tipo-especificador ID
                | tipo-especificador ID LCOLCH NUM RCOLCH;

tipo-especificador: INT 
                    | VOID; 

fun-declaracao: tipo-especificador ID LPAREN params RPAREN composto-decl;

params: param-lista
        | VOID;

param-lista: param-lista COMMA param
            | param;

param:  tipo-especificador ID
        | tipo-especificador ID LCOLCH RCOLCH;

composto-decl: LCHAVE local-declaracoes statement-lista RCHAVE;

local-declaracoes: local-declaracoes var-declaracao
                   | ; 

statement-lista: statement-lista statement
                 | ;

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

var: ID
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