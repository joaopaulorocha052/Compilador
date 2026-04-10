#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "parse.h"
#include "code_gen.h"
#include "../gen/sintatico.tab.h"


char* get_register(){
  return "$t1";
}


struct Quadrupla build_quad(QUADRUPLE_TYPES quad_type, struct ADDR addr1, struct ADDR addr2, struct ADDR addr3{

     
    
    
    return (struct Quadrupla){
	    .type = quad_type,
	    .addr1 = {.type = NAME, .ADDRESS.name = temp},
	    .addr2 = {.type = REGIST, .ADDRESS.name = get_register()},
	    .addr3 = {.type = VAZIO, .ADDRESS.name = "-"}
	    
	  };
  
}
void print_quad(struct Quadrupla quad){

  printf("Q_INIT %s %s %s", quad.addr1.ADDRESS.name, quad.addr2.ADDRESS.name, quad.addr3.ADDRESS.name);
  printf("\n");
  
}
void print_tree_code_gen(struct ParseTree* tree){


  
    if(tree == NULL) return;
    
    char str[3];
    char temp[15];
    char msg[30];

    struct Quadrupla quad;
    switch(tree->node_type){
        case OP_TERMINAL_NODE:
	  
	  // trocar depois por printToken
            switch(tree->node_value.op_value){
                case PLUS:
                    strcpy(str, "+");
                    break;
                case MINUS:
                    strcpy(str, "-");
                    break;
                case TIMES:
                    strcpy(str, "*");
                    break;
                case OVER:
                    strcpy(str, "/");
                    break;
                case LTE :
                    strcpy(str, "<=");
                    break;
                case LT  :
                    strcpy(str, "<");
                    break;
                case GT  :
                    strcpy(str, ">");
                    break;
                case GTE :
                    strcpy(str, ">=");
                    break;
                case EQ  :
                    strcpy(str, "==");
                    break;
                case DIFF:
                    strcpy(str, "!=");
                    break;
                default:
                    printf("Operação não reconhecida!!!");
                    break;
            }


            break;
        case OP_NODE:
	  //printf("OP_NODE\n");
            break;
        case NUM_NODE:
	  //printf("NUM_NODE: %d\n", tree->node_value.num_value);
            break;
        case VAR_NODE:
	  strcpy(temp, tree->children[0]->node_value.id_name);

	  quad = (struct Quadrupla){
	    .type = Q_INIT,
	    .addr1 = {.type = NAME, .ADDRESS.name = temp},
	    .addr2 = {.type = REGIST, .ADDRESS.name = get_register()},
	    .addr3 = {.type = VAZIO, .ADDRESS.name = "-"}
	    
	  };
	  print_quad(quad);
	  //printf("VAR_NODE\n");
            break;
        case ARGS_NODE:
	  //printf("ARGS_NODE\n");
            break;
        case ID_NODE:
	  //printf("ID_NODE: %s\n", tree->node_value.id_name);
            break;
        case FUNC_PARAM_NODE:
	  //printf("FUNC_PARAM_NODE\n");
            break;
        case FUNC_NODE:
	  //printf("FUNC_NODE\n");
            break;
        case DECL_LIST_NODE:
	  //printf("DECL_LIST_NODE\n");
            break;
        case ASSIGN_NODE:
	  //printf("ASSIGN_NODE:\n");
            break;
        case FUNC_ACTV_NODE:
	  //printf("FUNC_ACTV_NODE:\n");
            break;
        case RETURN_NODE:
	  //printf("RETURN_NODE:\n");
            break;
        case IF_NODE:
	  //printf("IF_NODE:\n");
            break;
        case WHILE_NODE:
	  //printf("WHILE_NODE:\n");
            break;
        default:
	  //printf("Unknown Node: %d\n", tree->node_type);
            break;
    }
    
    
    for(int i=0; i<NUM_CHILDREN; i++){
        print_tree_code_gen(tree->children[i]);
    }

    print_tree_code_gen(tree->sibling);
}
