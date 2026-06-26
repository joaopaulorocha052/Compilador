#include <stdio.h>
#include <stdlib.h>
#include <code_gen.h>
#include <string.h>
#include "symbol_table.h"
#include "utils.h"

#define MAX_OPERATION 100

extern HashTable* table;
int label_position[32];
char current_function_scope[SCOPE_NAME_SIZE] = "global";

AsmOperation operation_list[MAX_OPERATION];
int current_sp = 0;
int current_fp = 0;
int current_local_offset = 0;

int current_line = 0;

int current_list_position;

int get_new_register()
{
    static int returned_register = 4;

    if(returned_register >= MAX_NUM_REGISTER-3) returned_register = 0;

    return returned_register++;
}


AsmOperand reg(int register_operand){
    return (AsmOperand) {.operand=register_operand, .type=ASM_REGISTER};
}

AsmOperand num(int number_operand){
    return (AsmOperand) {.operand=number_operand, .type=ASM_NUMBER};
}

AsmOperation emit_operation(ASM_OPERATION op_type, AsmOperand operand1, AsmOperand operand2, AsmOperand operand3)
{
    AsmOperation temp = (AsmOperation) {.asm_operation_type=op_type, .operands={operand1, operand2, operand3}};
    
    operation_list[current_list_position] = temp;

    current_list_position++;
    return temp;
}

