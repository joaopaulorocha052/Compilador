%{
#define MAXTOKENLEN 100 
#include <stdio.h>
#include <stdlib.h>
char tokenString[MAXTOKENLEN+1];
typedef enum { IF , ELSE, WHILE, RETURN, INT, VOID, ASSIGN, EQ, LTE, LT, GTE, GT, DIFF, PLUS, MINUS, TIMES, OVER, LPAREN, RPAREN, COLON, SEMI, LCOLCH, RCOLCH, LCHAVE, RCHAVE, NUM, ID, FIM} TokenType;

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
"while"        {return WHILE;}    
"return"         {return RETURN;}
"int"          {return INT;}
"void"         {return VOID;}
"="            {return ASSIGN;}
"=="             {return EQ;}
"<="             {return LTE;}
"<"             {return LT;}
">="             {return GTE;}
">"             {return GT;}
"!="             {return DIFF;}
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
{newline}       {lineno++;}
{whitespace}    {/* skip whitespace */}
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

int main(int argc, char *argv[])
{
  TokenType currentToken;

  FILE * f_in;
  FILE * f_out;

   if (argc == 2)
  {
    if(f_in = fopen(argv[1],"r"))  yyin = f_in;
    else  perror(argv[0]);
  }
  else  yyin = stdin;

  f_out = fopen("tokens.txt", "w");



  while ((currentToken = yylex()) != FIM || currentToken != ERROR){
    strncpy(tokenString,yytext,MAXTOKENLEN);
    printToken(currentToken,tokenString);

  }  
  return currentToken;
}

