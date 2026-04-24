#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parse.h"
#include "code_gen.h"
#include "../gen/sintatico.tab.h"

int temporary_variable = 0;
int current_label = 0;

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

static struct Quadrupla build_var_quad(char* name){
  union ADDRESS addr1, addr2, addr3;

  addr1.name = name;
  addr2.name = "-";
  addr3.name = "-";

  return build_quad(Q_INIT, NAME, addr1, REGIST, addr2, VAZIO, addr3);
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

void print_quad(struct Quadrupla quad){

  printf("%s %s %s %s", quadruple_type_to_string(quad.type), quad.addr1.value.name, quad.addr2.value.name, quad.addr3.value.name);
  printf("\n");
  
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

const char* quadruple_type_to_string(QUADRUPLE_TYPES type) {
    switch (type) {
        case Q_SOMA:        return "Q_SOMA";
        case Q_SUB:         return "Q_SUB";
        case Q_DIV:         return "Q_DIV";
        case Q_MULT:        return "Q_MULT";
        case Q_MAIOR:       return "Q_MAIOR";
        case Q_MAIOR_Q:     return "Q_MAIOR_Q";
        case Q_MENOR_Q:     return "Q_MENOR_Q";
        case Q_IGUAL:       return "Q_IGUAL";
        case Q_DIFF:        return "Q_DIFF";
        case Q_MENOR:       return "Q_MENOR";
        case Q_ASSIGN:      return "Q_ASSIGN";
        case Q_ASSIGN_VET:  return "Q_ASSIGN_VET";
        case Q_ACCESS_VET:  return "Q_ACCESS_VET";
        case Q_IF:          return "Q_IF";
        case Q_GOTO:        return "Q_GOTO";
        case Q_INITVET:     return "Q_INITVET";
        case Q_INIT:        return "Q_INIT";
        case Q_DEF:         return "Q_DEF";
        case Q_CALL:        return "Q_CALL";
        case Q_ARG:         return "Q_ARG";
        case Q_RETURN:      return "Q_RETURN";
        case Q_HALT:        return "Q_HALT";
        default:            return "UNKNOWN";
    }
}


char* gen_code(struct ParseTree* tree){

  struct Quadrupla quad;

  if(tree == NULL) return NULL;

  char value1[16], value2[16], value3[16], label[4]; 
  char* temp_name = malloc(4);
  ADDR_TYPES value1_type, value2_type, value3_type;

  switch(tree->node_type){
    case NUM_NODE:
      char* temp = malloc(16);
      sprintf(temp, "%d", tree->node_value.num_value);
      return temp;
    case ASSIGN_NODE:
      if(tree->children[1] == NULL) return NULL; 
      sprintf(value1, "%s",gen_code(tree->children[1]));
      if(tree->children[1]->node_type == NUM_NODE){
        value1_type = INT_NUM;
      }
      else{
        value1_type = NAME;
      }
      quad = build_assign_quad(tree->children[0]->children[0]->node_value.id_name, value1_type, value1);
      print_quad(quad);
      gen_code(tree->sibling);

      return NULL;
    case VAR_DECL_NODE: 
      char temp1[16];
      strcpy(temp1, tree->children[0]->node_value.id_name); 
      quad = build_var_quad(temp1);

      print_quad(quad);
      gen_code(tree->sibling);
      return NULL;
    case VAR_NODE:
      return tree->children[0]->node_value.id_name;
    case OP_NODE:
      if(tree->children[0] == NULL || tree->children[2] == NULL) return NULL;
      int temporary = temporary_variable++;
      sprintf(value1, "%s",gen_code(tree->children[0]));
      sprintf(value2, "%s",gen_code(tree->children[2]));

      if(tree->children[0]->node_type == NUM_NODE){
        value1_type = INT_NUM;
      }
      else{
        value1_type = NAME;
      }

      if(tree->children[2]->node_type == NUM_NODE){
        value2_type = INT_NUM;
      }
      else{
        value2_type = NAME;
      }
      
      
      sprintf(temp_name, "_t%d", temporary);
      quad = build_op_quad(token_to_quad(tree->children[1]->node_value.op_value), temp_name, value1_type, value1, value2_type, value2);
      print_quad(quad);
      gen_code(tree->sibling);
      return temp_name;

    case IF_NODE:
  
      
      sprintf(value1, "%s",gen_code(tree->children[0]));
      sprintf(label, "L%d", current_label++);
      printf("Q_IF %s goto %s\n", value1, label);
      sprintf(value3, "%s",tree->children[2] == NULL ? "-" :gen_code(tree->children[2]));
      printf("%s  ", label);
      sprintf(value2, "%s",gen_code(tree->children[1]));

      if(tree->children[0]->node_type == NUM_NODE){
        value1_type = INT_NUM;
      }
      else{
        value1_type = NAME;
      }

      if(tree->children[1]->node_type == NUM_NODE){
        value2_type = INT_NUM;
      }
      else{
        value2_type = NAME;
      }

      if(tree->children[2] != NULL && tree->children[2]->node_type == NUM_NODE){
        value3_type = INT_NUM;
      }
      else{
        value3_type = NAME;
      }
      gen_code(tree->sibling);
    
      return NULL;
    case FUNC_ACTV_NODE:
      struct ParseTree* args = tree->children[1];
      struct ParseTree* t = args != NULL ? args->children[0] : NULL;
      while(t != NULL){
        sprintf(value1, "%s",gen_code(t));
        printf("PARAM %s\n", value1);
        t = t->sibling;
      }
    
      sprintf(temp_name, "_t%d", temporary_variable++);

      printf("CALL %s %s\n", tree->children[0]->node_value.id_name, temp_name);
      gen_code(tree->sibling);
      return temp_name;
    case FUNC_NODE:
    case DECL_LIST_NODE:
    case WHILE_NODE:
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
