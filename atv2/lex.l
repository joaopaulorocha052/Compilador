%{
#define MAXTOKENLEN 100 
#include <stdio.h>
#include <stdlib.h>
char tokenString[MAXTOKENLEN+1];
typedef enum { IF , ELSE, WHILE, RETURN, INT, VOID, ASSIGN, EQ, LTE, LT, GTE, GT, DIFF, PLUS, MINUS, TIMES, OVER, LPAREN, RPAREN, COLON, SEMI, LCOLCH, RCOLCH, LCHAVE, RCHAVE, NUM, ID, FIM, ERROR} TokenType;
int lineno = 0;
FILE * f_out;
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
","             {return COLON;}
";"             {return SEMI;}
"["             {return LCOLCH;}
"]"             {return RCOLCH;}
"{"             {return LCHAVE;}
"}"             {return RCHAVE;}
{number}        {return NUM;}
{identifier}    {return ID;}
{newline}       {lineno++; fprintf(f_out, "%s ", yytext);}
{whitespace}    {
                  /* Print para montar o arquivo, pode tirar depois */
                  fprintf(f_out, "%s ", yytext);
                }
<<EOF>>          return FIM;
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

const char* printToken(TokenType token) {
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
        case COLON:   return "COLON";
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

int main(int argc, char *argv[])
{
  TokenType currentToken;

  FILE * f_in;


   if (argc == 2)
  {
    if(f_in = fopen(argv[1],"r"))  yyin = f_in;
    else  perror(argv[0]);
  }
  else  yyin = stdin;

  f_out = fopen("saida.txt", "w");
  



  while ((currentToken = yylex()) != FIM && currentToken != ERROR){
    strncpy(tokenString,yytext,MAXTOKENLEN);
    fprintf(f_out, "%s ", printToken(currentToken));

  }  

  fclose(f_out);
  return currentToken;
}

