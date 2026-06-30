int vet[10];
int vetTeste[10];

int fun(void){
    return 2;
}

int foo(void){
    return fun();
}

void main(void){
    vet[1] = input();
    vetTeste[1] = input();
    vetTeste[3] = foo();
    output(vet[1]);
    output(vetTeste[3]);

    vet[2] = vetTeste[1];

    output(vet[2]);
}