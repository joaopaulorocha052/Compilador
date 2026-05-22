#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parse.h"
#include "code_gen.h"
#include "utils.h"
#include "../gen/sintatico.tab.h"

int temporary_variable = 0;
int current_label = 0;

extern struct QuadrupleList* quadList;

static struct Quadrupla build_quad(QUADRUPLE_TYPES quad_type,
			    ADDR_TYPES first_address_type,
			    union ADDRESS first_address_value,
			    ADDR_TYPES second_address_type,
			    union ADDRESS second_address_value,
			    ADDR_TYPES third_address_type,
			    union ADDRESS third_address_value
			    )
{

     
    
    
    return (struct Quadrupla){
	    .type = quad_type,
	    .addr1 = {.type = first_address_type, .value = first_address_value},
	    .addr2 = {.type = second_address_type, .value = second_address_value},
	    .addr3 = {.type = third_address_type, .value = third_address_value}
	    
	  };
  
}

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

static struct Quadrupla build_var_quad(char* name){
  union ADDRESS addr1, addr2, addr3;

  addr1.name = name;
  addr2.name = "-";
  addr3.name = "-";

  return build_quad(Q_INIT, NAME, addr1, REGIST, addr2, VAZIO, addr3);
}

static struct Quadrupla build_vet_quad(char* name, char* num){
  union ADDRESS addr1, addr2, addr3;

  addr1.name = name;
  addr2.name = num;
  addr3.name = "-";

  return build_quad(Q_INITVET, NAME, addr1, REGIST, addr2, VAZIO, addr3);
}

static struct Quadrupla build_assign_quad(char* name, ADDR_TYPES second_address_type, char * second_addres){
  union ADDRESS addr1, addr2, addr3;

  addr1.name = name;
  addr2.name = second_addres;
  addr3.name = "-";

  

  return build_quad(Q_ASSIGN, NAME, addr1, second_address_type, addr2, VAZIO, addr3);
}

static struct Quadrupla build_op_quad(QUADRUPLE_TYPES op_type, char* name, ADDR_TYPES second_address_type, char * second_addres, ADDR_TYPES third_address_type, char * third_addres){
  union ADDRESS addr1, addr2, addr3;

  addr1.name = name;
  addr2.name = second_addres;
  addr3.name = third_addres;

  

  return build_quad(op_type, REGIST, addr1, second_address_type, addr2, third_address_type, addr3);
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

void print_quad2(struct Quadrupla quad){

  printf("%s %s %d %s", quadruple_type_to_string(quad.type), quad.addr1.value.name, quad.addr2.value.int_num, quad.addr3.value.name);
  printf("\n");
  
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
      // printf("RETURN %s\n", gen_code(tree->children[0]));
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
        sprintf(size, "%d", 4);
        sprintf(temp_name, "_t%d", temporary_variable++);
        quad = build_op_quad(Q_MULT, temp_name, REGIST, gen_code(tree->children[1]), INT_NUM, size);

        print_quad(quad);

        sprintf(temp_return, "%s[%s]", tree->children[0]->node_value.id_name, temp_name);
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
      sprintf(label, "L%d:", current_label++);

      emit_quad(Q_IF, gen_code(tree->children[0]), label, "-");
      gen_code(tree->children[1]);
      printf("%s  ", label);
      gen_code(gen_code(tree->children[2]));

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
      printf("%s: \n", tree->children[0]->node_value.id_name);
      gen_code(tree->children[1]);
      gen_code(tree->children[2]);
      printf("END %s\n", tree->children[0]->node_value.id_name);
      gen_code(tree->sibling);
      break;


    case WHILE_NODE:
      int first_label, second_label, third_label;
      first_label = current_label;
      second_label = current_label + 1;
      third_label = current_label + 2;
      current_label = third_label+1;
      printf("L%d: ", first_label);
      sprintf(label, "L%d:", second_label);
      
      emit_quad(Q_IF, gen_code(tree->children[0]), label, "-");

      sprintf(label, "L%d:", second_label);
      emit_quad(Q_GOTO, label, "-", "-");
      printf("L%d: ", second_label);
      gen_code(tree->children[1]);
      sprintf(label, "L%d:", first_label);
      emit_quad(Q_GOTO, label, "-", "-");
      printf("L%d: ", third_label);
      
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

/* char* print_tree_code_gen(struct ParseTree* tree){ */

/*   int  children_already_checked = 0; */

  
/*   if(tree == NULL || children_already_checked) return; */
    
/*     char str[9]; */
/*     char temp[15]; */
/*     char msg[30]; */

    

/*     struct Quadrupla quad; */
/*     switch(tree->node_type){ */
/*         case OP_TERMINAL_NODE: */

/*             break; */
/*         case OP_NODE: */
/* 	  //printf("OP_NODE\n"); */

/* 	  quad = build_quad(tree->node_value.op_value, ) */
	  
/* 	  break; */
/*         case NUM_NODE: */
/* 	  //printf("NUM_NODE: %d\n", tree->node_value.num_value); */
/*             break; */
/*         case VAR_DECL_NODE: */
/* 	  strcpy(temp, tree->children[0]->node_value.id_name); */

/* 	  quad = build_var_quad(temp); */
/* 	  print_quad(quad); */
/* 	  //printf("VAR_NODE\n"); */
/*             break; */
/*         case ARGS_NODE: */
/* 	  //printf("ARGS_NODE\n"); */
/*             break; */
/*         case ID_NODE: */
/* 	  //printf("ID_NODE: %s\n", tree->node_value.id_name); */
/*             break; */
/*         case FUNC_PARAM_NODE: */
/* 	  //printf("FUNC_PARAM_NODE\n"); */
/*             break; */
/*         case FUNC_NODE: */
/* 	  //printf("FUNC_NODE\n"); */
/*             break; */
/*         case DECL_LIST_NODE: */
/* 	  //printf("DECL_LIST_NODE\n"); */
/*             break; */
/*         case ASSIGN_NODE: */
/* 	  strcpy(temp, tree->children[0]->children[0]->node_value.id_name); */
/* 	  int var1_type = tree->children[1]->node_type; */
/* 	  int teste; */
/* 	  if(var1_type == NUM_NODE){ */
/* 	    teste = tree->children[1]->node_value.num_value; */
/* 	  } */
/* 	  else{ */
	    
	    
/* 	  } */
/* 	  quad = build_assign_quad(temp, var1_type, &teste); */
/* 	  print_quad2(quad); */
/* 	  //printf("ASSIGN_NODE:\n"); */
/*             break; */
/*         case FUNC_ACTV_NODE: */
/* 	  //printf("FUNC_ACTV_NODE:\n"); */
/*             break; */
/*         case RETURN_NODE: */
/* 	  //printf("RETURN_NODE:\n"); */
/*             break; */
/*         case IF_NODE: */
/* 	  //printf("IF_NODE:\n"); */
/*             break; */
/*         case WHILE_NODE: */
/* 	  //printf("WHILE_NODE:\n"); */
/*             break; */
/*         default: */
/* 	  //printf("Unknown Node: %d\n", tree->node_type); */
/*             break; */
/*     } */
    
    
/*     for(int i=0; i<NUM_CHILDREN; i++){ */
/*         print_tree_code_gen(tree->children[i]); */
/*     } */

/*     print_tree_code_gen(tree->sibling); */
/* } */
