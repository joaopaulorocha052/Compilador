#ifndef BIN_GEN_H
#define BIN_GEN_H

#include <stdio.h>
#include "asm_gen.h"


#define BIN_GEN_WORD_BITS 32

void bin_gen_translate_operation(AsmOperation op, int label_position[], int call_target[], char* out_bits);

void bin_gen_write_clean(AsmOperation* operation_list, int count, int label_position[], int call_target[], FILE* output);

void bin_gen_write_commented(AsmOperation* operation_list, int count, int label_position[], int call_target[], FILE* output);

#endif