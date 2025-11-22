#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parse.h"
#include "lexer.h"
#include "sintatico.tab.h"


//RETURN_NODE, IF_NODE, WHILE_NODE

void print_tree_prefix(struct ParseTree* tree, int level, char* prefix){
    
    if(tree == NULL) return;

    char local_prefix[256] = "";
    int next_level = level;
    // for(int i=0; i<level; i++) strcat(local_prefix, "\u2502 ");
    
    strcat(local_prefix, prefix);


    printf("%s\u251C\u2500", local_prefix);
    char str[3];
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
            // trocar depois por printToken
            printf("OP_TERMINAL_NODE: %s\n", str);
            break;
        case OP_NODE:
            printf("OP_NODE\n");
            break;
        case NUM_NODE:
            printf("NUM_NODE: %d\n", tree->node_value.num_value);
            break;
        case VAR_NODE:
            printf("VAR_NODE\n");
            break;
        case ARGS_NODE:
            printf("ARGS_NODE\n");
            break;
        case ID_NODE:
            printf("ID_NODE: %s\n", tree->node_value.id_name);
            break;
        case FUNC_PARAM_NODE:
            printf("FUNC_PARAM_NODE\n");
            break;
        case FUNC_NODE:
            printf("FUNC_NODE\n");
            break;
        case DECL_LIST_NODE:
            printf("DECL_LIST_NODE\n");
            break;
        case ASSIGN_NODE:
            printf("ASSIGN_NODE:\n");
            break;
        case FUNC_ACTV_NODE:
            printf("FUNC_ACTV_NODE:\n");
            break;
        case RETURN_NODE:
            printf("RETURN_NODE:\n");
            break;
        case IF_NODE:
            printf("IF_NODE:\n");
            break;
        case WHILE_NODE:
            printf("WHILE_NODE:\n");
            break;
        default:
            printf("Unknown Node: %d\n", tree->node_type);
            break;
    }
    char *first_child_prefix;
    next_level++;
    for(int i=0; i<NUM_CHILDREN; i++){
        char child_prefix[256] = "";
        strcat(child_prefix, local_prefix);
        if(i < 2 && tree->children[i+1] == NULL) strcat(child_prefix, "  ");
        else strcat(child_prefix, "\u2502  ");
        if(i == 1) first_child_prefix = strdup(child_prefix);
        print_tree_prefix(tree->children[i], next_level, child_prefix);
    }

    print_tree_prefix(tree->sibling, level, first_child_prefix);

    free_node(tree);
}
void print_tree(struct ParseTree* tree, int level){
    
    if(tree == NULL) return;

    char local_prefix[80] = "";
    int next_level = level;
    for(int i=0; i<level; i++) strcat(local_prefix, "\u2502 ");

    strcat(local_prefix, "\u251C\u2500");

    printf("%s", local_prefix);
    char str[3];
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
            // trocar depois por printToken
            printf("OP_TERMINAL_NODE: %s\n", str);
            break;
        case OP_NODE:
            printf("OP_NODE\n");
            break;
        case NUM_NODE:
            printf("NUM_NODE: %d\n", tree->node_value.num_value);
            break;
        case VAR_NODE:
            printf("VAR_NODE\n");
            break;
        case ARGS_NODE:
            printf("ARGS_NODE\n");
            break;
        case ID_NODE:
            printf("ID_NODE: %s\n", tree->node_value.id_name);
            break;
        case FUNC_PARAM_NODE:
            printf("FUNC_PARAM_NODE\n");
            break;
        case FUNC_NODE:
            printf("FUNC_NODE\n");
            break;
        case DECL_LIST_NODE:
            printf("DECL_LIST_NODE\n");
            break;
        case ASSIGN_NODE:
            printf("ASSIGN_NODE:\n");
            break;
        case FUNC_ACTV_NODE:
            printf("FUNC_ACTV_NODE:\n");
            break;
        case RETURN_NODE:
            printf("RETURN_NODE:\n");
            break;
        case IF_NODE:
            printf("IF_NODE:\n");
            break;
        case WHILE_NODE:
            printf("WHILE_NODE:\n");
            break;
        default:
            printf("Unknown Node: %d\n", tree->node_type);
            break;
    }
    
    next_level++;
    for(int i=0; i<NUM_CHILDREN; i++){
        print_tree(tree->children[i], next_level);
    }

    print_tree(tree->sibling, level);

    free_node(tree);
}

