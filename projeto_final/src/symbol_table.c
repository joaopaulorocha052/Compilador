#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "symbol_table.h"
extern int error_num;

static int hash(char* key){

    int temp_sum = 0;
    for(int i=0; i<strlen(key); i++){
        temp_sum = temp_sum + key[i];
    }

    return temp_sum;
}


static void deallocate_line_list(LineList* list){
    LineList* temp = list->next_line;
    while(temp != NULL){
        LineList* next = temp->next_line;
        free(temp);
        temp = next;
    }

    free(list);
    
    return;
}

static void deallocate_item(HashItem* item){

    HashItem* current_item = item;

    while(current_item != NULL){
        HashItem* next_item = current_item->next_item;

        free(current_item->value);
        free(current_item->scope);

        if(current_item->lines != NULL){
            deallocate_line_list(current_item->lines);
        }

        free(current_item);
        current_item = next_item;

        
    }


    return;
}

static void deallocate_table(HashTable* table){

    for(int i = 0; i<HASH_TABLE_SIZE; i++){
        if(table->items[i] != NULL) deallocate_item(table->items[i]);
    }
    free(table->items);
    free(table);

    return;
}

static LineList* create_line_list(int num){
    LineList* temp = (LineList*) malloc(sizeof(LineList));

    temp->line_num = num;
    temp->next_line = NULL;

    return temp;
}
void insert_line(HashItem* item, int num){
    


    if(item->lines == NULL){
        item->lines = create_line_list(num);
        return;
    }

    LineList* temp = item->lines;

    while(temp->next_line != NULL){
        temp = temp->next_line;
    }

    temp->next_line = create_line_list(num);

    return;
}
static HashItem* create_item(char* value, char* scope, ExpKind kind, ExpType type, int qnt){
    HashItem* temp = (HashItem*) malloc(sizeof(HashItem));
    if(temp == NULL) return NULL;
    temp->value = strdup(value);
    temp->scope = strdup(scope);
    temp->next_item = NULL;
    temp->lines = NULL;
    temp->kind = kind;
    temp->type = type;
    temp->qnt_param = qnt;

    return temp;
}

HashTable* create_table(){
    HashTable* temp = (HashTable*) malloc(sizeof(HashTable));
    temp->size = 0;
    temp->items = (HashItem**) malloc(sizeof(HashItem*) * HASH_TABLE_SIZE);
    
    for(int i=0; i<HASH_TABLE_SIZE; i++) temp->items[i] = NULL;

    insert_item(temp, "input", "global", 0, FUNC, INT_EXP, 0);
    insert_item(temp, "output", "global", 0, FUNC, VOID_EXP, 1);

    return temp;
}

void insert_item(HashTable* table, char* value, char* scope, int line_num, ExpKind kind, ExpType type, int qnt){

    if ((kind == VAR) && (type == VOID_EXP)) {
        printf("ERRO SEMÂNTICO: Variável tipo void: %s - Linha: %d\n", value, line_num );
        error_num++;
        return;
    }

    HashItem* item = create_item(value, scope, kind, type, qnt);
    int index = hash(value)%HASH_TABLE_SIZE;

    if(search_item(table, value, scope) != NULL){
        printf("ERRO SEMÂNTICO: ID duplicado: %s - Linha: %d\n", value, line_num );
        error_num++;
        return;
    }
    if(kind == VAR) {
        HashItem* temp = search_item(table, value, "global");
        if(temp != NULL && temp->kind == FUNC){
            printf("ERRO SEMÂNTICO: ID já declarado como função: %s - Linha: %d\n", value, line_num );
            error_num++;
            return;
        }
    }
    insert_line(item, line_num);

    if(table->items[index] == NULL){
        table->items[index] = item;
        table->size++;
    }
   else{
        //printf("Tá cheio já neh\n"); // Mensagem original
        HashItem* current_item = table->items[index];
        
        while(current_item->next_item != NULL){ 
            current_item = current_item->next_item;
        }

        current_item->next_item = item;
    }
    
    return;
}


HashItem* search_item(HashTable* table, char* value, char* scope){
    int index = hash(value)%HASH_TABLE_SIZE;


    if(table->items[index] == NULL){
        return NULL;
    }

    HashItem* current_item = table->items[index];
    while (current_item != NULL){
        if(strcmp(current_item->value, value) == 0 \
        && strcmp(current_item->scope, scope) == 0){
            return current_item;
        }

        current_item = current_item->next_item;
    }
    
    return NULL;
}

const char* kind_to_string(ExpKind kind){
    switch (kind)
    {
    case VAR:
        return "VAR";
        break;
    case FUNC:
        return "FUNC";
        break;
    default:
        return "UNKNOWN KIND";
        break;
    }

}
const char* type_to_string(ExpType type){
    switch (type)
    {
    case INT_EXP:
        return "INT";
        break;
    case VOID_EXP:
        return "VOID";
        break;
    default:
        return "UNKNOWN TYPE";
        break;
    }

}
void print_table(HashTable* table){
    printf("============================================== TABELA DE SIMBOLOS ==============================================\n");
    printf("======== ID ========= EXPRESSAO ======= ESCOPO ====== TIPO = Quantidade de Parâmetros ============== LINHAS ==========\n");


    for(int i=0; i<HASH_TABLE_SIZE; i++){
        HashItem* current_item = table->items[i];
        if(current_item == NULL) continue;

        printf("|%-20s||%-8s||%-20s||%-5s||%-20d||     ",current_item->value, kind_to_string(current_item->kind), current_item->scope, type_to_string(current_item->type), current_item->qnt_param);
        LineList* temp = current_item->lines;
        printf("%d", temp->line_num);
        temp = temp->next_line;
        while(temp != NULL){
            printf(", %d", temp->line_num);
            temp = temp->next_line;
        }
        printf("\n");
        
        HashItem* temp_item = current_item;
        while(temp_item->next_item != NULL){
            printf("|%-20s||%-8s||%-20s||%-5s||%-20d||     ",temp_item->next_item->value, kind_to_string(temp_item->next_item->kind), temp_item->next_item->scope, type_to_string(temp_item->next_item->type), temp_item->next_item->qnt_param);
            LineList* temp = temp_item->next_item->lines;
            printf("%d", temp->line_num);
            temp = temp->next_line;
            while(temp != NULL){
                printf(", %d", temp->line_num);
                temp = temp->next_line;
            }
            printf("\n");
            temp_item = temp_item->next_item;
        }
    }
}
