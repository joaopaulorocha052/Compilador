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

/*
 * label_frame_offset[]/label_sp[] guardam, para cada label, o valor de
 * current_frame_offset/current_sp no MOMENTO em que o Q_LABEL
 * correspondente foi processado. Servem para detectar e corrigir um
 * problema real: um Q_GOTO que salta PARA TRÁS (ou seja, para um label
 * já processado -- o padrão de qualquer laço, como while) faz aquele
 * trecho de código EXECUTAR DE NOVO em runtime, repetindo qualquer
 * ADDI SP,SP,1 que exista dentro do corpo (reservando temporários a
 * cada iteração). current_frame_offset/current_sp, por serem
 * calculados em tempo de COMPILAÇÃO (uma única passada pelo código),
 * nunca contam essas repetições -- então, sem correção, SP real
 * (hardware) cresce sem limite a cada iteração de um laço, enquanto o
 * compilador segue assumindo um valor fixo. Isso desalinha qualquer
 * acesso a memória calculado depois do laço (variáveis locais, frames
 * de chamada de função, etc.).
 *
 * -1 sinaliza "label ainda não processado" (label 0 é um valor de
 * label válido, não pode ser usado como sentinela).
 */
int label_frame_offset[32];
int label_sp[32];

/*
 * call_target[] guarda o endereço de entrada resolvido de cada
 * Q_CALL/JAL individual, indexado por call_count (não por número de
 * label). É um array SEPARADO de label_position[] porque, diferente de
 * labels de if/while (que são únicos por construção, um por
 * if/while), o MESMO label_position[N] poderia ser reaproveitado por
 * várias quádruplas Q_CALL diferentes se elas compartilhassem índice
 * -- cada chamada de função precisa do seu próprio slot, já que a
 * resolução (impressão) só acontece depois que TODA a lista de
 * quádruplas foi traduzida, então qualquer slot compartilhado entre
 * chamadas diferentes teria seu valor sobrescrito pela última chamada
 * processada.
 */
int call_target[MAX_OPERATION];
int call_count = 0;

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

/*
 * Quando Q_PARAM é processado, current_function_scope ainda é o do
 * CHAMADOR (Q_PARAM é avaliado antes do Q_CALL, no escopo de quem
 * chama) -- mas o destino de cada parâmetro é o início do frame da
 * função CHAMADA, que ainda nem começou a existir. current_param_count
 * conta quantos parâmetros já foram empilhados desde o último Q_CALL.
 *
 * pending_param_regs[] guarda o REGISTRADOR onde Q_PARAM calculou o
 * valor de cada argumento -- a ESCRITA na memória não acontece em
 * Q_PARAM, e sim dentro do próprio Q_CALL (ver case Q_CALL). Motivo:
 * code_gen.c sempre emite, entre os Q_PARAM de uma chamada e o Q_CALL
 * correspondente, um Q_INIT extra (o do temporário "_tN" que vai
 * receber o valor de retorno -- ver FUNC_ACTV_NODE). Esse Q_INIT
 * avança o STACK_POINTER físico. Se Q_PARAM escrevesse imediatamente
 * usando o SP de SEU PRÓPRIO momento, o endereço gravado ficaria um
 * offset atrás de onde o frame da função chamada vai de fato nascer
 * (calculado pelo Q_CALL, DEPOIS desse Q_INIT extra já ter avançado o
 * SP) -- cada parâmetro chegaria deslocado, lendo o que deveria ser o
 * slot do endereço de retorno. Por isso a escrita real é adiada para
 * o Q_CALL, que já conhece o SP final correto.
 */
