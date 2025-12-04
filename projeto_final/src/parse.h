#ifndef PARSE_H
#define PARSE_H

#define  NUM_CHILDREN 3
#include "lexer.h"

typedef enum {ERROR_NODE, OP_TERMINAL_NODE, OP_NODE, NUM_NODE, VAR_NODE, ID_NODE, FUNC_NODE, FUNC_PARAM_NODE, DECL_LIST_NODE, UNKNOWN_NODE, ASSIGN_NODE, FUNC_ACTV_NODE, RETURN_NODE, IF_NODE, WHILE_NODE, ARGS_NODE} NodeType;
// Estrutura da árvore

struct ParseTree
{
    NodeType node_type;
    int line_num;
    union
    {
        int op_value; // TokenType
        int num_value;
        char *id_name;
    } node_value;
    struct ParseTree* sibling;
    struct ParseTree* children[NUM_CHILDREN];
    
};


struct ParseTree* SyntaticTree;
void print_tree(struct ParseTree* tree, int level) ;
static struct ParseTree* allocate_node(NodeType type);
static void free_node(struct ParseTree* node);
struct ParseTree* call_error();
struct ParseTree* create_error_node();
struct ParseTree* create_op_terminal(TokenType op);
struct ParseTree* create_op_node(struct ParseTree* first_child, struct ParseTree* second_child, struct ParseTree* last_child);
struct ParseTree* create_num_node(int value);
struct ParseTree* create_var_node(struct ParseTree* first_child, struct ParseTree* second_child);
struct ParseTree* create_id_node(char* id_name);
struct ParseTree* create_func_node(struct ParseTree* first_child, struct ParseTree* second_child, struct ParseTree* last_child);
struct ParseTree* create_param_node(struct ParseTree* first_child);
struct ParseTree* create_decl_list_node(struct ParseTree* first_child, struct ParseTree* second_child);
struct ParseTree* create_assign_node(struct ParseTree* first_child, struct ParseTree* second_child);
struct ParseTree* create_func_ativacao(struct ParseTree* first_child, struct ParseTree* second_child);
struct ParseTree* create_return_node(struct ParseTree* first_child);
struct ParseTree* create_if_node(struct ParseTree* first_child, struct ParseTree* second_child, struct ParseTree* last_child);
struct ParseTree* create_while_node(struct ParseTree* first_child, struct ParseTree* second_child);
struct ParseTree* create_args_node(struct ParseTree* first_child);
#endif