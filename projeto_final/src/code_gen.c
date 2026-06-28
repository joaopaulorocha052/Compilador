#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parse.h"
#include "code_gen.h"
#include "utils.h"
#include "symbol_table.h"
#include "../gen/sintatico.tab.h"

int temporary_variable = 0;
int current_label = 0;

extern struct QuadrupleList* quadList;
extern struct HashTable* table;


static struct Quadrupla create_quad(QUADRUPLE_TYPES quad_type,
			    struct ADDR first_address_value,
			    struct ADDR second_address_value,
			    struct ADDR third_address_value
			    )
{

    return (struct Quadrupla){
	    .type = quad_type,
	    .addr1 = first_address_value,
	    .addr2 =  second_address_value,
	    .addr3 = third_address_value
	    
	  };
  
}

int emit_quad(QUADRUPLE_TYPES quad_type, char* first_value, char* second_value, char* third_value){
  struct ADDR addrs[3];
  char* value_list[] = {first_value, second_value, third_value};

  struct Quadrupla quad;

  for(int i=0; i<3; i++){

    if(check_address_is_num(value_list[i])){

      addrs[i] = (struct ADDR){
        .type = INT_NUM,
        .value.int_num = atoi(value_list[i])
      };

    }else{
      addrs[i] = (struct ADDR){
        .type = NAME,
        .value.name = value_list[i]
      };
    }
  }

  quad = create_quad(quad_type, addrs[0], addrs[1], addrs[2]);

  if(!add_quadruple_to_list(quadList, quad)){
    printf("Erro ao adicionar quadrupla à lista");
    return 0;
  };
  
  
  return 1;
}



const QUADRUPLE_TYPES token_to_quad(int token) {
    switch (token) {
        case PLUS:   return Q_SOMA;
        case MINUS:  return Q_SUB;
        case TIMES:  return Q_MULT;
        case OVER:   return Q_DIV;

        case GT:     return Q_MAIOR;
        case GTE:    return Q_MAIOR_Q;
        case LT:     return Q_MENOR;
        case LTE:    return Q_MENOR_Q;

        case EQ:     return Q_IGUAL;
        case DIFF:   return Q_DIFF;

        case ASSIGN: return Q_ASSIGN;

        case IF:     return Q_IF;
        case RETURN: return Q_RETURN;

    }
}




