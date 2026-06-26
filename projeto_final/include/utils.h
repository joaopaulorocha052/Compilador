#ifndef _UTILS_
#define _UTILS_

#include "code_gen.h"
#include "asm_gen.h"


#define SCOPE_NAME_SIZE 100




struct QuadrupleList* init_list();
int add_quadruple_to_list(struct QuadrupleList* list, struct Quadrupla quad);
void print_list(struct QuadrupleList* list);
void print_quad(struct Quadrupla quad);
const char* quadruple_type_to_string(QUADRUPLE_TYPES type);
int check_address_is_num(char* value);
const char* asm_operation_to_string(ASM_OPERATION op);
void print_operand(AsmOperand op);
void print_asm_operation(AsmOperation op);
#endif