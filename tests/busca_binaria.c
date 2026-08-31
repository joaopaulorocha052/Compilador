int buscaBinaria(int vet[], int arraySize, int num){

    int sup; int inf; int index;

    inf = 0;
    sup = arraySize;
    while (inf < sup){
        index = (sup + inf)/2;

        if(vet[index] == num){
            return index;
        }

        if(vet[index] < num){
            sup = index - 1;
        }
        else{
            inf = index + 1;
        }
    }

    return 99;

}



void main(void){
    int vet[15];


    int i;
    i = 0;

    while(i < 15){
        vet[i] = i;
        i = i + 1;
    }


    output(buscaBinaria(vet, 15, 7));
}