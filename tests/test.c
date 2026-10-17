int vet[5];

int teste(int a, int b, int c, int d, int e, int f, int abacaxi[]){
    return a + b + c + d + e + f + abacaxi[3] + abacaxi[4];
}






void main(void){
    int i;
    i = 0;

    while(i < 5){
        vet[i] = i;
        i = i + 1;
    }


    output(teste(1, 2, 3, 4, 6, 7, vet));
}