int current_param_count = 0;
int pending_param_regs[MAX_OPERATION];

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

    // Checagem de segurança: sem isso, um programa que gere mais
    // instruções do que MAX_OPERATION corrompe silenciosamente a
    // memória adjacente (call_target[], label_position[], ou qualquer
    // outra coisa alocada depois de operation_list[] no binário) --
    // foi exatamente isso que causou um bug muito confuso antes desta
    // checagem existir (instruções do FIM do programa "vazando" para
    // o COMEÇO do assembly impresso, sem nenhuma relação lógica
    // aparente com o bug real).
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

    // Cada operando (e o destino) pode ser local OU global -- cada um
    // precisa da sua PRÓPRIA verificação de escopo, igual Q_ASSIGN já
    // faz corretamente. Antes desta correção, esta função SEMPRE usava
    // FRAME_POINTER como base, o que produzia endereços errados para
    // qualquer variável global usada numa expressão aritmética (ex:
    // "globalA = globalA + i" lia 'globalA' do frame local, não do
    // endereço global real).
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
            label_position[quad.addr1.value.int_num] = current_line;
            // Só registra o snapshot na PRIMEIRA vez que este label é
            // processado (sentinela -1) -- um label só é definido uma
            // vez no código-fonte, então isso roda uma única vez por
            // label, sempre.
            if (label_frame_offset[quad.addr1.value.int_num] == -1) {
                label_frame_offset[quad.addr1.value.int_num] = current_frame_offset;
                label_sp[quad.addr1.value.int_num] = current_sp;
            }
            break;
        case Q_GOTO: {
            int target_label = quad.addr1.value.int_num;

            // Se o label de destino JÁ foi processado (snapshot != -1),
            // este Q_GOTO salta PARA TRÁS -- o padrão de qualquer laço
            // (while). Isso significa que, em runtime, o trecho entre
            // o Q_LABEL e este Q_GOTO vai EXECUTAR DE NOVO a cada
            // repetição, incluindo qualquer ADDI SP,SP,1 que reserve
            // temporários dentro do corpo -- sem correção, SP real
            // cresceria sem limite a cada iteração, mesmo que
            // current_sp (compilação) nunca reflita isso. Por isso,
            // antes do salto de volta, devolvemos exatamente o espaço
            // acumulado desde aquele label (current_frame_offset atual
            // menos o valor que tinha lá).
            if (label_frame_offset[target_label] != -1) {
                int allocated_since_label = current_frame_offset - label_frame_offset[target_label];
                if (allocated_since_label > 0) {
                    emit_operation(ASM_SUBI, reg(STACK_POINTER), reg(STACK_POINTER), num(allocated_since_label));
                    current_line++;
                    current_sp -= allocated_since_label;
                    current_frame_offset -= allocated_since_label;
                }
            }

            emit_operation(ASM_JUMP, num(0), num(0), num(target_label));
            current_line++;
            break;
        }
        case Q_PARAM: {
            // addr1 = valor do argumento (constante ou variável do
            // escopo do CHAMADOR), na ordem em que aparece na chamada.
            //
            // Aqui só CALCULAMOS o valor e guardamos o registrador em
            // pending_param_regs[] -- a ESCRITA na memória do novo
            // frame é feita pelo Q_CALL, depois que o Q_INIT do
            // temporário de retorno (emitido entre os Q_PARAM e o
            // Q_CALL por code_gen.c) já tiver avançado o SP, e o
            // Q_CALL souber o SP final de verdade (ver comentário de
            // pending_param_regs[] e do Q_CALL abaixo).
            int param_reg = get_new_register();

            if (quad.addr1.type == INT_NUM) {
                emit_operation(ASM_ADDI, reg(param_reg), reg(0), num(quad.addr1.value.int_num));
            } else {
                HashItem* arg_item = search_item(table, quad.addr1.value.name, current_function_scope);
                int arg_base = (arg_item != NULL) ? FRAME_POINTER : 0;
                mem_offset_t arg_offset = get_symbol_offset(table, quad.addr1.value.name, current_function_scope);
                emit_operation(ASM_LW, reg(param_reg), reg(arg_base), num(arg_offset));
            }
            current_line++;

            pending_param_regs[current_param_count] = param_reg;
            current_param_count++;
            break;
        }

        case Q_CALL: {
            // addr1 = nome da função, addr2 = nome da variável (no
            // escopo do CHAMADOR) que vai receber o valor de retorno.
            //
            // Layout do frame da função CHAMADA (todos os offsets
            // relativos ao FP dela, definido no Q_FUNCLABEL):
            //   offset 0 -> FP do chamador
            //   offset 1 -> endereço de retorno (RA)
            //   offset 2 -> ENDEREÇO ABSOLUTO onde a função deve
            //               gravar o valor de retorno (não um offset
            //               -- um endereço de memória já resolvido,
            //               calculado abaixo pelo CHAMADOR, que é quem
            //               sabe onde sua própria variável de destino
            //               vive). A função chamada só precisa ler
            //               esse endereço e usar como base de um SW
            //               com offset 0 -- não precisa saber nada
            //               sobre o frame do chamador além disso.

            // 1) Salva o FP do CHAMADOR (offset 0) -- precisa ser ANTES
            //    do JAL, pelo mesmo motivo de sempre: depois que o
            //    prólogo da função chamada rodar, FRAME_POINTER já
            //    aponta para o novo frame, e o valor antigo se perde
            //    se não foi salvo agora.
            //
            // CORREÇÃO DO BUG (retorno sobrescrevendo RAM[0]): antes,
            // este SW usava base=R0 e offset=current_sp (endereço
            // ABSOLUTO fixo, calculado em tempo de COMPILAÇÃO). Isso só
            // funciona se current_sp (estático) estiver perfeitamente
            // sincronizado com o STACK_POINTER físico (runtime) -- e
            // isso quebra quando um Q_IF salta para FORA de um while
            // (BEQ direto para o label de fim): esse caminho nunca
            // passa pelo ajuste de SP que só existe no Q_GOTO (volta do
            // loop), então o SP físico real sai do while maior do que
            // current_sp assume. Qualquer Q_CALL feito depois grava
            // offset 0/2 do novo frame num endereço desatualizado,
            // enquanto a função chamada usa o SP físico real (correto)
            // para seu próprio FP -- os dois endereços divergem, e como
            // a RAM começa zerada, o retorno acaba em RAM[0].
            //
            // SOLUÇÃO: usar STACK_POINTER (registrador FÍSICO) como
            // base, em vez de R0+current_sp (valor estático). O
            // hardware sempre sabe o SP real, eliminando a dependência
            // de current_sp estar sincronizado -- mesma lógica que
            // Q_INIT/Q_ASSIGN/Q_RETURN já usam com FRAME_POINTER.
            emit_operation(ASM_SW, reg(FRAME_POINTER), reg(STACK_POINTER), num(0));
            current_line++;

            // 1.5) Grava cada argumento (calculado pelos Q_PARAM
            //      anteriores, guardados em pending_param_regs[]) no
            //      offset correspondente do novo frame: o 1º argumento
            //      em offset 3 (mesmo offset onde o Q_INIT do 1º
            //      parâmetro formal, do lado da função, já o espera),
            //      o 2º em offset 4, etc. Só agora, aqui, é que
            //      STACK_POINTER reflete o início real do novo frame
            //      (ver comentário de pending_param_regs[] acima sobre
            //      por que isso não pode ser feito dentro do próprio
            //      Q_PARAM).
            for (int p = 0; p < current_param_count; p++) {
                emit_operation(ASM_SW, reg(pending_param_regs[p]), reg(STACK_POINTER), num(3 + p));
                current_line++;
            }
            current_param_count = 0;

            // 2) Calcula o ENDEREÇO ABSOLUTO da variável de destino
            //    (addr2), no escopo do chamador, e grava esse endereço
            //    no offset 2 do frame que está sendo construído.
            //
            //    Se a variável é global, o endereço já É o offset dela
            //    (base R0) -- não precisa de nenhuma conta, é só um
            //    número conhecido em tempo de compilação.
            //
            //    Se é local, o endereço é FP_chamador + offset(addr2).
            //    Como LW/SW só aceitam offset IMEDIATO (não registrador
            //    -- essa é exatamente a limitação que motivou esta
            //    reescrita), calculamos a SOMA em tempo de execução
            //    com uma instrução ADDI sobre o FP atual (que aqui
            //    ainda é o do chamador, pois o JAL não rodou ainda):
            //    ADDI temp, FP, offset_da_variavel -> temp = endereço absoluto.
            HashItem* dest_item = search_item(table, quad.addr2.value.name, current_function_scope);
            if (dest_item == NULL) {
                // Mesmo fallback que get_symbol_offset já faz
                // internamente: se não achou no escopo local, tenta
                // "global" (variável global sendo usada de dentro de
                // uma função, que é exatamente o caso aqui: 'marcador'
                // é global, mas current_function_scope é "main").
                dest_item = search_item(table, quad.addr2.value.name, "global");
            }
            int dest_is_global = (dest_item != NULL && strcmp(dest_item->scope, "global") == 0);
            mem_offset_t dest_offset = get_symbol_offset(table, quad.addr2.value.name, current_function_scope);

            int dest_addr_reg = get_new_register();
            if (dest_is_global) {
                // Endereço absoluto já é o próprio offset (base R0).
                emit_operation(ASM_ADDI, reg(dest_addr_reg), reg(0), num(dest_offset));
            } else {
                // Endereço absoluto = FP_chamador + offset.
                emit_operation(ASM_ADDI, reg(dest_addr_reg), reg(FRAME_POINTER), num(dest_offset));
            }
            current_line++;

            emit_operation(ASM_SW, reg(dest_addr_reg), reg(STACK_POINTER), num(2));
            current_line++;

            // 3) JAL salva PC+1 em R31 (RA, ver unidade_de_controle.v) e
            //    salta para o endereço de entrada da função, resolvido
            //    via call_target[call_count] -- um slot PRÓPRIO desta
            //    chamada (não compartilhado com outras Q_CALL), porque
            //    a resolução final do endereço só acontece na
            //    impressão, depois que toda a lista de quádruplas já
            //    foi traduzida.
            mem_offset_t func_entry = get_symbol_offset(table, quad.addr1.value.name, "global");
            call_target[call_count] = func_entry;
            emit_operation(ASM_JAL, num(0), num(0), num(call_count));
            call_count++;
            current_line++;

            break;
        }

        case Q_PARAM_END:
            // SIMPLIFICAÇÃO TEMPORÁRIA: sem parâmetros nem valor de
            // retorno neste passo, não há nada para reservar aqui
            // ainda. Mantido como no-op (em vez de remover o case) para
            // não cair em "default" e ser ignorado silenciosamente de
            // forma confusa.
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
        case Q_RETURN: {
            // addr1 = valor a retornar (constante ou variável local
            // desta função). addr2 não é usado por Q_RETURN (é sempre
            // "-" -- quem carrega o nome da variável de DESTINO é
            // Q_CALL.addr2, não Q_RETURN).
            //
            // Layout (ver Q_CALL/Q_FUNCLABEL): offset 2 deste frame
            // tem o ENDEREÇO ABSOLUTO (não um offset -- um endereço já
            // resolvido) de onde gravar o valor de retorno, calculado
            // pelo chamador antes do JAL. Por isso aqui só precisamos
            // ler esse endereço para um registrador e usá-lo como base
            // de um SW com offset 0 -- não precisamos saber nada sobre
            // o frame do chamador além disso.

            // 1) Calcula o valor a retornar.
            int value_reg = get_new_register();
            if (quad.addr1.type == INT_NUM) {
                emit_operation(ASM_ADDI, reg(value_reg), reg(0), num(quad.addr1.value.int_num));
            } else {
                mem_offset_t value_offset = get_symbol_offset(table, quad.addr1.value.name, current_function_scope);
                emit_operation(ASM_LW, reg(value_reg), reg(FRAME_POINTER), num(value_offset));
            }
            current_line++;

            // 2) Lê o endereço de destino (offset 2) e grava o valor lá.
            int dest_addr_reg = get_new_register();
            emit_operation(ASM_LW, reg(dest_addr_reg), reg(FRAME_POINTER), num(2));
            current_line++;

            emit_operation(ASM_SW, reg(value_reg), reg(dest_addr_reg), num(0));
            current_line++;

            // 3) Restaura o endereço de retorno ANTES de restaurar/mexer
            //    no FP, já que esta leitura ainda usa o FP desta função
            //    (a função chamada), não o do chamador.
            int return_addr_reg = get_new_register();
            emit_operation(ASM_LW, reg(return_addr_reg), reg(FRAME_POINTER), num(1));
            current_line++;

            // 4) Restaura o FP do chamador.
            int saved_fp_reg = get_new_register();
            emit_operation(ASM_LW, reg(saved_fp_reg), reg(FRAME_POINTER), num(0));
            current_line++;

            // 5) Desaloca o frame inteiro (mesma lógica do Q_FUNCEND:
            //    SP volta para onde estava antes do prólogo desta
            //    função).
            emit_operation(ASM_ADD, reg(STACK_POINTER), reg(FRAME_POINTER), reg(0));
            current_sp = current_fp;
            current_line++;

            emit_operation(ASM_ADD, reg(FRAME_POINTER), reg(saved_fp_reg), reg(0));
            current_line++;

            // 6) JR salta para o endereço guardado no registrador (ver
            //    Processador.v: reg_or_im seleciona ula_1 = registers[src1]
            //    quando a instrução é JR -- por isso o operando aqui é
            //    um REGISTRADOR, não um número de label).
            emit_operation(ASM_JR, reg(return_addr_reg), num(0), num(0));
            current_line++;

            break;
        }

        case Q_FUNCLABEL: {
            // ---------------------------------------------------------
            // Registra o ENDEREÇO DE ENTRADA desta função, usando o
            // mesmo mecanismo de label_position[] -- reaproveitamos o
            // campo stack_pointer_offset do HashItem da própria função
            // (kind=FUNC, escopo "global", já que funções em C- só
            // existem no escopo global) para guardar a linha onde o
            // prólogo começa. Isso é o que Q_CALL vai consultar para
            // saber o endereço de destino do JAL.
            // ---------------------------------------------------------
            add_offset_to_symbol(table, quad.addr1.value.name, "global", current_line);

            // ---------------------------------------------------------
            // PRÓLOGO: o frame da nova função começa exatamente onde a
            // pilha está agora (current_sp). FP = SP congela essa
            // posição -- a partir daqui, todo acesso a algo deste frame
            // (parâmetros, slots especiais, variáveis locais) é
            // offset(FP).
            // ---------------------------------------------------------
            emit_operation(ASM_ADD, reg(FRAME_POINTER), reg(STACK_POINTER), reg(0));
            current_line++;

            current_fp = current_sp;

            // Reserva, nesta ordem fixa, os três primeiros offsets de
            // QUALQUER frame de função (inclusive main, por
            // uniformidade -- ela nunca de fato usa esses slots, já
            // que nunca é chamada via Q_CALL, mas reservá-los aqui
            // evita ter dois esquemas de offset diferentes):
            //   offset 0 -> FP do chamador (salvo pelo Q_CALL, ANTES
            //               do JAL -- ver case Q_CALL)
            //   offset 1 -> endereço de retorno (R31/RA, só existe
            //               DEPOIS do JAL, então só pode ser salvo aqui)
            //   offset 2 -> ENDEREÇO ABSOLUTO de onde Q_RETURN deve
            //               gravar o valor de retorno (salvo pelo
            //               Q_CALL, ANTES do JAL -- é o chamador quem
            //               sabe onde sua variável de destino vive)
            // Depois desses três, os parâmetros (Q_INIT emitido por
            // FUNC_PARAM_NODE em code_gen.c) ocupam offset 3, 4, ...,
            // e as variáveis locais do corpo vêm a seguir.
            emit_operation(ASM_SW, reg(RETURN_ADDRESS_POINTER), reg(FRAME_POINTER), num(1));
            current_line++;

            current_sp += 3;
            emit_operation(ASM_ADDI, reg(STACK_POINTER), reg(STACK_POINTER), num(3));
            current_line++;

            current_frame_offset = 3;

            memset(current_function_scope, 0, SCOPE_NAME_SIZE);
            strcpy(current_function_scope, quad.addr1.value.name);
            break;
        }

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

