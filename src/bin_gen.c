#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bin_gen.h"
#include "asm_gen.h"

/* mesmas funcoes de utils.c, reaproveitadas para a versao comentada */
extern void print_asm_operation(AsmOperation op);
extern const char* asm_operation_to_string(ASM_OPERATION op);

/* ---------------------------------------------------------------------
 * Helpers de codificacao de bits
 * --------------------------------------------------------------------- */

/* Escreve 'value' (truncado/maskado para 'width' bits) em 'dest',
 * começando em dest[*pos], avançando *pos por 'width' caracteres.
 * Não escreve o terminador '\0' -- quem chama cuida disso no final. */
static void put_bits(char* dest, int* pos, int value, int width) {
    unsigned int mask = (width >= 32) ? 0xFFFFFFFFu : ((1u << width) - 1u);
    unsigned int v = ((unsigned int) value) & mask;
    for (int i = width - 1; i >= 0; i--) {
        dest[*pos + (width - 1 - i)] = ((v >> i) & 1u) ? '1' : '0';
    }
    *pos += width;
}

static const int OPCODE_TYPE_R = 0x00;
static const int OPCODE_JUMP   = 0x01;
static const int OPCODE_JR     = 0x02;
static const int OPCODE_BEQ    = 0x03;
static const int OPCODE_JAL    = 0x04;
static const int OPCODE_LW     = 0x05;
static const int OPCODE_SW     = 0x06;
static const int OPCODE_ADDI   = 0x0A;
static const int OPCODE_SUBI   = 0x0B;
static const int OPCODE_HALT   = 0b111111;

/* select da ULA (ver ULA.v) -- so para as operacoes tipo R suportadas */
static int alu_select_for(ASM_OPERATION op) {
    switch (op) {
        case ASM_ADD:  return 0;
        case ASM_SUB:  return 1;
        case ASM_MULT: return 2;
        case ASM_DIV:  return 3;
        /* 4 = SHL, nao usado por nenhuma quadrupla ainda */
        case ASM_AND:  return 5;
        case ASM_OR:   return 6;
        case ASM_EQ:   return 7;
        case ASM_NEQ:  return 8;
        case ASM_GT:   return 9;
        case ASM_GTE:  return 10;
        case ASM_LT:   return 11;
        case ASM_LTE:  return 12;
        default:       return 0;
    }
}

static int is_type_r(ASM_OPERATION op) {
    switch (op) {
        case ASM_ADD: case ASM_SUB: case ASM_MULT: case ASM_DIV:
        case ASM_AND: case ASM_OR:
        case ASM_EQ: case ASM_NEQ: case ASM_GT: case ASM_GTE:
        case ASM_LT: case ASM_LTE:
            return 1;
        default:
            return 0;
    }
}

/* ---------------------------------------------------------------------
 * Traducao de uma AsmOperation para 32 bits
 * --------------------------------------------------------------------- */

