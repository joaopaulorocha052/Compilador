#include <stdio.h>
#include <stdlib.h>
#include <code_gen.h>
#include <string.h>
#include "symbol_table.h"
#include "utils.h"

#define MAX_OPERATION 1000

extern HashTable* table;
int label_position[32];
char current_function_scope[SCOPE_NAME_SIZE] = "global";

int label_frame_offset[32];
int label_sp[32];

int call_target[MAX_OPERATION];
int call_count = 0;

AsmOperation operation_list[MAX_OPERATION];

int current_sp = 0;
int current_fp = 0;
int current_local_offset = 0;
int current_frame_offset = 0;

int current_func_prologue_idx = -1;

int current_param_count = 0;
int pending_param_regs[MAX_OPERATION];

int current_list_position;

int get_new_register()
{
    static int returned_register = 4;

    if(returned_register >= MAX_NUM_REGISTER-3) returned_register = 4;

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
    AsmOperation temp = (AsmOperation) {.asm_operation_type=op_type, .operands={operand1, operand2, operand3}, .msg = NULL};

    if (current_list_position >= MAX_OPERATION) {
        fprintf(stderr,
            "ERRO FATAL: o programa gerou mais de %d instrucoes "
            "(MAX_OPERATION). Aumente MAX_OPERATION em asm_gen.c.\n",
            MAX_OPERATION);
        exit(1);
    }

    operation_list[current_list_position] = temp;

    current_list_position++;
    return temp;
}

void print_op_list(){
    for(int i=0; i<current_list_position; i++){
        print_asm_operation(operation_list[i]);
    }
}


static int symbol_kind_flag(char* name, char* scope)
{
    HashItem* item = search_item(table, name, scope);
    if (item == NULL) item = search_item(table, name, "global");
    return (item != NULL) ? item->qnt_param : 0;
}

static int emit_vector_base_address(char* name, char* scope)
{
    HashItem* item = search_item(table, name, scope);
    int base = (item != NULL) ? FRAME_POINTER : 0;
    mem_offset_t off = get_symbol_offset(table, name, scope);

    if (symbol_kind_flag(name, scope) == 2) {
        int ptr_reg = get_new_register();
        emit_operation(ASM_LW, reg(ptr_reg), reg(base), num(off));
        return ptr_reg;
    } else {
        int base_reg = get_new_register();
        emit_operation(ASM_ADDI, reg(base_reg), reg(base), num(off));
        return base_reg;
    }
}

static int emit_binary_op(ASM_OPERATION asm_op, struct Quadrupla quad)
{
    int reg1 = get_new_register();
    int reg2 = get_new_register();
    int result_reg = get_new_register();
    int count = 0;

    if (quad.addr2.type == INT_NUM) {
        emit_operation(ASM_ADDI, reg(reg1), reg(0), num(quad.addr2.value.int_num));
    } else {
        HashItem* item2 = search_item(table, quad.addr2.value.name, current_function_scope);
        int base2 = (item2 != NULL) ? FRAME_POINTER : 0;
        mem_offset_t offset1 = get_symbol_offset(table, quad.addr2.value.name, current_function_scope);
        emit_operation(ASM_LW, reg(reg1), reg(base2), num(offset1));
    }
    count++;

    if (quad.addr3.type == INT_NUM) {
        emit_operation(ASM_ADDI, reg(reg2), reg(0), num(quad.addr3.value.int_num));
    } else {
        HashItem* item3 = search_item(table, quad.addr3.value.name, current_function_scope);
        int base3 = (item3 != NULL) ? FRAME_POINTER : 0;
        mem_offset_t offset2 = get_symbol_offset(table, quad.addr3.value.name, current_function_scope);
        emit_operation(ASM_LW, reg(reg2), reg(base3), num(offset2));
    }
    count++;

    emit_operation(asm_op, reg(result_reg), reg(reg1), reg(reg2));
    count++;

    HashItem* dest_item = search_item(table, quad.addr1.value.name, current_function_scope);
    int dest_base = (dest_item != NULL) ? FRAME_POINTER : 0;
    mem_offset_t dest_offset = get_symbol_offset(table, quad.addr1.value.name, current_function_scope);
    emit_operation(ASM_SW, reg(result_reg), reg(dest_base), num(dest_offset));
    count++;

    return count;
}

