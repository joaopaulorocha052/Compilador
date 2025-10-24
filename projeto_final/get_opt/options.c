

#include <stdlib.h>
#include <stdio.h>
#include "options.h"


int check_dash_option(char str[]){
    return (str[0] == '-');
}
void resolve_option(char option, char* value ){
    switch (option)
    {
        case 'l':
            printf("Utilizando o Flex para análise Léxica\n");
            flex_flag = 1;
            break;
        case 'd':
            printf("Olá, estou aqui\n");
            break;
        default:
            printf("Opção desconhecida: -%c\n", option);
            break;
    }
}

void options(int argc, char *argv[]){
    int i = 2;
    size_t argument_list_size = argc;
    while(argv[i] != NULL){
        if(check_dash_option(argv[i])){
            if (i + 1 < argument_list_size && !check_dash_option(argv[i + 1])) {
                resolve_option(argv[i][1], argv[i + 1]);
                ++i;
            } else {
                resolve_option(argv[i][1], NULL);
            }
        }
        i++;
    }


}