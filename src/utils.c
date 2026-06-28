#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "code_gen.h"
#include "asm_gen.h"

struct QuadrupleList* quadList;

extern int label_position[32];

extern int call_target[100];
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

    if(quad.type == Q_LABEL){
        printf("L%d: \n", quad.addr1.value.int_num);
        return;
    }
    else if(quad.type == Q_FUNCEND){
        printf("end %s\n\n", quad.addr1.value.name);
        return;
    }
    else if(quad.type == Q_FUNCLABEL){
        printf("\n%s:\n", quad.addr1.value.name);
        return;
    }
    
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
        case Q_PARAM_END:   return "Q_PARAM_END";
        case Q_LABEL:       return "Q_LABEL";

        default:            return "UNKNOWN";
    }
}


typedef char binary_repr[33];

void convert_int_to_bin(int num){
    binary_repr result;
    memset(result, 0, 33);
    
    for(int i=31; i>=0; i--){
        if(num & (1U << i)){
            result[31-i] = '1';
        }else{
            result[31-i] = '0';
        }
    }
    
    printf("%s", result);
    
}


const char* asm_operation_to_string(ASM_OPERATION op) {
    switch (op) {
        case ASM_ADD:  return "ADD";
        case ASM_ADDI: return "ADDI";
        case ASM_SUB:  return "SUB";
        case ASM_SUBI: return "SUBI";
        case ASM_MULT: return "MULT";
        case ASM_DIV:  return "DIV";
        case ASM_JUMP: return "JUMP";
        case ASM_JAL:  return "JAL";
        case ASM_JR:   return "JR";
        case ASM_BEQ:  return "BEQ";
        case ASM_BNE:  return "BNE";
        case ASM_LW:   return "LW";
        case ASM_SW:   return "SW";
        case ASM_AND:  return "AND";
        case ASM_OR:   return "OR";
        case ASM_LI:   return "LI";
        case ASM_SI:   return "SI";
        case ASM_EQ:   return "EQ";
        case ASM_NEQ:  return "NEQ";
        case ASM_GT:   return "GT";
        case ASM_GTE:  return "GTE";
        case ASM_LT:   return "LT";
        case ASM_LTE:  return "LTE";
        default:   return "UNKNOWN_OP";
    }
}

void print_operand(AsmOperand op) {
    if (op.type == ASM_REGISTER) {
        if (op.operand == FRAME_POINTER) {
            printf("FP");
        } else if (op.operand == STACK_POINTER) {
            printf("SP");
        } else if(op.operand == RETURN_ADDRESS_POINTER){
            printf("RA");
        }
        else {
            printf("R%d", op.operand);
        }
    } else if (op.type == ASM_NUMBER) {
        printf("%d", op.operand);
    }
}

void print_asm_operation(AsmOperation op) {
    printf("%s ", asm_operation_to_string(op.asm_operation_type));

    switch (op.asm_operation_type) {
        
        case ASM_ADD:
        case ASM_SUB:
        case ASM_MULT:
        case ASM_DIV:
        case ASM_AND:
        case ASM_OR:
        case ASM_EQ:
        case ASM_NEQ:
        case ASM_GT:
        case ASM_GTE:
        case ASM_LT:
        case ASM_LTE:
            print_operand(op.operands[0]);
            printf(", ");
            print_operand(op.operands[1]);
            printf(", ");
            print_operand(op.operands[2]);
            break;

        case ASM_ADDI:
        case ASM_SUBI:
            print_operand(op.operands[0]);
            printf(", ");
            print_operand(op.operands[1]);
            printf(", ");
            print_operand(op.operands[2]);
            break;

        case ASM_LI:
            print_operand(op.operands[0]);
            printf(", ");
            print_operand(op.operands[2]);
            break;

        case ASM_LW:
        case ASM_SW:
            print_operand(op.operands[0]);
            printf(", ");
            print_operand(op.operands[1]);
            printf("(");
            print_operand(op.operands[2]);
            printf(")");
            break;

        case ASM_BEQ:
        case ASM_BNE:
            print_operand(op.operands[0]);
            printf(", ");
            print_operand(op.operands[1]);
            printf(", ");
            printf("%d", label_position[op.operands[2].operand]);
            break;

        case ASM_JUMP:
            printf("%d", label_position[op.operands[2].operand]);
            break;

        case ASM_JAL:
            printf("%d", call_target[op.operands[2].operand]);
            break;

        case ASM_JR:
            print_operand(op.operands[0]);
            break;

        default:
            print_operand(op.operands[0]);
            printf(", ");
            print_operand(op.operands[1]);
            printf(", ");
            print_operand(op.operands[2]);
            break;
    }
    
    printf("\n");
}