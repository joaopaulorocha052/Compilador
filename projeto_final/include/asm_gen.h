#ifndef _ASM_GEN_
#define _ASM_GEN_


#ifdef DEBUG_BUILD
#define MAX_NUM_REGISTER 32
#else
#define MAX_NUM_REGISTER 32
#endif

#define RETURN_ADDRESS_POINTER MAX_NUM_REGISTER-1 
#define STACK_POINTER MAX_NUM_REGISTER-2
#define FRAME_POINTER MAX_NUM_REGISTER-3



typedef enum asm_opr
{
    ASM_REGISTER,
    ASM_NUMBER
}ASM_OPERAND_TYPE;

typedef enum asm_op_type
{
    ASM_ADD,
    ASM_ADDI,
    ASM_SUB,
    ASM_SUBI,
    ASM_MULT,
    ASM_EQ,
    ASM_DIV,
    ASM_JUMP,
    ASM_JAL,
    ASM_JR,
    ASM_BEQ,
    ASM_BNE,
    ASM_SW,
    ASM_LW,
    ASM_AND,
    ASM_OR,
    ASM_LI,
    ASM_SI,

}ASM_OPERATION;

typedef struct AsmOperand
{
    ASM_OPERAND_TYPE type;
    int operand;
} AsmOperand;


typedef struct AsmOperation
{
    ASM_OPERATION asm_operation_type;
    AsmOperand operands[3];
}AsmOperation;

#endif