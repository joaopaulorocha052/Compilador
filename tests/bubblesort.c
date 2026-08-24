int vet[10];


int bubbleSort(int arr[], int arraySize, int teste){


    int i; int j; int swapped;
    
    i = 0;
    j = 0;


    while(i < arraySize){
        swapped = 0;
         j = 0;
        while( j < arraySize - i - 1){
            int nextItemIndex;
            nextItemIndex = j + 1;
            if(arr[j] > arr[nextItemIndex]){
                int t;

                t = arr[nextItemIndex];

                arr[nextItemIndex] = arr[j];
                arr[j] = t;

                swapped = 1;
            }
            j = j + 1;
        }
        i = i + 1;
        if(swapped == 0){
            i =  arraySize + 1;
        }
    }

    return 0;
}


void main(void){

    int i;
    
    i = 0;

    while(i<10){
        vet[i] = input();
        i = i + 1;
    }

    bubbleSort(vet, 10, 0);

    i = 0;

    while(i < 10){
        output(vet[i]);
        i = i + 1;
    }
}