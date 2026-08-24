#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
extern int error_num;

int flex_flag;

struct ParseTree* (*parse_function)(void);
struct ParseTree* sintTree;

typedef struct opt
{
    int asm_print;
    int ic_print;
    int table_print;
    int tree_print;
} Options;


int main(int argc, char *argv[]){
    Options options;
    
    FILE * file;
    quadList = init_list();

    if(argc < 2) {
        printf("Uso do programa: %s <arquivo_de_leitura> <-l -> uso do flex>\n", argv[0]);
        return -1;
    }

    for(int i=0; i<argc; i++){
        if(strcmp(argv[i], "--asm_print") == 0 || strcmp(argv[i], "-a") == 0){
            options.asm_print = 1;
        }
        else if(strcmp(argv[i], "--ir_print") == 0 || strcmp(argv[i], "-i") == 0){
            options.ic_print = 1;
        }
        else if(strcmp(argv[i], "--table_print") == 0 || strcmp(argv[i], "-t") == 0){
            options.table_print = 1;
        }
        else if(strcmp(argv[i], "--tree_print") == 0 || strcmp(argv[i], "-s") == 0){
            options.tree_print = 1;
        }
    }

    
    parse_function = &yyparse;

    file = fopen(argv[1], "r");
    yyin = file;

    
    table = create_table();
    yyparse();

    if(error_num > 0){
        printf("Erros encontrados, geração de código abortada! \n");
        return -1;
    }
    gen_code(sintTree);
    if(options.table_print == 1) print_table(table);
    if(options.tree_print == 1) print_tree(sintTree, 0);
    if(options.ic_print == 1) print_list(quadList);

    asm_gen(quadList);
    if(options.asm_print == 1) print_op_list();

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