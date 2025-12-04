#ifndef LEXER_H
#define LEXER_H

#define MAXTOKENLEN 100 

// #include "sintatico.tab.h"

// typedef yytoken_kind_t TokenType;
// extern char tokenString[MAXTOKENLEN+1];


// typedef enum { IF = 258 , ELSE, WHILE, RETURN, INT, VOID, ASSIGN, EQ, LTE, LT, GTE, GT, DIFF, PLUS, MINUS, \
//     TIMES, OVER, LPAREN, RPAREN, COLON, SEMI, LCOLCH, RCOLCH, LCHAVE, RCHAVE, NUM, ID, FIM, ERROR} TokenType;

typedef int TokenType;
    
struct Token {
    TokenType type;
    int line;
    char lexeme[MAXTOKENLEN + 1];
};


extern int lineno;
extern char* token_string;
extern int token_num;

// Function to print the token name
const char* tokenToString(TokenType token);

#endif