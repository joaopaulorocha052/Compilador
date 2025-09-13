#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *arquivo;
    arquivo = fopen("sort.txt", "r");
    int ch;

    if (arquivo == NULL) {
        perror("Erro ao abrir o arquivo 'sort.txt'");
        return 1;
    }

    while((ch = fgetc(arquivo)) != EOF) {
        printf("%c",ch)
    }
    return 0;
}