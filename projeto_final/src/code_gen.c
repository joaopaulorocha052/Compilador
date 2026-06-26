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

  char value1[16], value2[16], value3[16], label[4]; 
  char temp1[16];
  char* temp_name = malloc(4);
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

      emit_quad(token_to_quad(tree->children[1]->node_value.op_value), temp_name, gen_code(tree->children[0]), gen_code(tree->children[2]));
      gen_code(tree->sibling);
      return temp_name;

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

      emit_quad(Q_CALL, tree->children[0]->node_value.id_name, temp_name, "-");
      gen_code(tree->sibling);
      return temp_name;

    case FUNC_NODE:
      emit_quad(Q_FUNCLABEL, tree->children[0]->node_value.id_name, "-", "-");

      gen_code(tree->children[1]);
      gen_code(tree->children[2]);

      emit_quad(Q_FUNCEND, tree->children[0]->node_value.id_name, "-", "-");
      gen_code(tree->sibling);
      break;


    case WHILE_NODE:
      int first_label, second_label, third_label;
      first_label = current_label;
      second_label = current_label + 1;
      third_label = current_label + 2;
      current_label = third_label+1;

      sprintf(label, "%d", first_label);
      emit_quad(Q_LABEL, label, "-", "-");
      sprintf(label, "%d", second_label);
      emit_quad(Q_IF, gen_code(tree->children[0]), label, "-");
      

      sprintf(label, "%d", third_label);
      emit_quad(Q_GOTO, label, "-", "-");

      sprintf(label, "%d", second_label);
      emit_quad(Q_LABEL, label, "-", "-");
      gen_code(tree->children[1]);

      sprintf(label, "%d", first_label);
      emit_quad(Q_GOTO, label, "-", "-");

      sprintf(label, "%d", third_label);
      emit_quad(Q_LABEL, label, "-", "-");
      
      gen_code(tree->sibling);
      return NULL;

    case DECL_LIST_NODE:
      for (int i = 0; i < NUM_CHILDREN; i++) {
          gen_code(tree->children[i]);
      }
      gen_code(tree->sibling);
      return NULL;
    }
}

