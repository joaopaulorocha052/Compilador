int ehPalindromo(int vet[], int tamanho){
    int dir; int esq;


    esq = 0; dir = tamanho - 1;
    while(esq < dir) {
        if(vet[esq] != vet[dir]){
            return 0;
        }

        esq = esq + 1;
        dir = dir - 1;
    }

    return 1;
}


void main (void){
    int vetor[5];
    int result;

    vetor[0] = 1;
    vetor[1] = 2;
    vetor[2] = 5;
    vetor[3] = 2;
    vetor[4] = 0;

    result = ehPalindromo(vetor, 5);

    output(result); 

}