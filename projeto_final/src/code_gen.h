#ifndef CODE_GEN_H
#define CODE_GEN_H

typedef enum {
    Q_SOMA,
    Q_SUB,
    Q_DIV,
    Q_MULT,
    Q_MAIOR,
    Q_MAIOR_Q,
    Q_MENOR_Q,
    Q_IGUAL,
    Q_DIFF,
    Q_MENOR,
    Q_ASSIGN,
    Q_ASSIGN_VET,
    Q_ACCESS_VET,
    Q_IF,
    Q_GOTO,
    Q_INITVET,
    Q_INIT,
    Q_DEF,
    Q_CALL,
    Q_ARG,
    Q_RETURN,
    Q_HALT
} QUADRUPLE_TYPES;

typedef enum {
  VAZIO,
  REGIST,
  NAME,
  INT_NUM
} ADDR_TYPES;

struct ADDR{
  ADDR_TYPES type;
   union{
    int int_num;
    char* name;
  } ADDRESS;
};
struct Quadrupla{
  QUADRUPLE_TYPES type;
  struct ADDR addr1, addr2, addr3;
  char* msg;
};

void print_quad(struct Quadrupla quad);
void print_tree_code_gen(struct ParseTree* tree);

#endif