AsmOperation translate_quad(struct Quadrupla quad)
{
    mem_offset_t temp_offset;
    switch (quad.type)
    {
        case Q_LABEL:
            label_position[quad.addr1.value.int_num] = current_list_position;

            if (label_frame_offset[quad.addr1.value.int_num] == -1) {
                label_frame_offset[quad.addr1.value.int_num] = current_frame_offset;
                label_sp[quad.addr1.value.int_num] = current_sp;
            }
            break;
        case Q_GOTO: {
            int target_label = quad.addr1.value.int_num;

            emit_operation(ASM_JUMP, num(0), num(0), num(target_label));
            break;
        }
        case Q_PARAM: {

            int param_reg;

            if (quad.addr1.type == INT_NUM) {
                param_reg = get_new_register();
                emit_operation(ASM_ADDI, reg(param_reg), reg(0), num(quad.addr1.value.int_num));
            } else {
                char* arg_name = quad.addr1.value.name;

                if (symbol_kind_flag(arg_name, current_function_scope) != 0) {

                    param_reg = emit_vector_base_address(arg_name, current_function_scope);
                } else {
                    param_reg = get_new_register();
                    HashItem* arg_item = search_item(table, arg_name, current_function_scope);
                    int arg_base = (arg_item != NULL) ? FRAME_POINTER : 0;
                    mem_offset_t arg_offset = get_symbol_offset(table, arg_name, current_function_scope);
                    emit_operation(ASM_LW, reg(param_reg), reg(arg_base), num(arg_offset));
                }
            }

            pending_param_regs[current_param_count] = param_reg;
            current_param_count++;
            break;
        }

        case Q_CALL: {

            emit_operation(ASM_SW, reg(FRAME_POINTER), reg(STACK_POINTER), num(0));

            for (int p = 0; p < current_param_count; p++) {
                emit_operation(ASM_SW, reg(pending_param_regs[p]), reg(STACK_POINTER), num(3 + p));
            }
            current_param_count = 0;

            HashItem* dest_item = search_item(table, quad.addr2.value.name, current_function_scope);
            if (dest_item == NULL) {

                dest_item = search_item(table, quad.addr2.value.name, "global");
            }
            int dest_is_global = (dest_item != NULL && strcmp(dest_item->scope, "global") == 0);
            mem_offset_t dest_offset = get_symbol_offset(table, quad.addr2.value.name, current_function_scope);

            int dest_addr_reg = get_new_register();
            if (dest_is_global) {

                emit_operation(ASM_ADDI, reg(dest_addr_reg), reg(0), num(dest_offset));
            } else {

                emit_operation(ASM_ADDI, reg(dest_addr_reg), reg(FRAME_POINTER), num(dest_offset));
            }

            emit_operation(ASM_SW, reg(dest_addr_reg), reg(STACK_POINTER), num(2));

            mem_offset_t func_entry = get_symbol_offset(table, quad.addr1.value.name, "global");
            call_target[call_count] = func_entry;
            emit_operation(ASM_JAL, num(0), num(0), num(call_count));
            operation_list[current_list_position-1].msg = strdup(quad.addr1.value.name);
            call_count++;

            break;
        }

        case Q_PARAM_END:

            break;

        case Q_INIT:

            if(search_item(table, quad.addr1.value.name, current_function_scope) == NULL) {
                insert_item(table, quad.addr1.value.name, current_function_scope, 0, VAR, INT_EXP, 0);
            }

            if(strcmp(current_function_scope, "global") == 0)
            {
                add_offset_to_symbol(table, quad.addr1.value.name, current_function_scope, current_local_offset);
                current_local_offset++;
                emit_operation(ASM_ADDI, reg(STACK_POINTER), reg(STACK_POINTER), num(1));
                current_sp++;
            }
            else{
                add_offset_to_symbol(table, quad.addr1.value.name, current_function_scope, current_frame_offset);
                current_frame_offset++;
            }
            break;

        case Q_INITVET: {
            HashItem* vet_item = search_item(table, quad.addr1.value.name, current_function_scope);
            if(vet_item == NULL) {
                insert_item(table, quad.addr1.value.name, current_function_scope, 0, VAR, INT_EXP, 1);
            } else {

                vet_item->qnt_param = 1;
            }

            if(strcmp(current_function_scope, "global") == 0)
            {
                add_offset_to_symbol(table, quad.addr1.value.name, current_function_scope, current_local_offset);
                current_local_offset += quad.addr2.value.int_num;
                emit_operation(ASM_ADDI, reg(STACK_POINTER), reg(STACK_POINTER), num(quad.addr2.value.int_num));
                current_sp += quad.addr2.value.int_num;
            }
            else{
                add_offset_to_symbol(table, quad.addr1.value.name, current_function_scope, current_frame_offset);
                current_frame_offset += quad.addr2.value.int_num;
            }
            break;
        }
        case Q_ASSIGN: {
            int current_pointer;
            HashItem* temp_hash_item = search_item(table, quad.addr1.value.name, current_function_scope);
 
            if(temp_hash_item != NULL) {
                current_pointer = FRAME_POINTER;
            } else {
                current_pointer = 0;
            }
 
            int new_reg = get_new_register();
            mem_offset_t offset = get_symbol_offset(table, quad.addr1.value.name, current_function_scope);
 
            if(quad.addr2.type == NAME) {
                if(quad.addr3.type != VAZIO){

                    int base_reg = emit_vector_base_address(quad.addr2.value.name, current_function_scope);
 
                    int index_reg = get_new_register();
                    if (quad.addr3.type == INT_NUM) {
                        emit_operation(ASM_ADDI, reg(index_reg), reg(0), num(quad.addr3.value.int_num));
                    } else {
                        HashItem* index_hash_item = search_item(table, quad.addr3.value.name, current_function_scope);
                        int index_pointer = (index_hash_item != NULL) ? FRAME_POINTER : 0;
                        mem_offset_t index_offset = get_symbol_offset(table, quad.addr3.value.name, current_function_scope);
                        emit_operation(ASM_LW, reg(index_reg), reg(index_pointer), num(index_offset));
                    }
 
                    int addr_reg = get_new_register();
                    emit_operation(ASM_ADD, reg(addr_reg), reg(base_reg), reg(index_reg));
 
                    emit_operation(ASM_LW, reg(new_reg), reg(addr_reg), num(0));
                    emit_operation(ASM_SW, reg(new_reg), reg(current_pointer), num(offset));
                }
                else{
 
                    int source_pointer;
                    HashItem* source_hash_item = search_item(table, quad.addr2.value.name, current_function_scope);
 
                    if(source_hash_item != NULL) {
                        source_pointer = FRAME_POINTER;
                    } else {
                        source_pointer = 0;
                    }
 
                    mem_offset_t assign_offset = get_symbol_offset(table, quad.addr2.value.name, current_function_scope);
 
                    emit_operation(ASM_LW, reg(new_reg), reg(source_pointer), num(assign_offset));
                    emit_operation(ASM_SW, reg(new_reg), reg(current_pointer), num(offset));
                }
            } else {
 
                emit_operation(ASM_ADDI, reg(new_reg), reg(0), num(quad.addr2.value.int_num));
                emit_operation(ASM_SW, reg(new_reg), reg(current_pointer), num(offset));
            }
 
            break;
        }
        case Q_PARAM_VET: {

            HashItem* param_item = search_item(table, quad.addr1.value.name, current_function_scope);
            if(param_item == NULL) {
                insert_item(table, quad.addr1.value.name, current_function_scope, 0, VAR, INT_EXP, 2);
            } else {

                param_item->qnt_param = 2;
            }

            add_offset_to_symbol(table, quad.addr1.value.name, current_function_scope, current_frame_offset);
            current_frame_offset++;
            break;
        }
        case Q_ASSIGN_VET: {
            int base_reg = emit_vector_base_address(quad.addr1.value.name, current_function_scope);

            int index_reg = get_new_register();
            if (quad.addr2.type == INT_NUM) {
                emit_operation(ASM_ADDI, reg(index_reg), reg(0), num(quad.addr2.value.int_num));
            } else {
                HashItem* idx_item = search_item(table, quad.addr2.value.name, current_function_scope);
                int idx_base = (idx_item != NULL) ? FRAME_POINTER : 0;
                mem_offset_t idx_offset = get_symbol_offset(table, quad.addr2.value.name, current_function_scope);
                emit_operation(ASM_LW, reg(index_reg), reg(idx_base), num(idx_offset));
            }

            int addr_reg = get_new_register();
            emit_operation(ASM_ADD, reg(addr_reg), reg(base_reg), reg(index_reg));

            int val_reg = get_new_register();
            if (quad.addr3.type == INT_NUM) {
                emit_operation(ASM_ADDI, reg(val_reg), reg(0), num(quad.addr3.value.int_num));
            } else {
                HashItem* val_item = search_item(table, quad.addr3.value.name, current_function_scope);
                int val_base = (val_item != NULL) ? FRAME_POINTER : 0;
                mem_offset_t val_offset = get_symbol_offset(table, quad.addr3.value.name, current_function_scope);
                emit_operation(ASM_LW, reg(val_reg), reg(val_base), num(val_offset));
            }

            emit_operation(ASM_SW, reg(val_reg), reg(addr_reg), num(0));
            break;
        }
        case Q_SOMA:
            emit_binary_op(ASM_ADD, quad);
            break;
        case Q_SUB:
            emit_binary_op(ASM_SUB, quad);
            break;
        case Q_MULT:
            emit_binary_op(ASM_MULT, quad);
            break;
        case Q_DIV:
            emit_binary_op(ASM_DIV, quad);
            break;
        case Q_IGUAL:
            emit_binary_op(ASM_EQ, quad);
            break;
        case Q_DIFF:
            emit_binary_op(ASM_NEQ, quad);
            break;
        case Q_MAIOR:
            emit_binary_op(ASM_GT, quad);
            break;
        case Q_MAIOR_Q:
            emit_binary_op(ASM_GTE, quad);
            break;
        case Q_MENOR:
            emit_binary_op(ASM_LT, quad);
            break;
        case Q_MENOR_Q:
            emit_binary_op(ASM_LTE, quad);
            break;
        case Q_IF:
            int temp_reg1 = get_new_register();

            if (quad.addr1.type == INT_NUM) {

                emit_operation(ASM_ADDI, reg(temp_reg1), reg(0), num(quad.addr1.value.int_num));
            } else {

                mem_offset_t offset = get_symbol_offset(table, quad.addr1.value.name, current_function_scope);
                emit_operation(ASM_LW, reg(temp_reg1), reg(FRAME_POINTER), num(offset));
            }

            int label_number = quad.addr2.value.int_num;

            emit_operation(ASM_BEQ, reg(temp_reg1), reg(0), num(label_number));
            break;
        case Q_RETURN: {

            int value_reg = get_new_register();
            if (quad.addr1.type == INT_NUM) {
                emit_operation(ASM_ADDI, reg(value_reg), reg(0), num(quad.addr1.value.int_num));
            } else {
                mem_offset_t value_offset = get_symbol_offset(table, quad.addr1.value.name, current_function_scope);
                emit_operation(ASM_LW, reg(value_reg), reg(FRAME_POINTER), num(value_offset));
            }

            int dest_addr_reg = get_new_register();
            emit_operation(ASM_LW, reg(dest_addr_reg), reg(FRAME_POINTER), num(2));
            operation_list[current_list_position-1].msg = strdup("return");

            emit_operation(ASM_SW, reg(value_reg), reg(dest_addr_reg), num(0));

            int return_addr_reg = get_new_register();
            emit_operation(ASM_LW, reg(return_addr_reg), reg(FRAME_POINTER), num(1));

            int saved_fp_reg = get_new_register();
            emit_operation(ASM_LW, reg(saved_fp_reg), reg(FRAME_POINTER), num(0));

            emit_operation(ASM_ADD, reg(STACK_POINTER), reg(FRAME_POINTER), reg(0));
            current_sp = current_fp;

            emit_operation(ASM_ADD, reg(FRAME_POINTER), reg(saved_fp_reg), reg(0));

            emit_operation(ASM_JR, reg(return_addr_reg), num(0), num(0));

            break;
        }

        case Q_FUNCLABEL: {

            add_offset_to_symbol(table, quad.addr1.value.name, "global", current_list_position);

            emit_operation(ASM_ADD, reg(FRAME_POINTER), reg(STACK_POINTER), reg(0));
            operation_list[current_list_position-1].msg = strdup(quad.addr1.value.name);

            current_fp = current_sp;

            emit_operation(ASM_SW, reg(RETURN_ADDRESS_POINTER), reg(FRAME_POINTER), num(1));

            current_sp += 3;
            current_func_prologue_idx = current_list_position;
            emit_operation(ASM_ADDI, reg(STACK_POINTER), reg(STACK_POINTER), num(3));

            current_frame_offset = 3;

            memset(current_function_scope, 0, SCOPE_NAME_SIZE);
            strcpy(current_function_scope, quad.addr1.value.name);
            break;
        }

        case Q_FUNCEND: {
            if (current_func_prologue_idx != -1) {
                operation_list[current_func_prologue_idx].operands[2] = num(current_frame_offset);
                current_func_prologue_idx = -1;
            }

            int is_main = (strcmp(quad.addr1.value.name, "main") == 0);

            if (is_main) {

                emit_operation(ASM_ADD, reg(STACK_POINTER), reg(FRAME_POINTER), reg(0));
                current_sp = current_fp;
            } else {
                int return_addr_reg = get_new_register();
                emit_operation(ASM_LW, reg(return_addr_reg), reg(FRAME_POINTER), num(1));

                int saved_fp_reg = get_new_register();
                emit_operation(ASM_LW, reg(saved_fp_reg), reg(FRAME_POINTER), num(0));

                emit_operation(ASM_ADD, reg(STACK_POINTER), reg(FRAME_POINTER), reg(0));
                current_sp = current_fp;

                emit_operation(ASM_ADD, reg(FRAME_POINTER), reg(saved_fp_reg), reg(0));
                emit_operation(ASM_JR, reg(return_addr_reg), num(0), num(0));
            }

            current_frame_offset = 0;
            memset(current_function_scope, 0, SCOPE_NAME_SIZE);
            strcpy(current_function_scope, "global");
            break;
        }

        default:
            break;
    }
}