static void free_node(struct ParseTree* node){
    if(node->node_type == ID_NODE) free(node->node_value.id_name);

    free(node);
}
static struct ParseTree* allocate_node(NodeType type){
    struct ParseTree* node = (struct ParseTree*) malloc(sizeof(struct ParseTree));

    if(node == NULL){
        printf("Erro ao alocar memória para o nó %d\n", type);
        return NULL;
    }

    node->node_type = type;
    node->sibling = NULL;
    for(int i=0; i<NUM_CHILDREN; i++) node->children[i] = NULL;

    return node;

}

struct ParseTree* create_op_terminal(TokenType op) {
    struct ParseTree* node = allocate_node(OP_TERMINAL_NODE);

    node->node_value.op_value = op;

    return node;
}

struct ParseTree* create_func_node(struct ParseTree* first_child, struct ParseTree* second_child, struct ParseTree* last_child){
    struct ParseTree* node = allocate_node(FUNC_NODE);

    node->children[0] = first_child;
    node->children[1] = second_child;
    node->children[2] = last_child;

    return node;
}

struct ParseTree* create_func_ativacao(struct ParseTree* first_child, struct ParseTree* second_child){
    struct ParseTree* node = allocate_node(FUNC_ACTV_NODE);

    node->children[0] = first_child;
    node->children[1] = second_child;

    return node;
}


struct ParseTree* create_num_node(int value){
    struct ParseTree* node = allocate_node(NUM_NODE);

    node->node_value.num_value = value;

    return node;
}

struct ParseTree* create_id_node(char* id_name){
    struct ParseTree* node = allocate_node(ID_NODE);

    node->node_value.id_name = strdup(id_name);

    return node;
}

struct ParseTree* create_var_node(struct ParseTree* first_child, struct ParseTree* second_child){
    // first child - variable id | second child - vector size (NULL if integer)
    struct ParseTree* node = allocate_node(VAR_NODE);

    node->children[0] = first_child;
    node->children[1] = second_child;

    return node;
}

struct ParseTree* create_param_node(struct ParseTree* first_child){
    // first child - variable id | second child - vector size (NULL if integer)
    struct ParseTree* node = allocate_node(FUNC_PARAM_NODE);

    node->children[0] = first_child;

    return node;
}

struct ParseTree* create_args_node(struct ParseTree* first_child){
    // first child - variable id | second child - vector size (NULL if integer)
    struct ParseTree* node = allocate_node(ARGS_NODE);

    node->children[0] = first_child;

    return node;
}

struct ParseTree* create_decl_list_node(struct ParseTree* first_child, struct ParseTree* second_child){
    struct ParseTree* node = allocate_node(DECL_LIST_NODE);

    node->children[0] = first_child;
    node->children[1] = second_child;

    return node;
}
struct ParseTree* create_op_node(struct ParseTree* first_child, struct ParseTree* second_child, struct ParseTree* last_child){
    // first child - op | second child 
    struct ParseTree* node = allocate_node(OP_NODE);
    

    node->children[0] = first_child;
    node->children[1] = second_child;
    node->children[2] = last_child;

    return node;
}

struct ParseTree* create_return_node(struct ParseTree* first_child){
    // first child - expressao
    struct ParseTree* node = allocate_node(RETURN_NODE);

    node->children[0] = first_child;

    return node;
}

struct ParseTree* create_if_node(struct ParseTree* first_child, struct ParseTree* second_child, struct ParseTree* last_child){
    struct ParseTree* node = allocate_node(IF_NODE);

    node->children[0] = first_child;
    node->children[1] = second_child;
    node->children[2] = last_child;

    return node;
}

struct ParseTree* create_while_node(struct ParseTree* first_child, struct ParseTree* second_child){
    // first child - expressao | second child - statement
    struct ParseTree* node = allocate_node(WHILE_NODE);

    node->children[0] = first_child;
    node->children[1] = second_child;

    return node;
}

struct ParseTree* create_assign_node(struct ParseTree* first_child, struct ParseTree* second_child){
    // first child - variable id | second child - expressão
    struct ParseTree* node = allocate_node(ASSIGN_NODE);

    node->children[0] = first_child;
    node->children[1] = second_child;

    return node;
}