void bin_gen_translate_operation(AsmOperation op, int label_position[], int call_target[], char* out_bits) {
    int pos = 0;

    if (is_type_r(op.asm_operation_type)) {
        /* Tipo R: dest=operands[0], src1=operands[1], src2=operands[2]
         * (mesma convencao usada em emit_binary_op/ Q_FUNCLABEL/etc,
         * onde o primeiro operando emitido e sempre o destino). */
        int select = alu_select_for(op.asm_operation_type);
        put_bits(out_bits, &pos, OPCODE_TYPE_R, 6);
        put_bits(out_bits, &pos, op.operands[1].operand, 5); /* src1 */
        put_bits(out_bits, &pos, op.operands[2].operand, 5); /* src2 */
        put_bits(out_bits, &pos, op.operands[0].operand, 5); /* dest */
        put_bits(out_bits, &pos, 0, 5);                       /* shiftAmnt (nao usado) */
        put_bits(out_bits, &pos, select, 6);
    }
    else if (op.asm_operation_type == ASM_ADDI || op.asm_operation_type == ASM_SUBI) {
        /* ADDI/SUBI dest, src1, imediato -- igual ao formato impresso
         * por print_asm_operation: operands[0]=dest, operands[1]=src1,
         * operands[2]=imediato. */
        int opcode = (op.asm_operation_type == ASM_ADDI) ? OPCODE_ADDI : OPCODE_SUBI;
        put_bits(out_bits, &pos, opcode, 6);
        put_bits(out_bits, &pos, op.operands[1].operand, 5); /* src1 */
        put_bits(out_bits, &pos, op.operands[0].operand, 5); /* dest */
        put_bits(out_bits, &pos, op.operands[2].operand, 16); /* imediato */
    }
    else if (op.asm_operation_type == ASM_LW) {
        /* LW dest, base(offset) -- operands[0]=dest, operands[1]=base,
         * operands[2]=offset (mesma convencao de Q_ASSIGN/emit_binary_op,
         * todas emitem LW com (dest, base, offset) nessa ordem). */
        put_bits(out_bits, &pos, OPCODE_LW, 6);
        put_bits(out_bits, &pos, op.operands[0].operand, 5); /* dest */
        put_bits(out_bits, &pos, op.operands[1].operand, 5); /* base */
        put_bits(out_bits, &pos, op.operands[2].operand, 16); /* offset */
    }
    else if (op.asm_operation_type == ASM_SW) {
        /* SW dado, base(offset) -- operands[0]=dado, operands[1]=base,
         * operands[2]=offset. No formato Store do hardware, o campo
         * [25:21] (logo depois do opcode) e o DADO, e [20:16] e a
         * BASE -- ordem invertida em relacao ao LW. */
        put_bits(out_bits, &pos, OPCODE_SW, 6);
        put_bits(out_bits, &pos, op.operands[0].operand, 5); /* dado */
        put_bits(out_bits, &pos, op.operands[1].operand, 5); /* base */
        put_bits(out_bits, &pos, op.operands[2].operand, 16); /* offset */
    }
    else if (op.asm_operation_type == ASM_BEQ || op.asm_operation_type == ASM_BNE) {
        /* BEQ/BNE src1, src2, label -- operands[2] e um INDICE em
         * label_position[], resolvido aqui para o endereco final
         * (mesma logica de print_asm_operation). */
        int target = label_position[op.operands[2].operand];
        put_bits(out_bits, &pos, OPCODE_BEQ, 6);
        put_bits(out_bits, &pos, op.operands[0].operand, 5); /* src1 */
        put_bits(out_bits, &pos, op.operands[1].operand, 5); /* src2 */
        put_bits(out_bits, &pos, target, 16);
    }
    else if (op.asm_operation_type == ASM_JUMP) {
        /* JUMP label -- operands[2] e indice em label_position[]. */
        int target = label_position[op.operands[2].operand];
        put_bits(out_bits, &pos, OPCODE_JUMP, 6);
        put_bits(out_bits, &pos, target, 26);
    }
    else if (op.asm_operation_type == ASM_JAL) {
        /* JAL funcao -- operands[2] e indice em call_target[] (NAO em
         * label_position[] -- cada chamada tem seu proprio slot, ver
         * asm_gen.c). */
        int target = call_target[op.operands[2].operand];
        put_bits(out_bits, &pos, OPCODE_JAL, 6);
        put_bits(out_bits, &pos, target, 26);
    }
    else if (op.asm_operation_type == ASM_JR) {
        /* JR Rsrc -- operands[0] e o REGISTRADOR com o endereco de
         * retorno (nao um label). Vai nos 5 bits menos significativos
         * da instrucao (ver unidade_de_controle.v: src1=instrucao[4:0]).
         * 6 (opcode) + 21 (nao usado) + 5 (src1) = 32 bits. */
        put_bits(out_bits, &pos, OPCODE_JR, 6);
        put_bits(out_bits, &pos, 0, 21); /* bits [25:5] nao usados pelo JR */
        put_bits(out_bits, &pos, op.operands[0].operand, 5); /* src1, bits [4:0] */
    }
    else if (op.asm_operation_type == ASM_HALT) {
        /* HALT Rsrc -- operands[0] e o REGISTRADOR com o endereco de
         * retorno (nao um label). Vai nos 5 bits menos significativos
         * da instrucao (ver unidade_de_controle.v: src1=instrucao[4:0]).
         * 6 (opcode) + 21 (nao usado) + 5 (src1) = 32 bits. */
        put_bits(out_bits, &pos, OPCODE_HALT, 6);
        put_bits(out_bits, &pos, 0, 21); /* bits [25:5] nao usados pelo JR */
        put_bits(out_bits, &pos, op.operands[0].operand, 5); /* src1, bits [4:0] */
    }
    else {
        /* Operacao desconhecida/nao suportada -- preenche com zeros
         * (NOP seguro: ADD R0,R0,R0, ver opcode tipo R = 0). */
        put_bits(out_bits, &pos, 0, 32);
    }

    out_bits[BIN_GEN_WORD_BITS] = '\0';
}

