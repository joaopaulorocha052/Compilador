#ifndef PARSE_H
#define PARSE_H

#define  NUM_CHILDREN 3


typedef enum {OP_NODE, NUM_NODE, VAR_NODE, ID_NODE, FUNC_NODE, FUNC_PARAM_NODE, FUNC_BODY_NODE, UNKNOWN_NODE} NodeType;
typedef enum {INT_EXP, VOID_EXP} ExpType;
// Estrutura da árvore

struct ParseTree
{
    NodeType node_type;
    ExpType exp_type;
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
struct ParseTree* create_new_node(int value, NodeType type, char* name, struct ParseTree* first_child, struct ParseTree* second_child, struct ParseTree* last_child);
struct ParseTree* create_id_node(char* id_name);
struct ParseTree* create_num_node(int value);
struct ParseTree* create_var_node(struct ParseTree* first_child, struct ParseTree* second_child);
struct ParseTree* create_func_node(struct ParseTree* first_child, struct ParseTree* second_child, struct ParseTree* last_child);

#endif