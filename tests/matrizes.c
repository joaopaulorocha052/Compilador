int matA[9];
int matB[9];
int matC[9];

void inicializaMatrizes(void) {
    int i;
    i = 0;
    while(i < 9) {
        matA[i] = i + 1;     
        matB[i] = 1;        
        matC[i] = 0;
        i = i + 1;
    }
}

void multiplica(void) {
    int i; int j; int k;
    i = 0;
    while (i < 3) {
        j = 0;
        while (j < 3) {
            int soma; int idxA; int idxB; int idxC;
            soma = 0;
            k = 0;
            while (k < 3) {
                idxA = (i * 3) + k;
                idxB = (k * 3) + j;
                soma = soma + (matA[idxA] * matB[idxB]);
                k = k + 1;
            }
            idxC = (i * 3) + j;
            matC[idxC] = soma;
            j = j + 1;
        }
        i = i + 1;
    }
}

void main(void) {
    int i;
    inicializaMatrizes();
    multiplica();
    
    i = 0;
    while(i < 9) {
        output(matC[i]);
        i = i + 1;
    }
}