/* ---------------------------------------------------------------------
 * Escrita dos arquivos de saida
 * --------------------------------------------------------------------- */

void bin_gen_write_clean(AsmOperation* operation_list, int count, int label_position[], int call_target[], FILE* output) {
    char bits[BIN_GEN_WORD_BITS + 1];

    for (int i = 0; i < count; i++) {
        bin_gen_translate_operation(operation_list[i], label_position, call_target, bits);
        fprintf(output, "%s\n", bits);
    }
}

/* Captura a saida de print_asm_operation (que imprime em stdout) como
 * string, para reaproveitar a MESMA logica de formatacao de texto que
 * o resto do compilador ja usa, em vez de duplicar a decisao de "qual
 * operando e base/offset/registrador" numa segunda funcao de
 * impressao. Evita ter duas fontes de verdade para o formato textual. */
static void capture_asm_text(AsmOperation op, char* buffer, size_t buffer_size) {
    FILE* mem_stream = fmemopen(buffer, buffer_size, "w");
    if (mem_stream == NULL) {
        snprintf(buffer, buffer_size, "<erro: nao foi possivel capturar>");
        return;
    }

    FILE* original_stdout = stdout;
    stdout = mem_stream;
    print_asm_operation(op);
    stdout = original_stdout;

    fflush(mem_stream);
    fclose(mem_stream);

    /* print_asm_operation termina com '\n'; remove para a linha de
     * comentario ficar limpa. */
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
}

void bin_gen_write_commented(AsmOperation* operation_list, int count, int label_position[], int call_target[], FILE* output) {
    char bits[BIN_GEN_WORD_BITS + 1];
    char text[256];

    fprintf(output,
        "; ============================================================\n"
        "; Programa C- traduzido para binario (32 bits por instrucao)\n"
        "; Arquitetura: ver unidade_de_controle.v / ULA.v / pc.v / registradores.v\n"
        "; Convencao de registradores: R0=zero fixo, R1=output, R2=input,\n"
        "; R3=config, FP=R29, SP=R30, RA=R31\n"
        "; Endereco de instrucao = indice da linha (0-indexado), usado pelo PC\n"
        "; ============================================================\n\n"
    );

    for (int i = 0; i < count; i++) {
        bin_gen_translate_operation(operation_list[i], label_position, call_target, bits);
        capture_asm_text(operation_list[i], text, sizeof(text));

        unsigned long value = strtoul(bits, NULL, 2);

        fprintf(output, "; Linha %02d | ASM: %s\n", i, text);
        fprintf(output, "%c%c%c%c %c%c%c%c %c%c%c%c %c%c%c%c %c%c%c%c %c%c%c%c %c%c%c%c %c%c%c%c   ; 0x%08lX\n\n",
            bits[0], bits[1], bits[2], bits[3],
            bits[4], bits[5], bits[6], bits[7],
            bits[8], bits[9], bits[10], bits[11],
            bits[12], bits[13], bits[14], bits[15],
            bits[16], bits[17], bits[18], bits[19],
            bits[20], bits[21], bits[22], bits[23],
            bits[24], bits[25], bits[26], bits[27],
            bits[28], bits[29], bits[30], bits[31],
            value
        );
    }
}