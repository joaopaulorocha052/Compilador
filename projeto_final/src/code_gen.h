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



void print_quad(struct Quadrupla quad);
void print_tree_code_gen(struct ParseTree* tree);
static struct Quadrupla build_quad(QUADRUPLE_TYPES quad_type,
			    ADDR_TYPES first_address_type,
			    union ADDRESS fisrt_address_value,
			    ADDR_TYPES second_address_type,
			    union ADDRESS second_address_value,
			    ADDR_TYPES third_address_type,
			    union ADDRESS third_address_value
				   );
static struct Quadrupla build_var_quad(char* name);
const char* quadruple_type_to_string(QUADRUPLE_TYPES type);
char* gen_code(struct ParseTree* tree);
static struct Quadrupla build_op_quad(QUADRUPLE_TYPES op_type, char* name, ADDR_TYPES second_address_type, char * second_addres, ADDR_TYPES third_address_type, char * third_addres);
const QUADRUPLE_TYPES token_to_quad(int token);
#endif