/*
 * emit_output_function: gera o CORPO REAL da função "output", para que
 * ela possa ser chamada via Q_CALL/JAL como qualquer função do
 * usuário (mesma infraestrutura de Q_PARAM/Q_FUNCLABEL/Q_RETURN/
 * Q_FUNCEND -- nada de caso especial no Q_CALL).
 *
 * "output" já existe na tabela de símbolos desde create_table()
 * (FUNC, escopo "global", qnt_param=1) -- só faltava o CORPO. Esse
 * corpo precisa existir ANTES de qualquer variável global do usuário
 * ser declarada, porque reutiliza o mesmo mecanismo de frame de
 * qualquer função (current_fp = current_sp no momento do
 * Q_FUNCLABEL) -- se ele nascesse DEPOIS de globais já terem avançado
 * current_sp, não haveria problema nenhum também, mas gerando ele
 * primeiro garantimos que o frame de "output" ocupa os primeiros
 * endereços da RAM (0, 1, 2 para FP/RA/retorno, 3 para o parâmetro),
 * e as globais do usuário começam a partir daí -- igual qualquer
 * função normal reservaria seu próprio espaço antes da próxima coisa
 * usar aquele offset.
 *
 * Hardware (ver IO.v e registradores.v): R1 = registrador de saída
 * (output_value), R3 = registrador de config (io_config). Bit 0 do
 * config liga a saída, bit 2 seleciona LEDs como dispositivo -- valor
 * fixo 0b101 = 5. Não há nenhum "enable"/timing especial: basta
 * escrever em R3 e depois em R1, ambos via escrita normal de
 * registrador (reg_write + dest).
 */
