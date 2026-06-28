#include <stdlib.h>
#include <stdio.h>
#include "code_gen.h"
#include "symbol_table.h"
#include "utils.h"
#include "asm_gen.h"
#include "bin_gen.h"
#include "../get_opt/options.h"


extern struct ParseTree* yyparse(void);
extern char* yytext;
extern FILE * yyin;

extern HashTable* table;
extern char* temp_name_buffer;
extern char* token_string;
extern int lineno;
extern struct QuadrupleList* quadList;


extern AsmOperation operation_list[];
extern int current_list_position;
extern int label_position[];
extern int call_target[];

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

    asm_gen(quadList);

    FILE* bin_clean = fopen("programa.bin.txt", "w");
    if (bin_clean != NULL) {
        bin_gen_write_clean(operation_list, current_list_position, label_position, call_target, bin_clean);
        fclose(bin_clean);
    } else {
        printf("ERRO: nao foi possivel criar programa.bin.txt\n");
    }

    FILE* bin_commented = fopen("programa_comentado.bin.txt", "w");
    if (bin_commented != NULL) {
        bin_gen_write_commented(operation_list, current_list_position, label_position, call_target, bin_commented);
        fclose(bin_commented);
    } else {
        printf("ERRO: nao foi possivel criar programa_comentado.bin.txt\n");
    }

    fclose(file);
    free(temp_name_buffer);
    free(token_string);

    return 0;
}