#ifndef OPTIONS_H
#define OPTIONS_H

extern int flex_flag;
extern int debug_flag;
extern int lex_stop_flag;

int check_dash_option(char str[]);

void resolve_option(char option, char* value );

void options(int argc, char *argv[]);
#endif //OPTIONS_H