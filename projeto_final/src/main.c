#include <stdlib.h>
#include <stdio.h>
#include "code_gen.h"
#include "symbol_table.h"
#include "utils.h"
#include "../get_opt/options.h"


extern struct ParseTree* yyparse(void);
extern char* yytext;
extern FILE * yyin;

extern HashTable* table;
extern char* temp_name_buffer;
extern char* token_string;
extern int lineno;
extern struct QuadrupleList* quadList;

int flex_flag;

struct ParseTree* (*parse_function)(void);
struct ParseTree* sintTree;


int main(int argc, char *argv[]){
    
    FILE * file;
    quadList = init_list();

    if(argc < 2) {
        printf("Uso do programa: %s <arquivo_de_leitura> <-l -> uso do flex>\n", argv[0]);
        return -1;
    }

    
    parse_function = &yyparse;

    file = fopen(argv[1], "r");
    yyin = file;

    
    table = create_table();
    yyparse();


    gen_code(sintTree);
    
    print_list(quadList);

    fclose(file);
    free(temp_name_buffer);
    free(token_string);

    return 0;
}
