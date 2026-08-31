void main(void){


    int continueLoop;

    int num1; int num2; int operacao; int result;
    
    
    continueLoop = 1;
    while(continueLoop == 1){

        operacao = input();



        if(operacao != 16){

            num1 = input();
            num2 = input();
    
            if(operacao == 1){
                result = num1 + num2;
            }
    
            if(operacao == 2){
    
                if(num1 < num2){
                    result = 9999;
                }else{
                    result = num1 - num2;
                }
            }
            if(operacao == 4){
                result = num1 * num2;
            }
            if(operacao == 8){
                if(num2 == 0){
                    result = 9999;
                }else{
                    result == num1/num2;
    
                }
            }
    
            output(result);
        }
        else{
            continueLoop = 0;
        }
    }
}