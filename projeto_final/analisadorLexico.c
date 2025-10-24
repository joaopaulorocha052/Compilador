#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>


bool end_of_file = false;

#define N_ESTADOS 15
#define N_CLASSES 8

int states[N_ESTADOS][N_CLASSES] = {
/* letra num  =   <   >  simb  esp  outro */
/*0*/ { 1, 2, 3, 6, 9, 12, 13, 14 },
/*1*/ { 1, 1, 14,14,14,14,14,  2 },
/*2*/ {14, 2, 14,14,14,14,14,  2 },
/*3*/ {14,14, 4,14,14,14,14,  5 },
/*4*/ {14,14,14,14,14,14,14, 14 },
/*5*/ {14,14,14,14,14,14,14, 14 },
/*6*/ {14,14, 7,14,14,14,14,  8 },
/*7*/ {14,14,14,14,14,14,14, 14 },
/*8*/ {14,14,14,14,14,14,14, 14 },
/*9*/ {14,14,14,14,10,14,14, 11 },
/*10*/{14,14,14,14,14,14,14, 14 },
/*11*/{14,14,14,14,14,14,14, 14 },
/*12*/{14,14,14,14,14,14,14, 14 },
/*13*/{13,13,13,13,13,13,13,  0 },
/*14*/{14,14,14,14,14,14,14, 14 }
};

bool Advance[N_ESTADOS][N_CLASSES] = {
/* letra num   =    <    >   simb  esp  outro */
/*0*/ { true, true, true, true, true, true, true, false },
/*1*/ { true, true, false,false,false,false,false, false },
/*2*/ { false,true, false,false,false,false,false, false },
/*3*/ { false,false,true, false,false,false,false, false },
/*4*/ { false,false,false,false,false,false,false, false },
/*5*/ { false,false,false,false,false,false,false, false },
/*6*/ { false,false,true, false,false,false,false, false },
/*7*/ { false,false,false,false,false,false,false, false },
/*8*/ { false,false,false,false,false,false,false, false },
/*9*/ { false,false,false,false,true, false,false, false },
/*10*/{ false,false,false,false,false,false,false, false },
/*11*/{ false,false,false,false,false,false,false, false },
/*12*/{ false,false,false,false,false,false,false, false },
/*13*/{ true, true, true, true, true, true, true, false },
/*14*/{ false,false,false,false,false,false,false, false }
};

bool Acept[N_ESTADOS] = {
/*0*/  false,
/*1*/  true,   // ID
/*2*/  true,   // NUM
/*3*/  false,
/*4*/  true,   // ==
/*5*/  true,   // =
/*6*/  false,
/*7*/  true,   // <=
/*8*/  true,   // <
/*9*/  false,
/*10*/ true,   // >=
/*11*/ true,   // >
/*12*/ true,   // símbolos simples
/*13*/ false,  // espaço
/*14*/ false   // erro
};

// int states[3][3] ={
//                     {1, 3, 3}, 
//                     {1, 1, 2}, 
//                     {3, 3, 3}
//                     };
// bool Advance[3][3] = {
//                     {true, false, false}, 
//                     {true, true, false}, 
//                     {false, false, false}
//                     };
// bool Acept[3] = {false, false, true};

char *key_words[] = {"int", "for", "return", "while", "void", "if"};

bool check_for_key_word(char *str){
    for(int i=0; i<sizeof(key_words)/8; i++){
        if(strcmp(str, key_words[i]) == 0) return true;
    }

    return false;
}

char get_char(FILE * f){
    
    char next_char;
    next_char = getc(f);
    if(next_char != EOF){
        return next_char;
    }
    
    end_of_file = true;
    return -1;
}

void form_lexeme(char dest[], char append){
    int i = 0;

    
    while(dest[i] != '\0') i++;
    

    dest[i] = append;
    dest[i++] = '\0';
}

int get_char_state(char c){
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return 0;
    else if (c >= '0' && c <= '9') return 1;
    else if (c == '=') return 2;
    else if (c == '<') return 3;
    else if (c == '>') return 4;
    else if (c == ',' || c == '.' || c == ';' || c == '[' || c == ']' ||
             c == '{' || c == '}' || c == '(' || c == ')' ||
             c == '+' || c == '-' || c == '*' || c == '/') return 5;
    else if (c == ' ' || c == '\t' || c == '\n') return 6;
    else return 7;
}

int main(int argc, char *argv[]){

    char *file_path;
    if(argc < 2) file_path = "sort.txt";
    else file_path = argv[1];


    FILE *file = fopen(file_path, "r");
    

    if (file == NULL){
        printf("Arquivo nao pode ser aberto\n");
        return -1;
    }
    char lexeme[10] = "";

    while(end_of_file == false){
        
        int current_state = 0;
        int new_state;
        int lexeme_index = 0;
        bool error = false;
        char current_char = get_char(file);
        if(current_char == EOF) break;
        memset(lexeme, 0, sizeof(lexeme));
        // lexeme[lexeme_index] = current_char;
        // lexeme_index++;

        if(!isalpha(current_char)) error = true;
    
        while (Acept[current_state] == false && error == false){
            new_state = states[current_state][get_char_state(current_char)];
    
            if(Advance[current_state][get_char_state(current_char)]) {
                lexeme[lexeme_index] = current_char;
                lexeme_index++;
                current_char = get_char(file);
                if (current_char == EOF) break;
            }else
            {
                ungetc(current_char, file);
            }
            
    
            current_state = new_state;
            
            
            
        }

        if(!Acept[current_state]) printf("%c", current_char);
        else if(check_for_key_word(lexeme)) printf("%s", lexeme);   
        else printf("ID (%s)", lexeme);
    }

    return 0;
}