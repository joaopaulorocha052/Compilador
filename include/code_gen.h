#ifndef CODE_GEN_H
#define CODE_GEN_H


extern int temporary_variable;

typedef enum {
    Q_SOMA = 0,
    Q_SUB,
    Q_DIV,
    Q_MULT,
    Q_MAIOR,
    Q_MAIOR_Q,
    Q_MENOR_Q,
    Q_IGUAL,
    Q_DIFF,
    Q_MENOR,
    Q_ASSIGN, // x
    Q_ASSIGN_VET,
    Q_ACCESS_VET,
    Q_IF,
    Q_GOTO,
    Q_LABEL,
    Q_INITVET,// x
    Q_INIT, // x
    Q_DEF,
    Q_CALL,
    Q_ARG,
    Q_PARAM,
    Q_PARAM_END,
    Q_RETURN,
    Q_FUNCLABEL,
    Q_FUNCEND,
    Q_HALT
} QUADRUPLE_TYPES;

typedef enum {
  VAZIO,
  REGIST,
  NAME,
  INT_NUM
} ADDR_TYPES;

union ADDRESS{
    int int_num;
    char* name;
  };

struct ADDR{
  ADDR_TYPES type;
  union ADDRESS value;
   
};
struct Quadrupla{
  QUADRUPLE_TYPES type;
  struct ADDR addr1, addr2, addr3;
  char* msg;
};

struct ListNode{
  struct Quadrupla quad;
  struct ListNode* next;

};

struct QuadrupleList{
  struct ListNode* list;
  struct ListNode* tail;
};


extern struct QuadrupleList* quadruples;
extern struct Quadrupla* head;

static struct Quadrupla create_quad(QUADRUPLE_TYPES quad_type,
			    struct ADDR first_address_value,
			    struct ADDR second_address_value,
			    struct ADDR third_address_value
			    );
int emit_quad(QUADRUPLE_TYPES quad_type, char* first_value, char* second_value, char* third_value);
void print_quad(struct Quadrupla quad);
void print_tree_code_gen(struct ParseTree* tree);
const char* quadruple_type_to_string(QUADRUPLE_TYPES type);
char* gen_code(struct ParseTree* tree);
const QUADRUPLE_TYPES token_to_quad(int token);
#endif