int primos[30];

void crivo(int n) {
    int i; int j;
    
    i = 0;
    while (i < n) { 
        primos[i] = 1; 
        i = i + 1; 
    }
    
    primos[0] = 0; 
    primos[1] = 0;

    i = 2;
    while (i < n) {
        if (primos[i] == 1) {
            j = i + i;
            while (j < n) {
                primos[j] = 0;  
                j = j + i;
            }
        }
        i = i + 1;
    }
}

void main(void) {
    int i;
    int limite;
    
    limite = 30;
    crivo(limite);
    
    i = 0;
    while (i < limite) {
        if (primos[i] == 1) {
            output(i); 
        }
        i = i + 1;
    }
}