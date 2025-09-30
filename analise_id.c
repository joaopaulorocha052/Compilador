#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int getCategoria(int ch) {
    if (isalpha(ch)) return 0;
    if (isdigit(ch)) return 1;
    return 2;
}

int main() {
    FILE *arquivo;
    FILE *tokens;
    int ch;
    const char *reservadas[] = {
        "int","char","float","double","void",
        "if","else","switch","case","default","for","while","do",
        "break","continue","return"
    };
    int qntReservadas = sizeof(reservadas) / sizeof(reservadas[0]);
    int reservada = 0;
    char buffer[100];
    int tam = 0;

    int estadoInicial = 0;
    int estadoAtual = estadoInicial;
    int t[2][3] = {
        {1,    3,    3},
        {1,    1,    2}
    };
    int aceita[3] = { 0, 0, 1};
    int caracterAtual;


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
    printf("inicializando ...");

    while((ch = fgetc(arquivo)) != EOF) {
        int caracterAtual = getCategoria(ch);

        estadoAtual = t[estadoAtual][caracterAtual];

        if (estadoAtual == 3) {
            buffer[tam] = '\0';
            fputs(buffer, tokens);
            fputc(ch, tokens);
            estadoAtual = estadoInicial;
            tam = 0;
        } else if ( estadoAtual == 2) {
            buffer[tam] = '\0';
            reservada = 0;
            for (int i = 0; i < qntReservadas; i++) {
                    if (strcmp(buffer, reservadas[i]) == 0) {
                        reservada = 1;
                        break;
                    }
            }
             if (reservada) {
                fputs(buffer, tokens);
            } else {
                fputs("ID", tokens);
            }
            fputc(ch, tokens);
            tam = 0;
            estadoAtual = estadoInicial;
        } else {
            if (tam < 99) {
                buffer[tam++] = ch;
            }
        }

        printf("%c",ch);
    }

    fclose(arquivo);
    fclose(tokens);

    return 0;
}