static void emit_input_function()
{
    struct ADDR func_name = (struct ADDR){.type = NAME, .value.name = "input"};
    struct ADDR dash = (struct ADDR){.type = NAME, .value.name = "-"};
    struct Quadrupla quad;

    quad = (struct Quadrupla){.type = Q_FUNCLABEL, .addr1 = func_name, .addr2 = dash, .addr3 = dash};
    translate_quad(quad);

    emit_operation(ASM_ADDI, reg(3), reg(0), num(2));

    emit_operation(ASM_NOP, reg(0), reg(0), reg(0));

    emit_operation(ASM_ADDI, reg(3), reg(0), num(0));

    int dest_addr_reg = get_new_register();
    emit_operation(ASM_LW, reg(dest_addr_reg), reg(FRAME_POINTER), num(2));

    emit_operation(ASM_SW, reg(2), reg(dest_addr_reg), num(0));

    int return_addr_reg = get_new_register();
    emit_operation(ASM_LW, reg(return_addr_reg), reg(FRAME_POINTER), num(1));

    int saved_fp_reg = get_new_register();
    emit_operation(ASM_LW, reg(saved_fp_reg), reg(FRAME_POINTER), num(0));

    emit_operation(ASM_ADD, reg(STACK_POINTER), reg(FRAME_POINTER), reg(0));
    current_sp = current_fp;

    emit_operation(ASM_ADD, reg(FRAME_POINTER), reg(saved_fp_reg), reg(0));

    emit_operation(ASM_JR, reg(return_addr_reg), num(0), num(0));

    if (current_func_prologue_idx != -1) {
        operation_list[current_func_prologue_idx].operands[2] = num(current_frame_offset);
        current_func_prologue_idx = -1;
    }

    current_frame_offset = 0;
    memset(current_function_scope, 0, SCOPE_NAME_SIZE);
    strcpy(current_function_scope, "global");
}

