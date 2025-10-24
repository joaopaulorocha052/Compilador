#include <stdlib.h>
#include <stdio.h>
#include "lexer.h"
#include "../get_opt/options.h"


extern int yylex(void);
extern char* yytext;
extern FILE * yyin;

int flex_flag;

TokenType (*lexer)(void);

TokenType my_yylex(){
    return FIM;
}
int main(int argc, char *argv[]){

    struct Token currentToken;

    TokenType token;
    
    FILE * file;

    if(argc < 2) {
        printf("Uso do programa: %s <arquivo_de_leitura> <-l -> uso do flex>\n", argv[0]);
        return -1;
    }
    options(argc, argv);

    if(flex_flag == 1) lexer = &my_yylex;
    else lexer = &yylex;

    file = fopen(argv[1], "r");
    yyin = file;

    while((token = lexer()) != FIM){
        currentToken.type = token;
        snprintf(currentToken.lexeme, MAXTOKENLEN + 1, "%s", yytext);
        currentToken.line = lineno;
        printf("Token: %s, Lexeme: %s, Linha: %d\n", tokenToString(currentToken.type), currentToken.lexeme, currentToken.line);
        
    }

    fclose(file);

    return 0;
}