static void emit_output_function()
{
    struct ADDR func_name = (struct ADDR){.type = NAME, .value.name = "output"};
    struct ADDR dash = (struct ADDR){.type = NAME, .value.name = "-"};
    struct Quadrupla quad;

    // Prólogo: idêntico ao de qualquer função (Q_FUNCLABEL já cuida
    // de registrar o endereço de entrada na tabela de símbolos, fazer
    // FP=SP, salvar RA, e reservar offsets 0/1/2).
    quad = (struct Quadrupla){.type = Q_FUNCLABEL, .addr1 = func_name, .addr2 = dash, .addr3 = dash};
    translate_quad(quad);

    // Parâmetro único de output: vira offset 3, igual qualquer outro
    // parâmetro formal (mesmo mecanismo de FUNC_PARAM_NODE/Q_INIT já
    // usado por funções do usuário -- não existe diferença aqui).
    struct ADDR param_name = (struct ADDR){.type = NAME, .value.name = "__output_param"};
    quad = (struct Quadrupla){.type = Q_INIT, .addr1 = param_name, .addr2 = dash, .addr3 = dash};
    translate_quad(quad);

    // Corpo: nenhuma quádrupla existente faz "escrever direto num
    // registrador especial", então isso é emitido diretamente como
    // AsmOperation, sem passar por translate_quad.

    // 1) Config = 5 (bit 0 output ON + bit 2 LEDs) -- direto em R3,
    //    sem precisar de registrador intermediário.
    emit_operation(ASM_ADDI, reg(3), reg(0), num(5));
    current_line++;

    // 2) Lê o parâmetro (offset 3 do próprio frame) e grava em R1.
    int value_reg = get_new_register();
    mem_offset_t param_offset = get_symbol_offset(table, "__output_param", "output");
    emit_operation(ASM_LW, reg(value_reg), reg(FRAME_POINTER), num(param_offset));
    current_line++;

    emit_operation(ASM_ADD, reg(1), reg(value_reg), reg(0));
    current_line++;

    // Epílogo: igual a qualquer função sem valor de retorno real --
    // o Q_CALL do chamador ainda monta um temporário e grava seu
    // endereço no offset 2 (igual sempre faz), mas como "output" é
    // void, esse slot simplesmente nunca é lido aqui. O Q_RETURN
    // normal faria isso (ler addr1, gravar no offset 2) -- como não
    // há valor de retorno, fazemos só o que sobra do Q_RETURN: restaurar
    // RA e FP do chamador e saltar de volta (JR), sem o LW/SW do valor.
    int return_addr_reg = get_new_register();
    emit_operation(ASM_LW, reg(return_addr_reg), reg(FRAME_POINTER), num(1));
    current_line++;

    int saved_fp_reg = get_new_register();
    emit_operation(ASM_LW, reg(saved_fp_reg), reg(FRAME_POINTER), num(0));
    current_line++;

    emit_operation(ASM_ADD, reg(STACK_POINTER), reg(FRAME_POINTER), reg(0));
    current_sp = current_fp;
    current_line++;

    emit_operation(ASM_ADD, reg(FRAME_POINTER), reg(saved_fp_reg), reg(0));
    current_line++;

    emit_operation(ASM_JR, reg(return_addr_reg), num(0), num(0));
    current_line++;

    // Fecha o escopo -- mesma contabilidade de Q_FUNCEND (current_sp já
    // foi ajustado acima, igual ao epílogo de Q_RETURN faz).
    current_frame_offset = 0;
    memset(current_function_scope, 0, SCOPE_NAME_SIZE);
    strcpy(current_function_scope, "global");
}

