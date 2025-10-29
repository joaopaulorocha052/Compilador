%option noyywrap
%{
#include "lexer.h"
#include "sintatico.tab.h"
extern YYSTYPE yylval;
FILE * f_out;
int lineno = 0;
%}

digit       [0-9]
number      {digit}+
letter      [a-zA-Z]
identifier  {letter}({letter}|{digit})*
newline     \n
whitespace  [ \t]+

%%

"if"            {return IF;}
"else"          {return ELSE;}
"while"         {return WHILE;}    
"return"        {return RETURN;}
"int"           {return INT;}
"void"          {return VOID;}
"="             {return ASSIGN;}
"=="            {return EQ;}
"<="            {return LTE;}
"<"             {return LT;}
">="            {return GTE;}
">"             {return GT;}
"!="            {return DIFF;}
"+"             {return PLUS;}
"-"             {return MINUS;}
"*"             {return TIMES;}
"/"             {return OVER;}
"("             {return LPAREN;}
")"             {return RPAREN;}
","             {return COMMA;}
";"             {return SEMI;}
"["             {return LCOLCH;}
"]"             {return RCOLCH;}
"{"             {return LCHAVE;}
"}"             {return RCHAVE;}
{number}        {yylval = atoi(yytext);return NUM;}
{identifier}    {return ID;}
{newline}       {lineno++;}
{whitespace}    {
                  /* Ignorar whitespace */
                }
"/*"             { char c;
                  do
                  { c = input();
                    if (c == '\n') lineno++;
                    if( c == '*') {
                        c = input();
                        if ( c == '/') break;
                    }

                  } while (c != EOF);
                  return FIM;
                }
.               {return ERROR;}

%%

const char* tokenToString(TokenType token) {
    switch(token) {
        case IF:      return "IF";
        case ELSE:    return "ELSE";
        case WHILE:   return "WHILE";
        case RETURN:  return "RETURN";
        case INT:     return "INT";
        case VOID:    return "VOID";
        case ASSIGN:  return "ASSIGN";
        case EQ:      return "EQ";
        case LTE:     return "LTE";
        case LT:      return "LT";
        case GTE:     return "GTE";
        case GT:      return "GT";
        case DIFF:    return "DIFF";
        case PLUS:    return "PLUS";
        case MINUS:   return "MINUS";
        case TIMES:   return "TIMES";
        case OVER:    return "OVER";
        case LPAREN:  return "LPAREN";
        case RPAREN:  return "RPAREN";
        case COMMA:   return "COMMA";
        case SEMI:    return "SEMI";
        case LCOLCH:  return "LCOLCH";
        case RCOLCH:  return "RCOLCH";
        case LCHAVE:  return "LCHAVE";
        case RCHAVE:  return "RCHAVE";
        case NUM:     return "NUM";
        case ID:      return "ID";
        case FIM:     return "FIM";
        case ERROR:   return "ERROR";
        default:      return "TOKEN DESCONHECIDO";
    }
}

