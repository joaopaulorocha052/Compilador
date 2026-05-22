#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "code_gen.h"

struct QuadrupleList* quadList;

struct QuadrupleList* init_list(){
    struct QuadrupleList* list = malloc(sizeof(struct QuadrupleList));
    list->list = NULL;
    list->tail = NULL;

    return list;
}


int add_quadruple_to_list(struct QuadrupleList* list, struct Quadrupla quad){
    struct ListNode* temp = malloc(sizeof(struct ListNode));

    temp->quad = quad;
    temp->next = NULL;

    if(list->tail == NULL){
        list->list = temp;
        list->tail = temp;

        return 1;
    }else{
        list->tail->next = temp;
        list->tail = temp;

        return 1;
    }
    
    return 0;
}

void print_list(struct QuadrupleList* list){
    struct ListNode* current = list->list;
    while(current != NULL){
        print_quad(current->quad);
        current = current->next;
    }

    return;
}

void print_quad(struct Quadrupla quad){
    struct ADDR addrs[3] = {quad.addr1, quad.addr2, quad.addr3};
    
    printf("%s ", quadruple_type_to_string(quad.type));
    for(int i=0; i<3; i++){
        if(addrs[i].type == INT_NUM){
            printf("%d ", addrs[i].value.int_num);
        }
        else{
            printf("%s ", addrs[i].value.name);
        }
    }
    
  printf("\n");
  
}

int check_address_is_num(char* value) {
    return isdigit(value[0]);
}

const char* quadruple_type_to_string(QUADRUPLE_TYPES type) {
    switch (type) {
        case Q_SOMA:        return "Q_SOMA";
        case Q_SUB:         return "Q_SUB";
        case Q_DIV:         return "Q_DIV";
        case Q_MULT:        return "Q_MULT";
        case Q_MAIOR:       return "Q_MAIOR";
        case Q_MAIOR_Q:     return "Q_MAIOR_Q";
        case Q_MENOR_Q:     return "Q_MENOR_Q";
        case Q_IGUAL:       return "Q_IGUAL";
        case Q_DIFF:        return "Q_DIFF";
        case Q_MENOR:       return "Q_MENOR";
        case Q_ASSIGN:      return "Q_ASSIGN";
        case Q_ASSIGN_VET:  return "Q_ASSIGN_VET";
        case Q_ACCESS_VET:  return "Q_ACCESS_VET";
        case Q_IF:          return "Q_IF";
        case Q_GOTO:        return "Q_GOTO";
        case Q_INITVET:     return "Q_INITVET";
        case Q_INIT:        return "Q_INIT";
        case Q_DEF:         return "Q_DEF";
        case Q_CALL:        return "Q_CALL";
        case Q_ARG:         return "Q_ARG";
        case Q_RETURN:      return "Q_RETURN";
        case Q_HALT:        return "Q_HALT";
        case Q_PARAM:       return "Q_PARAM";
        case Q_LABEL:       return "Q_LABEL";
        default:            return "UNKNOWN";
    }
}