void asm_gen(struct QuadrupleList* list)
{   
    printf("\n\nAsm generator using %d Registers\n\n", MAX_NUM_REGISTER);

    // -1 sinaliza "ainda não processado" -- necessário porque 0 é um
    // número de label válido, não pode servir de sentinela.
    for (int i = 0; i < 32; i++) {
        label_frame_offset[i] = -1;
    }

    // "output" precisa existir em assembly ANTES de qualquer global ou
    // função do usuário -- gerar o corpo dela aqui, com current_sp
    // ainda em 0, garante que seu frame ocupa os primeiros endereços
    // da RAM (0..3) e que tudo que vem depois (globais do usuário,
    // outras funções) começa a partir do current_sp já avançado por
    // ela -- exatamente como qualquer função reserva seu próprio
    // espaço antes do que vem depois reaproveitar aquele offset.
    //
    // PORÉM: o PC físico sempre começa em 0 no reset (ver pc.v:
    // destino_instrucao <= 8'b0). Se o corpo de "output" for a
    // primeira coisa em operation_list[], o processador entraria
    // direto nele ao ligar -- sem nunca ter sido chamado por ninguém
    // (RA/FP do "chamador" seriam lixo). Por isso, um JUMP precisa
    // desviar a execução por cima do corpo de "output" ANTES dele,
    // assim como o JUMP existente já desvia por cima das funções do
    // usuário até chegar em "main". Esse JUMP usa o mesmo mecanismo de
    // label_position[]/resolução tardia: reservamos o índice 30 (29 já
    // não é usado por nenhum label de código C- real, e 31 já está
    // reservado para o endereço de "main" mais abaixo).
    int output_jump_index = current_list_position;
    emit_operation(ASM_JUMP, num(0), num(0), num(0));
    current_line++;

    emit_output_function();

    label_position[30] = current_line;
    operation_list[output_jump_index].operands[2] = num(30);

    // O ponto de entrada do processador é sempre o endereço 0 (ver
    // pc.v: destino_instrucao <= 8'b0 no reset). Mas no código gerado,
    // 'main' pode aparecer em qualquer posição entre as FUNÇÕES -- ela
    // é só mais uma função na lista de quádruplas, na ordem em que
    // foram declaradas no programa C-, não necessariamente a primeira.
    //
    // IMPORTANTE: isso não pode simplesmente saltar para 'main' como
    // a primeiríssima instrução do programa (índice 0), porque antes
    // de qualquer Q_FUNCLABEL a lista de quádruplas sempre contém as
    // declarações de variáveis GLOBAIS (Q_INIT/Q_INITVET com escopo
    // "global") -- e essas precisam executar normalmente (avançando
    // SP, reservando espaço) antes de qualquer função rodar. Pular
    // direto para 'main' faria esse código de alocação de globais
    // nunca executar, deixando SP sem avançar para protegê-las,
    // mesmo que o acesso direto a elas (endereço fixo a partir de
    // R0) continuasse funcionando por coincidência -- até colidir
    // com a primeira variável local de alguma função que reutilize
    // aquele mesmo espaço de pilha.
    //
    // Solução: o JUMP para 'main' é inserido exatamente na fronteira
    // entre "declarações globais" e "funções" -- ou seja, bem antes
    // do primeiro Q_FUNCLABEL da lista, não antes de tudo.
    struct ListNode* node = list->list;
    int jump_inserted = 0;
    int jump_index = -1;

    while(node != NULL){
        if (!jump_inserted && node->quad.type == Q_FUNCLABEL) {
            // Chegamos na primeira função da lista -- todas as
            // declarações globais já foram processadas (a gramática
            // de C- garante que elas sempre vêm antes de qualquer
            // função). É aqui, e não antes, que o JUMP para 'main'
            // deve ser inserido. Guardamos o ÍNDICE exato em
            // operation_list[] (não vamos "procurar" por ele depois --
            // buscar por um JUMP com operando 0 seria ambíguo, já que
            // um Q_GOTO comum para o label "0" também produziria um
            // JUMP com esse mesmo operando).
            jump_index = current_list_position;
            emit_operation(ASM_JUMP, num(0), num(0), num(0));
            current_line++;
            jump_inserted = 1;
        }

        translate_quad(node->quad);
        node = node->next;
    }

    // Agora que 'main' já foi processada (Q_FUNCLABEL já rodou e
    // registrou seu endereço de entrada via add_offset_to_symbol),
    // resolve o destino do JUMP inserido acima, usando o índice exato
    // capturado em jump_index. print_asm_operation imprime JUMP/JAL
    // como label_position[operando] -- ou seja, o operando gravado em
    // emit_operation é sempre um ÍNDICE em label_position[], nunca o
    // endereço final direto (a resolução acontece só na impressão,
    // depois que toda a lista já foi processada). Por isso reservamos
    // aqui o último slot de label_position[] (índice 31, nunca usado
    // por nenhum label de código C- real, que começam em 0 e crescem)
    // para guardar o endereço de 'main'.
    if (jump_index >= 0) {
        mem_offset_t main_entry = get_symbol_offset(table, "main", "global");
        label_position[31] = main_entry;
        operation_list[jump_index].operands[2] = num(31);
    }
    emit_operation(ASM_HALT, reg(0), reg(0), reg(0));
    print_op_list();   
}