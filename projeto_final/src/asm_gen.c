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

/*
 * current_sp acompanha a posição REAL da pilha (registrador SP físico).
 * Ela só cresce (Q_INIT/Q_INITVET avançam) e só diminui no epílogo de
 * uma função (Q_FUNCEND), nunca é "resetada" arbitrariamente -- senão
 * duas funções diferentes acabariam reaproveitando os mesmos endereços
 * de pilha mesmo que uma ainda esteja "viva" (ex: recursão, ou uma
 * função que chama outra).
 *
 * current_local_offset é só para variáveis GLOBAIS, que vivem em
 * endereços fixos a partir de 0(R0) e nunca são desalocadas.
 *
 * current_frame_offset é o offset relativo ao FP da função atual.
 * Esse sim é zerado a cada Q_FUNCLABEL, porque cada função tem seu
 * próprio frame e suas variáveis locais sempre começam em offset 0
 * relativo ao FP daquela chamada específica.
 */
int current_sp = 0;
int current_fp = 0;
int current_local_offset = 0;
int current_frame_offset = 0;

int current_line = 0;

int current_list_position;

int get_new_register()
{
    static int returned_register = 4;

    // Registradores 0-3 são especiais e reservados pelo hardware
    // (R0=zero fixo, R1=output, R2=input, R3=config -- ver registradores.v),
    // e 29-31 são FP/SP/RA (ver asm_gen.h). O "pool" de registradores de uso
    // livre para temporários é só 4..28. Por isso o reset tem que voltar
    // para 4, e não para 0 -- senão um programa com operações suficientes
    // em sequência eventualmente aloca R0/R1/R2/R3 como destino de
    // resultado, e essas escritas se perdem (R0 é fixo em zero) ou
    // corrompem registradores de I/O.
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

/*
 * Traduz uma operação binária genérica (aritmética ou relacional):
 *   addr1 = addr2 <asm_op> addr3
 *
 * addr2/addr3 podem ser constante (INT_NUM) ou variável (NAME); nos dois
 * casos o valor termina num registrador novo. addr1 é sempre o destino
 * (um temporário "_tN" criado em OP_NODE), que por construção do
 * code_gen.c só existe dentro do corpo de uma função -- nunca no escopo
 * global -- então o destino sempre usa FRAME_POINTER como base, igual
 * o Q_IGUAL original já assumia.
 *
 * Retorna o número de instruções emitidas, para quem chama incrementar
 * current_line corretamente.
 */
static int emit_binary_op(ASM_OPERATION asm_op, struct Quadrupla quad)
{
    int reg1 = get_new_register();
    int reg2 = get_new_register();
    int result_reg = get_new_register();
    int count = 0;

    if (quad.addr2.type == INT_NUM) {
        emit_operation(ASM_ADDI, reg(reg1), reg(0), num(quad.addr2.value.int_num));
    } else {
        mem_offset_t offset1 = get_symbol_offset(table, quad.addr2.value.name, current_function_scope);
        emit_operation(ASM_LW, reg(reg1), reg(FRAME_POINTER), num(offset1));
    }
    count++;

    if (quad.addr3.type == INT_NUM) {
        emit_operation(ASM_ADDI, reg(reg2), reg(0), num(quad.addr3.value.int_num));
    } else {
        mem_offset_t offset2 = get_symbol_offset(table, quad.addr3.value.name, current_function_scope);
        emit_operation(ASM_LW, reg(reg2), reg(FRAME_POINTER), num(offset2));
    }
    count++;

    emit_operation(asm_op, reg(result_reg), reg(reg1), reg(reg2));
    count++;

    mem_offset_t dest_offset = get_symbol_offset(table, quad.addr1.value.name, current_function_scope);
    emit_operation(ASM_SW, reg(result_reg), reg(FRAME_POINTER), num(dest_offset));
    count++;

    return count;
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
            // Garante que o símbolo existe na tabela antes de tentar
            // gravar um offset nele. Variáveis declaradas pelo usuário
            // já foram inseridas na fase sintática (sintatico.y), então
            // search_item já encontra e o insert_item abaixo nem executa.
            // Mas temporários gerados pelo compilador (_t0, _t1, ...)
            // nunca passam pela fase sintática -- são criados direto em
            // code_gen.c -- então, sem isso, eles nunca entrariam na
            // tabela e add_offset_to_symbol não teria em que gravar o
            // offset (ela só atualiza um item que já existe).
            if(search_item(table, quad.addr1.value.name, current_function_scope) == NULL) {
                insert_item(table, quad.addr1.value.name, current_function_scope, 0, VAR, INT_EXP, 0);
            }

            if(strcmp(current_function_scope, "global") == 0)
            {
                // Globais: endereço fixo a partir de 0(R0), nunca desalocado.
                add_offset_to_symbol(table, quad.addr1.value.name, current_function_scope, current_local_offset);
                current_local_offset++;
            }
            else{
                // Locais: offset relativo ao FP da função atual.
                add_offset_to_symbol(table, quad.addr1.value.name, current_function_scope, current_frame_offset);
                current_frame_offset++;
            }
            // SP avança de qualquer forma -- é ele que efetivamente reserva
            // a palavra de memória, seja ela global (no fundo da pilha,
            // antes de qualquer função rodar) ou local (dentro do frame
            // da função atual).
            emit_operation(ASM_ADDI, reg(STACK_POINTER), reg(STACK_POINTER), num(1));
            current_sp++;
            current_line++;
            break;

        case Q_INITVET:
            if(search_item(table, quad.addr1.value.name, current_function_scope) == NULL) {
                insert_item(table, quad.addr1.value.name, current_function_scope, 0, VAR, INT_EXP, 0);
            }

            if(strcmp(current_function_scope, "global") == 0)
            {
                add_offset_to_symbol(table, quad.addr1.value.name, current_function_scope, current_local_offset);
                current_local_offset += quad.addr2.value.int_num;
            }
            else{
                add_offset_to_symbol(table, quad.addr1.value.name, current_function_scope, current_frame_offset);
                current_frame_offset += quad.addr2.value.int_num;
            }
            emit_operation(ASM_ADDI, reg(STACK_POINTER), reg(STACK_POINTER), num(quad.addr2.value.int_num));
            current_sp += quad.addr2.value.int_num;
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
        // ---------------------------------------------------------------
        // Operações binárias (aritméticas e relacionais): todas seguem o
        // mesmo padrão -- carregar os dois operandos, aplicar a ULA com
        // o select correspondente, e gravar o resultado no destino.
        // ---------------------------------------------------------------
        case Q_SOMA:
            current_line += emit_binary_op(ASM_ADD, quad);
            break;
        case Q_SUB:
            current_line += emit_binary_op(ASM_SUB, quad);
            break;
        case Q_MULT:
            current_line += emit_binary_op(ASM_MULT, quad);
            break;
        case Q_DIV:
            current_line += emit_binary_op(ASM_DIV, quad);
            break;
        case Q_IGUAL:
            current_line += emit_binary_op(ASM_EQ, quad);
            break;
        case Q_DIFF:
            current_line += emit_binary_op(ASM_NEQ, quad);
            break;
        case Q_MAIOR:
            current_line += emit_binary_op(ASM_GT, quad);
            break;
        case Q_MAIOR_Q:
            current_line += emit_binary_op(ASM_GTE, quad);
            break;
        case Q_MENOR:
            current_line += emit_binary_op(ASM_LT, quad);
            break;
        case Q_MENOR_Q:
            current_line += emit_binary_op(ASM_LTE, quad);
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
                emit_operation(ASM_LW, reg(temp_reg1), reg(FRAME_POINTER), num(offset));
            }

            // ---------------------------------------------------------
            // PASSO 2: Realizar o salto (Branch)
            // ---------------------------------------------------------
            // O addr2 guarda o NÚMERO do label (ex: "1", "2"), não a
            // posição final resolvida. Assim como o Q_GOTO faz com o
            // JUMP, guardamos aqui o número do label cru -- é o
            // print_asm_operation (em utils.c) quem resolve
            // label_position[] na hora de IMPRIMIR, depois que a lista
            // inteira de quádruplas já foi percorrida e todo
            // label_position[] já está completo.
            //
            // Resolver aqui dentro (como o código fazia antes) quebra
            // qualquer salto para frente (forward jump): no momento em
            // que o Q_IF é processado, o label de destino pode ainda
            // não ter sido visto (Q_LABEL correspondente vem depois na
            // lista), então label_position[label] ainda não tem o
            // valor certo. Isso é exatamente o caso de um "while", onde
            // o Q_IF do topo do loop salta para um label que só é
            // definido no fim do laço.
            //
            // CORREÇÃO: a quádrupla Q_IF significa "se a condição for
            // FALSA, salte para a label" -- o bloco "then" (ou corpo,
            // no caso do while) vem imediatamente em sequência depois
            // do Q_IF (sem goto), e a label marca o destino do salto
            // quando a condição é falsa. Como R0 já é fixo em zero no
            // hardware, basta comparar contra ele direto.
            int label_number = quad.addr2.value.int_num;

            // Emite: BEQ temp_reg, R0, label_number (salta se condição == falso)
            // O 3º operando aqui é o NÚMERO do label, resolvido depois na impressão.
            emit_operation(ASM_BEQ, reg(temp_reg1), reg(0), num(label_number));
            current_line+=2;
            break;
        case Q_FUNCLABEL:
            // ---------------------------------------------------------
            // PRÓLOGO: o frame da nova função começa exatamente onde a
            // pilha está agora. FP = SP "congela" essa posição, e é a
            // partir dela que todo acesso a variável local da função
            // (offset(FP)) vai ser calculado.
            //
            // OBS: isso ainda NÃO salva o FP do chamador em memória.
            // Isso é responsabilidade do Q_CALL (que ainda não está
            // implementado) -- antes de chegar aqui, quem chama precisa
            // ter empilhado o FP antigo, porque assim que executarmos
            // este ADD, o valor anterior de FP é perdido caso não tenha
            // sido salvo. Para uma função só (ex: main, chamada uma
            // única vez, sem retorno para ninguém) isso ainda não importa.
            // ---------------------------------------------------------
            emit_operation(ASM_ADD, reg(FRAME_POINTER), reg(STACK_POINTER), reg(0));
            current_line++;

            current_fp = current_sp;
            current_frame_offset = 0;

            memset(current_function_scope, 0, SCOPE_NAME_SIZE);
            strcpy(current_function_scope, quad.addr1.value.name);
            break;
        
        case Q_FUNCEND:
            // ---------------------------------------------------------
            // EPÍLOGO: desaloca o frame inteiro de uma vez só, devolvendo
            // SP para o valor que tinha antes do prólogo (que é exatamente
            // o que está guardado em FP). Não importa quantas variáveis
            // locais ou temporários a função declarou -- elas todas somem
            // juntas aqui.
            //
            // OBS: assim como no prólogo, isto ainda não restaura o FP do
            // CHAMADOR nem faz JR de volta -- isso é trabalho do Q_RETURN/
            // Q_CALL, que vai precisar ter salvo o FP antigo em algum lugar
            // antes do prólogo acima sobrescrever o registrador.
            // ---------------------------------------------------------
            emit_operation(ASM_ADD, reg(STACK_POINTER), reg(FRAME_POINTER), reg(0));
            current_line++;

            current_sp = current_fp;
            current_frame_offset = 0;

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