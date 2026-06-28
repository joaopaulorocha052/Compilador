#ifndef BIN_GEN_H
#define BIN_GEN_H

#include <stdio.h>
#include "asm_gen.h"

/*
 * bin_gen.c -- Traduz a lista de AsmOperation ja montada por asm_gen()
 * (operation_list[], current_list_position) direto para binario de 32
 * bits por instrucao, sem passar por texto.
 *
 * Formatos de instrucao (confirmados linha a linha contra
 * unidade_de_controle.v):
 *
 *   Tipo R   (opcode 000000): [31:26]=opcode [25:21]=src1 [20:16]=src2
 *                             [15:11]=dest [10:6]=shiftAmnt [5:0]=select
 *   JUMP     (opcode 000001): [31:26]=opcode [25:0]=end_jump
 *   JR       (opcode 000010): [31:26]=opcode [4:0]=src1 (endereco no registrador)
 *   BEQ      (opcode 000011): [31:26]=opcode [25:21]=src1 [20:16]=src2 [15:0]=end_jump
 *   JAL      (opcode 000100): [31:26]=opcode [25:0]=end_jump (PC+1 salvo em R24 pelo hardware)
 *   LW       (opcode 000101): [31:26]=opcode [25:21]=dest [20:16]=src1(base) [15:0]=imediato
 *   SW       (opcode 000110): [31:26]=opcode [25:21]=src2(dado) [20:16]=src1(base) [15:0]=imediato
 *   ADDI     (opcode 001010): [31:26]=opcode [25:21]=src1 [20:16]=dest [15:0]=imediato
 *   SUBI     (opcode 001011): [31:26]=opcode [25:21]=src1 [20:16]=dest [15:0]=imediato
 *
 * select da ULA (5 bits funcionais, ver ULA.v):
 *   ADD=0 SUB=1 MULT=2 DIV=3 SHL=4 AND=5 OR=6 EQ=7 NEQ=8 GT=9 GTE=10 LT=11 LTE=12
 *
 * Registradores especiais: R0=zero fixo R1=output R2=input R3=config
 * FP=R29 SP=R30 RA(RETURN_ADDRESS_POINTER)=R31
 */

#define BIN_GEN_WORD_BITS 32

/*
 * Traduz UMA AsmOperation para uma palavra de 32 bits, escrita em
 * out_bits como string de caracteres '0'/'1' (out_bits deve ter pelo
 * menos 33 bytes, para o terminador '\0').
 *
 * label_position e call_target sao os mesmos arrays usados por
 * print_asm_operation em utils.c -- precisam estar com a resolucao já
 * completa (ou seja, chamar isso só depois que asm_gen() já processou
 * toda a lista de quádruplas).
 */
void bin_gen_translate_operation(AsmOperation op, int label_position[], int call_target[], char* out_bits);

/*
 * Percorre operation_list[0..count-1] e escreve o binario LIMPO (só os
 * bits, uma instrucao por linha, sem comentarios) em 'output'. Formato
 * compativel com $readmemb / memoria_instrucao_arquivo.v.
 */
void bin_gen_write_clean(AsmOperation* operation_list, int count, int label_position[], int call_target[], FILE* output);

/*
 * Percorre operation_list[0..count-1] e escreve o binario COMENTADO em
 * 'output': para cada instrucao, imprime o indice/endereco, os bits
 * agrupados em nibbles de 4, e o valor em hexadecimal. Não tem acesso
 * ao texto assembly original (trabalha direto da struct), então usa
 * asm_operation_to_string/print_operand (utils.c) para reconstruir uma
 * representação legível equivalente.
 */
void bin_gen_write_commented(AsmOperation* operation_list, int count, int label_position[], int call_target[], FILE* output);

#endif