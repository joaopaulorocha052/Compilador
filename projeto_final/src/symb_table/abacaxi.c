#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "symbol_table.h"

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
static HashItem* create_item(char* value, char* scope){
    HashItem* temp = (HashItem*) malloc(sizeof(HashItem));
    if(temp == NULL) return NULL;

    temp->value = strdup(value);
    temp->scope = strdup(scope);
    temp->next_item = NULL;
    temp->lines = NULL;

    return temp;
}

static HashTable* create_table(){
    HashTable* temp = (HashTable*) malloc(sizeof(HashTable));
    temp->size = 0;
    temp->items = (HashItem**) malloc(sizeof(HashItem*) * HASH_TABLE_SIZE);
    

    for(int i=0; i<HASH_TABLE_SIZE; i++) temp->items[i] = NULL;

    return temp;
}

void insert_item(HashTable* table, char* value, char* scope, int line_num){

    HashItem* item = create_item(value, scope);
    insert_line(item, line_num);
    int index = hash(value)%HASH_TABLE_SIZE;

    if(table->items[index] == NULL){
        table->items[index] = item;
        table->size++;
    }
   else{
        //printf("Tá cheio já neh\n"); // Mensagem original
        int existing_item = 0;
        HashItem* current_item = table->items[index];
        
        while(current_item != NULL){ 
            
            //printf("Item Atual: %s %s\n", current_item->value, value); // Mensagem original
            if(strcmp(current_item->value, value) == 0 && strcmp(current_item->scope, scope) == 0){
                
                insert_line(current_item, line_num);
                //printf("%d %d\n", current_item->lines->line_num, current_item->lines->next_line->line_num); // Mensagem original
                
                existing_item = 1;

                deallocate_item(item);

                return;
            }
            
            if(current_item->next_item == NULL) break; 
            current_item = current_item->next_item;
        }

        if(!existing_item) current_item->next_item = item;
    }
    
    return;
}


bool search_item(HashTable* table, char* value, char* scope){
    int index = hash(value)%HASH_TABLE_SIZE;

    if(strcmp(table->items[index]->value, value) == 0 \
    && strcmp(table->items[index]->scope, scope) == 0){
        return true;
    }

    HashItem* current_item = table->items[index];
    while (current_item != NULL){
        if(strcmp(current_item->value, value) == 0 \
        && strcmp(current_item->scope, scope) == 0){
            return true;
        }

        current_item = current_item->next_item;
    }
    
    return false;
}

void print_table(HashTable* table){
    printf("============================================== TABELA DE SIMBOLOS ==============================================\n");
    printf("======== ID ================ EXPRESSAO ============= ESCOPO ============== TIPO ============== LINHAS ==========\n");


    for(int i=0; i<HASH_TABLE_SIZE; i++){
        HashItem* current_item = table->items[i];
        if(current_item == NULL) continue;

        printf("|%-20s||%-20s||%-20s||%-20s||     ",current_item->value, "", current_item->scope, "");
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
            printf("|%-20s||%-20s||%-20s||%-20s||     ",temp_item->next_item->value, "", temp_item->next_item->scope, "");
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


int main(){
    
    HashTable* table = create_table();
    
    insert_item(table, "abc", "def", 2);
    insert_item(table, "abc", "def", 3);
    insert_item(table, "abc", "ghi", 3);
    insert_item(table, "sgdfbd", "afgfsfb", 45);



    print_table(table);
    deallocate_table(table);
    
    return 0;
    


}