char* gen_code(struct ParseTree* tree){

  struct Quadrupla quad;

  if(tree == NULL) return NULL;

  char value1[16], value2[16], value3[16], label[16]; 
  char temp1[16];
  char* temp_name = malloc(16);
  ADDR_TYPES value1_type, value2_type, value3_type;

  switch(tree->node_type){

    case NUM_NODE:
      char* temp = malloc(16);
      sprintf(temp, "%d", tree->node_value.num_value);
      return temp;

    case RETURN_NODE:

      emit_quad(Q_RETURN, gen_code(tree->children[0]), "-", "-");
      return NULL;

    case ASSIGN_NODE:
      emit_quad(Q_ASSIGN, tree->children[0]->children[0]->node_value.id_name, gen_code(tree->children[1]), "-");
      gen_code(tree->sibling);

      return NULL;

    case VAR_DECL_NODE: 
      emit_quad(Q_INIT, tree->children[0]->node_value.id_name, "-", "-");
      gen_code(tree->sibling);
      return NULL;

    case VET_DECL_NODE:
      sprintf(temp_name, "%d", tree->children[1]->node_value.num_value);
      emit_quad(Q_INITVET, tree->children[0]->node_value.id_name, temp_name, "-");
      gen_code(tree->sibling);
      return NULL;

    case VAR_NODE:
      
      if(tree->children[1] == NULL) return tree->children[0]->node_value.id_name;
      else{
        char size[1];
        char *temp_return = malloc(16);

        sprintf(temp_return, "%s[%s]", tree->children[0]->node_value.id_name, gen_code(tree->children[1]));
        return temp_return;
        
      }

    case ID_NODE:
      return tree->children[0]->node_value.id_name;

    case OP_NODE:
      if(tree->children[0] == NULL || tree->children[2] == NULL) return NULL;
      int temporary = temporary_variable++;
      sprintf(temp_name, "_t%d", temporary);

      // Temporários também precisam de espaço alocado na pilha/memória,
      // assim como uma variável declarada pelo usuário. Sem isso, o
      // asm_gen não tem como saber o offset de "_tN" e ele acaba
      // colidindo com o offset 0 (padrão) de outra variável.
      emit_quad(Q_INIT, temp_name, "-", "-");

      emit_quad(token_to_quad(tree->children[1]->node_value.op_value), temp_name, gen_code(tree->children[0]), gen_code(tree->children[2]));
      gen_code(tree->sibling);
      return temp_name;

    case FUNC_PARAM_NODE: {
      // children[0] é o primeiro parâmetro formal (um VAR_NODE),
      // encadeado via sibling para os demais (ver param-lista em
      // sintatico.y). Cada parâmetro já foi inserido na tabela de
      // símbolos durante a fase sintática (com o escopo da função),
      // mas nunca passa por Q_INIT -- sem isso, asm_gen.c não tem como
      // saber em que offset cada parâmetro vive dentro do frame da
      // função. Emitindo Q_INIT aqui, na ordem de declaração, eles
      // recebem offsets 0, 1, 2... automaticamente, do mesmo jeito que
      // qualquer variável local declarada no corpo da função.
      struct ParseTree* param = tree->children[0];
      while (param != NULL) {
        emit_quad(Q_INIT, param->children[0]->node_value.id_name, "-", "-");
        param = param->sibling;
      }
      // Marca o fim da lista de parâmetros -- é o sinal para
      // asm_gen.c reservar o slot do valor de retorno EXATAMENTE
      // aqui, antes de qualquer variável local/temporário do corpo
      // ocupar esse offset (ver case Q_PARAM_END em asm_gen.c).
      emit_quad(Q_PARAM_END, "-", "-", "-");
      return NULL;
    }

    case IF_NODE:
      sprintf(label, "%d", current_label++);
      char* label_string = strdup(label);
      emit_quad(Q_IF, gen_code(tree->children[0]), label_string, "-");
      gen_code(tree->children[1]);
      
      if(tree->children[2] != NULL)
      {
        sprintf(label, "%d", current_label++);
        char* goto_label = strdup(label);
        emit_quad(Q_GOTO, goto_label, "-", "-");
        emit_quad(Q_LABEL, label_string, "-", "-");
        gen_code(tree->children[2]);
        emit_quad(Q_LABEL, goto_label, "-", "-");
      }
      else
      {
        emit_quad(Q_LABEL, label_string, "-", "-");
      }
      


      gen_code(tree->sibling);
    
      return NULL;

    case FUNC_ACTV_NODE:
      struct ParseTree* args = tree->children[1];
      struct ParseTree* t = args != NULL ? args->children[0] : NULL;
      while(t != NULL){
        emit_quad(Q_PARAM, gen_code(t), "-", "-");
        t = t->sibling;
      }
    
      sprintf(temp_name, "_t%d", temporary_variable++);

      // Mesmo motivo do OP_NODE: "_tN" precisa de offset alocado antes
      // de Q_CALL escrever o valor de retorno nele.
      emit_quad(Q_INIT, temp_name, "-", "-");

      emit_quad(Q_CALL, tree->children[0]->node_value.id_name, temp_name, "-");
      gen_code(tree->sibling);
      return temp_name;

    case FUNC_NODE:
      emit_quad(Q_FUNCLABEL, tree->children[0]->node_value.id_name, "-", "-");

      // children[1] só é um FUNC_PARAM_NODE de verdade quando a função
      // declara parâmetros (params: param-lista). Quando a função é
      // declarada void (sem parâmetros), children[1] pode ser NULL ou
      // não ser um FUNC_PARAM_NODE -- nesse caso simplesmente não há
      // nada para processar aqui (gen_code não tem um case genérico
      // para outros tipos de nó, então chamá-lo às ciegas seria
      // comportamento indefinido).
      if (tree->children[1] != NULL && tree->children[1]->node_type == FUNC_PARAM_NODE) {
        gen_code(tree->children[1]);
      } else {
        // Mesmo sem parâmetros, Q_PARAM_END precisa ser emitido --
        // asm_gen.c depende dela existir SEMPRE para reservar o slot
        // do valor de retorno no offset certo (ver case Q_PARAM_END
        // em asm_gen.c). Funções void sem parâmetros caem aqui.
        emit_quad(Q_PARAM_END, "-", "-", "-");
      }

      gen_code(tree->children[2]);

      emit_quad(Q_FUNCEND, tree->children[0]->node_value.id_name, "-", "-");
      gen_code(tree->sibling);
      break;


    case WHILE_NODE: {
      // Mesma semântica de Q_IF usada no IF_NODE: "salte para a label
      // quando a condição for FALSA". Antes, este WHILE_NODE gerava
      // Q_IF com a semântica OPOSTA (salta quando verdadeiro), o que
      // só funcionava porque o asm_gen.c interpretava Q_IF de forma
      // diferente dependendo de quem chamou -- inconsistência perigosa.
      // Agora as duas estruturas de controle (if e while) emitem Q_IF
      // com o mesmo significado, e o asm_gen.c não precisa saber qual
      // construção C- originou a quádrupla.
      //
      // Estrutura gerada:
      //   L_topo:
      //     Q_IF cond, L_fim      -> se falso, sai do loop
      //     <corpo>
      //     Q_GOTO L_topo         -> reavalia a condição
      //   L_fim:
      int topo_label, fim_label;
      topo_label = current_label++;
      fim_label = current_label++;

      sprintf(label, "%d", topo_label);
      emit_quad(Q_LABEL, label, "-", "-");

      sprintf(label, "%d", fim_label);
      emit_quad(Q_IF, gen_code(tree->children[0]), label, "-");

      gen_code(tree->children[1]);

      sprintf(label, "%d", topo_label);
      emit_quad(Q_GOTO, label, "-", "-");

      sprintf(label, "%d", fim_label);
      emit_quad(Q_LABEL, label, "-", "-");

      gen_code(tree->sibling);
      return NULL;
    }

    case DECL_LIST_NODE:
      for (int i = 0; i < NUM_CHILDREN; i++) {
          gen_code(tree->children[i]);
      }
      gen_code(tree->sibling);
      return NULL;
    }
}