static void emit_output_function()
{
    struct ADDR func_name = (struct ADDR){.type = NAME, .value.name = "output"};
    struct ADDR dash = (struct ADDR){.type = NAME, .value.name = "-"};
    struct Quadrupla quad;

    quad = (struct Quadrupla){.type = Q_FUNCLABEL, .addr1 = func_name, .addr2 = dash, .addr3 = dash};
    translate_quad(quad);

    struct ADDR param_name = (struct ADDR){.type = NAME, .value.name = "__output_param"};
    quad = (struct Quadrupla){.type = Q_INIT, .addr1 = param_name, .addr2 = dash, .addr3 = dash};
    translate_quad(quad);

    emit_operation(ASM_ADDI, reg(3), reg(0), num(5));

    int value_reg = get_new_register();
    mem_offset_t param_offset = get_symbol_offset(table, "__output_param", "output");
    emit_operation(ASM_LW, reg(value_reg), reg(FRAME_POINTER), num(param_offset));

    emit_operation(ASM_ADD, reg(1), reg(value_reg), reg(0));

    int return_addr_reg = get_new_register();
    emit_operation(ASM_LW, reg(return_addr_reg), reg(FRAME_POINTER), num(1));

    int saved_fp_reg = get_new_register();
    emit_operation(ASM_LW, reg(saved_fp_reg), reg(FRAME_POINTER), num(0));

    emit_operation(ASM_ADD, reg(STACK_POINTER), reg(FRAME_POINTER), reg(0));
    current_sp = current_fp;

    emit_operation(ASM_ADD, reg(FRAME_POINTER), reg(saved_fp_reg), reg(0));

    emit_operation(ASM_JR, reg(return_addr_reg), num(0), num(0));

    if (current_func_prologue_idx != -1) {
        operation_list[current_func_prologue_idx].operands[2] = num(current_frame_offset);
        current_func_prologue_idx = -1;
    }

    current_frame_offset = 0;
    memset(current_function_scope, 0, SCOPE_NAME_SIZE);
    strcpy(current_function_scope, "global");
}

