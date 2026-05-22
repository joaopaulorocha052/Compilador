#ifndef _UTILS_
#define _UTILS_

#include "code_gen.h"



struct QuadrupleList* init_list();
int add_quadruple_to_list(struct QuadrupleList* list, struct Quadrupla quad);
void print_list(struct QuadrupleList* list);
void print_quad(struct Quadrupla quad);
const char* quadruple_type_to_string(QUADRUPLE_TYPES type);
int check_address_is_num(char* value);
#endif