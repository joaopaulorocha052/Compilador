
int stalinSort(int arr[], int size){


    int writeIndex; int maxVal; int i;
    writeIndex = 1;
    maxVal = arr[0];
    i = 1;



    while(i < size){

        if(arr[i] >= maxVal){
            maxVal = arr[i];
            arr[writeIndex] = arr[i];
            writeIndex = writeIndex + 1;
        }

        i = i + 1;
    }

    return writeIndex;
    
}


void printArray(int arr[], int size){
    
    int i;
    i = 0;

    while(i < size){
        output(arr[i]);
        i = i + 1;
    }

}



void main(void){

    int vet[25];

    int i;
    int arraySize;
    int newSize;
    i = 0;
    arraySize = 25;
    while (i < arraySize){
        vet[i]= input();
        i = i + 1;
    }

    newSize = stalinSort(vet, arraySize);

    printArray(vet, newSize);

}