void print_op_list(){
    for(int i=0; i<current_list_position; i++){
        print_asm_operation(operation_list[i]);
    }
}
AsmOperation translate_quad(struct Quadrupla quad)
{
    mem_offset_t temp_offset;
    switch (quad.type)
    {
        case Q_LABEL:
            label_position[quad.addr1.value.int_num] = current_line; 
            break;
        case Q_GOTO:
            emit_operation(ASM_JUMP, num(0), num(0), num(quad.addr1.value.int_num));
            current_line++;
            break;
        case Q_INIT:
            if(strcmp(current_function_scope, "global") == 0)
            {
                add_offset_to_symbol(table, quad.addr1.value.name, current_function_scope, current_local_offset);
                emit_operation(ASM_ADDI, reg(STACK_POINTER), reg(STACK_POINTER), num(1));

                current_local_offset++;
            }
            else{
                add_offset_to_symbol(table, quad.addr1.value.name, current_function_scope, current_sp);
                emit_operation(ASM_ADDI, reg(STACK_POINTER), reg(STACK_POINTER), num(1));
                current_sp++;
            }
            current_line++;
            break;

        case Q_INITVET:
            if(strcmp(current_function_scope, "global") == 0)
            {
                add_offset_to_symbol(table, quad.addr1.value.name, current_function_scope, current_local_offset);
                current_local_offset += quad.addr2.value.int_num;
            }
            else{
                add_offset_to_symbol(table, quad.addr1.value.name, current_function_scope, current_sp);
                current_sp += quad.addr2.value.int_num;
            }
            emit_operation(ASM_ADDI, reg(STACK_POINTER), reg(STACK_POINTER), num(quad.addr2.value.int_num));
            current_line++;
            break;
        case Q_ASSIGN: {
            int current_pointer; // Aponta para a base do DESTINO
            HashItem* temp_hash_item = search_item(table, quad.addr1.value.name, current_function_scope);
            
            // Verifica se o DESTINO é local (usa FP) ou global (usa Registo 0)
            if(temp_hash_item != NULL) {
                current_pointer = FRAME_POINTER;
            } else {
                current_pointer = 0;
            }
            
            int new_reg = get_new_register();
            mem_offset_t offset = get_symbol_offset(table, quad.addr1.value.name, current_function_scope);

            if(quad.addr2.type == NAME) {
                // SE A ORIGEM FOR UMA VARIÁVEL:
                int source_pointer; // Aponta para a base da ORIGEM
                HashItem* source_hash_item = search_item(table, quad.addr2.value.name, current_function_scope);
                
                // Verifica se a ORIGEM é local ou global
                if(source_hash_item != NULL) {
                    source_pointer = FRAME_POINTER;
                } else {
                    source_pointer = 0;
                }
                
                mem_offset_t assign_offset = get_symbol_offset(table, quad.addr2.value.name, current_function_scope);
                
                // CORREÇÃO: Lemos da origem (Base source_pointer) e gravamos no destino (Base current_pointer)
                // Ordem: Instrução, Reg Destino/Dado, Reg Base, Offset Numérico
                emit_operation(ASM_LW, reg(new_reg), reg(source_pointer), num(assign_offset));
                emit_operation(ASM_SW, reg(new_reg), reg(current_pointer), num(offset));
            } else {
                // SE A ORIGEM FOR UM NÚMERO (CONSTANTE):
                // CORREÇÃO: Usamos o reg(0) como base do ADDI para não acumular "lixo" do registrador alocado
                emit_operation(ASM_ADDI, reg(new_reg), reg(0), num(quad.addr2.value.int_num));
                emit_operation(ASM_SW, reg(new_reg), reg(current_pointer), num(offset));
            }
            
            current_line += 2;
            break;
        }
        case Q_IGUAL:
        
            int reg1 = get_new_register();
            int reg2 = get_new_register();
            int temp_reg = get_new_register();

            if (quad.addr2.type == INT_NUM) { 
                // Se for número, usamos ADDI reg1, R0, valor
                emit_operation(ASM_ADDI, reg(reg1), reg(0), num(quad.addr2.value.int_num));
            } else {
                // Se for variável, pegamos o offset e usamos LW reg1, offset(FP)
                mem_offset_t offset1 = get_symbol_offset(table, quad.addr2.value.name, current_function_scope);
                emit_operation(ASM_LW, reg(reg1), num(offset1), reg(FRAME_POINTER));
            }

            if (quad.addr3.type == INT_NUM) {
                emit_operation(ASM_ADDI, reg(reg2), reg(0), num(quad.addr3.value.int_num));
            } 
            else {
                mem_offset_t offset2 = get_symbol_offset(table, quad.addr3.value.name, current_function_scope);
                emit_operation(ASM_LW, reg(reg2), num(offset2), reg(FRAME_POINTER));
            }


            emit_operation(ASM_EQ, reg(temp_reg), reg(reg1), reg(reg2));


            mem_offset_t dest_offset = get_symbol_offset(table, quad.addr1.value.name, current_function_scope);
            emit_operation(ASM_SW, reg(temp_reg), num(dest_offset), reg(FRAME_POINTER));
            current_line+=4;
            
            break;
        case Q_IF:
            int temp_reg1 = get_new_register();

            // ---------------------------------------------------------
            // PASSO 1: Carregar a variável de condição (addr1, ex: _t0)
            // ---------------------------------------------------------
            if (quad.addr1.type == INT_NUM) {
                // Caso raro onde o IF testa um número direto (ex: if (1) )
                emit_operation(ASM_ADDI, reg(temp_reg1), reg(0), num(quad.addr1.value.int_num));
            } else {
                // Caso padrão: carrega o valor booleano salvo na memória
                mem_offset_t offset = get_symbol_offset(table, quad.addr1.value.name, current_function_scope);
                emit_operation(ASM_LW, reg(temp_reg1), num(offset), reg(FRAME_POINTER));
            }

            // ---------------------------------------------------------
            // PASSO 2: Preparar o valor Verdadeiro (1) para o BEQ
            // ---------------------------------------------------------
            int reg_true = get_new_register();
            emit_operation(ASM_ADDI, reg(reg_true), reg(0), num(1));

            // ---------------------------------------------------------
            // PASSO 3: Realizar o salto (Branch)
            // ---------------------------------------------------------
            // O addr2 guarda o nome da label (como string). Como o seu code_gen.c
            // gera labels como números (ex: "1", "2"), convertemos para inteiro.
            int target_label = label_position[quad.addr2.value.int_num]; 

            // Emite: BEQ temp_reg, reg_true, target_label
            emit_operation(ASM_BEQ, reg(temp_reg1), reg(reg_true), num(target_label));
            current_line+=3;
            break;
        case Q_FUNCLABEL:
            current_fp = current_sp;
            memset(current_function_scope, 0, SCOPE_NAME_SIZE);
            strcpy(current_function_scope, quad.addr1.value.name);
            break;
        
        case Q_FUNCEND:
            current_local_offset = 0;
            memset(current_function_scope, 0, SCOPE_NAME_SIZE);
            strcpy(current_function_scope, "global");       
            break;
            
        default:
            break;
    }
}

void asm_gen(struct QuadrupleList* list)
{   
    printf("\n\nAsm generator using %d Registers\n\n", MAX_NUM_REGISTER);


    struct ListNode* node = list->list;

    while(node != NULL){
        translate_quad(node->quad);
        node = node->next;
    }

    print_op_list();   
}