void asm_gen(struct QuadrupleList* list)
{
    printf("\n\nAsm generator using %d Registers\n\n", MAX_NUM_REGISTER);

    for (int i = 0; i < 32; i++) {
        label_frame_offset[i] = -1;
    }
    struct ListNode* global_node = list->list;
    while (global_node != NULL && global_node->quad.type != Q_FUNCLABEL) {
        translate_quad(global_node->quad);
        global_node = global_node->next;
    }

    int startup_jump_index = current_list_position;
    emit_operation(ASM_JUMP, num(0), num(0), num(0));

    emit_output_function();
    emit_input_function();

    label_position[30] = current_list_position;
    operation_list[startup_jump_index].operands[2] = num(30);
    struct ListNode* node = global_node;
    int jump_inserted = 0;
    int jump_index = -1;

    while(node != NULL){
        if (!jump_inserted && node->quad.type == Q_FUNCLABEL) {

            jump_index = current_list_position;
            emit_operation(ASM_JUMP, num(0), num(0), num(0));
            jump_inserted = 1;
        }

        translate_quad(node->quad);
        node = node->next;
    }

    if (jump_index >= 0) {
        mem_offset_t main_entry = get_symbol_offset(table, "main", "global");
        label_position[31] = main_entry;
        operation_list[jump_index].operands[2] = num(31);
    }
    emit_operation(ASM_HALT, reg(0), reg(0), reg(0));
}