int nthFibonacci(int n) {
    if (n <= 1){
        return n;
    }
    return nthFibonacci(n - 1) + nthFibonacci(n - 2);
}

int main(void){
    int n; int result;
    n = input();
    result = nthFibonacci(n);
    output(result);
    return 0;
}