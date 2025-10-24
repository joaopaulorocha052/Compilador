#ifndef PARSE_H
#define PARSE_H


typedef enum {OP_NODE, NUM_NODE} NodeType;

// Estrutura da árvore

struct ParseTree
{
    NodeType node_type;
    union
    {
        char op_value;
        int num_value;
    } node_value;
    struct ParseTree* children[2];
    
};

void print_tree(struct ParseTree* tree);
struct ParseTree* create_new_node(int value, NodeType type);

#endif