#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *arquivo;
    FILE *tokens;
    int ch;

    arquivo = fopen("sort.txt", "r");
    

    if (arquivo == NULL) {
        perror("Erro ao abrir o arquivo 'sort.txt'");
        return 1;
    }

    tokens = fopen("tokens.txt", "w");
    if (tokens == NULL) {
        perror("Erro ao criar o arquivo de destino (copia.txt)");
        fclose(arquivo);
        return 1;
    }

    while((ch = fgetc(arquivo)) != EOF) {

        fputc(ch, tokens);

        printf("%c",ch);
    }

    fclose(arquivo);
    fclose(tokens);

    return 0;
}