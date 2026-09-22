#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#define INST0(_name) (instruction_t) { .type = INSTRUCTION_TYPE_##_name }
#define INST1(_name, _op0) (instruction_t) { .type = INSTRUCTION_TYPE_##_name, .operand_0 = _op0 }
#define INST2(_name, _op0, _op1) (instruction_t) { .type = INSTRUCTION_TYPE_##_name, .operand_0 = _op0, .operand_1 = _op1 }
#define INST3(_name, _op0, _op1, _op2) (instruction_t) { .type = INSTRUCTION_TYPE_##_name, .operand_0 = _op0, .operand_1 = _op1, .operand_2 = _op2 }

static constexpr uint64_t AVM_MEMORY_SIZE = 1024;
static constexpr uint8_t AVM_REGISTER_SIZE = 16;

typedef enum : uint8_t
{
    RESULT_OK = 0,
    RESULT_INVALID_INSTRUCTION,
    RESULT_INVALID_REGISTER,
    RESULT_INVALID_REGISTER_ACCESS,
    RESULT_INVALID_MEMORY_ACCESS,
    RESULT_INVALID_INSTRUCTION_ACCESS,
} result_t;

typedef enum : uint8_t
{
    INSTRUCTION_TYPE_NOP = 0,
    INSTRUCTION_TYPE_HALT,

    INSTRUCTION_TYPE_MOVE,

    INSTRUCTION_TYPE_SET,
    INSTRUCTION_TYPE_LOAD,

    INSTRUCTION_TYPE_STORE,

    INSTRUCTION_TYPE_ADD,
    INSTRUCTION_TYPE_SUB,
    INSTRUCTION_TYPE_MUL,
    INSTRUCTION_TYPE_DIV,
    INSTRUCTION_TYPE_MOD,

    INSTRUCTION_TYPE_EQU,
    INSTRUCTION_TYPE_NEQ,

    INSTRUCTION_TYPE_LTH,
    INSTRUCTION_TYPE_GTH,

    INSTRUCTION_TYPE_LEQ,
    INSTRUCTION_TYPE_GEQ,

    INSTRUCTION_TYPE_PRINT,

    INSTRUCTION_TYPE_JUMP,
    INSTRUCTION_TYPE_JUMP_IF,
} instruction_type_t;

typedef struct
{
    const instruction_type_t type;
    const int64_t operand_0;
    const int64_t operand_1;
    const int64_t operand_2;
} instruction_t;

typedef struct
{
    int64_t memory[AVM_MEMORY_SIZE];

    int64_t registers[AVM_REGISTER_SIZE];
    instruction_t* program;
    uint64_t program_size;
    uint64_t pip; // program instruction pointer
    bool halt;
} avm_t; // arctic virtual machine

static result_t avm_binary_op(avm_t* avm)
{
    const instruction_t* instruction = &avm->program[avm->pip];

    const int64_t operand_0 = instruction->operand_0;
    const int64_t operand_1 = instruction->operand_1;
    const int64_t operand_2 = instruction->operand_2;

    if (0 > operand_1 || operand_1 >= AVM_REGISTER_SIZE ||
        0 > operand_2 || operand_2 >= AVM_REGISTER_SIZE)
        return RESULT_INVALID_REGISTER_ACCESS;

    const int64_t left = avm->registers[operand_1];
    const int64_t right = avm->registers[operand_2];

    switch (instruction->type)
    {
        case INSTRUCTION_TYPE_ADD:
            avm->registers[operand_0] = left + right;
            break;
        case INSTRUCTION_TYPE_SUB:
            avm->registers[operand_0] = left - right;
            break;
        case INSTRUCTION_TYPE_MUL:
            avm->registers[operand_0] = left * right;
            break;
        case INSTRUCTION_TYPE_DIV:
            avm->registers[operand_0] = left / right;
            break;
        case INSTRUCTION_TYPE_MOD:
            avm->registers[operand_0] = left % right;
            break;
        case INSTRUCTION_TYPE_EQU:
            avm->registers[operand_0] = (int64_t)(left == right);
            break;
        case INSTRUCTION_TYPE_NEQ:
            avm->registers[operand_0] = (int64_t)(left != right);
            break;
        case INSTRUCTION_TYPE_LTH:
            avm->registers[operand_0] = (int64_t)(left < right);
            break;
        case INSTRUCTION_TYPE_GTH:
            avm->registers[operand_0] = (int64_t)(left > right);
            break;
        case INSTRUCTION_TYPE_LEQ:
            avm->registers[operand_0] = (int64_t)(left <= right);
            break;
        case INSTRUCTION_TYPE_GEQ:
            avm->registers[operand_0] = (int64_t)(left >= right);
            break;
        default:
            return RESULT_INVALID_INSTRUCTION;
    }

    avm->pip += 1;
    return RESULT_OK;
}

static result_t avm_instruction_execute(avm_t* avm)
{
    const instruction_t* instruction = &avm->program[avm->pip];

    switch (instruction->type)
    {
        case INSTRUCTION_TYPE_NOP:
            avm->pip += 1;
            return RESULT_OK;
        case INSTRUCTION_TYPE_SET:
            if (0 > instruction->operand_0 || instruction->operand_0 >= AVM_REGISTER_SIZE)
                return RESULT_INVALID_REGISTER_ACCESS;
            avm->registers[instruction->operand_0] = instruction->operand_1;
            avm->pip += 1;
            return RESULT_OK;
        case INSTRUCTION_TYPE_LOAD:
            if (0 > instruction->operand_0 || instruction->operand_0 >= AVM_REGISTER_SIZE)
                return RESULT_INVALID_REGISTER_ACCESS;
            if (0 > instruction->operand_1 || instruction->operand_1 >= AVM_MEMORY_SIZE)
                return RESULT_INVALID_MEMORY_ACCESS;
            avm->registers[instruction->operand_0] = avm->memory[instruction->operand_1];
            avm->pip += 1;
            return RESULT_OK;
        case INSTRUCTION_TYPE_STORE:
            if (0 > instruction->operand_0 || instruction->operand_0 >= AVM_REGISTER_SIZE)
                return RESULT_INVALID_REGISTER_ACCESS;
            if (0 > instruction->operand_1 || instruction->operand_1 >= AVM_MEMORY_SIZE)
                return RESULT_INVALID_MEMORY_ACCESS;
            avm->memory[instruction->operand_1] = avm->registers[instruction->operand_0];
            avm->pip += 1;
            return RESULT_OK;
        case INSTRUCTION_TYPE_MOVE:
            if (0 > instruction->operand_0 || instruction->operand_0 >= AVM_REGISTER_SIZE ||
                0 > instruction->operand_1 || instruction->operand_1 >= AVM_REGISTER_SIZE)
                return RESULT_INVALID_REGISTER_ACCESS;
            avm->registers[instruction->operand_0] = avm->registers[instruction->operand_1];
            avm->pip += 1;
            return RESULT_OK;
        case INSTRUCTION_TYPE_ADD:
        case INSTRUCTION_TYPE_SUB:
        case INSTRUCTION_TYPE_MUL:
        case INSTRUCTION_TYPE_DIV:
        case INSTRUCTION_TYPE_MOD:
        case INSTRUCTION_TYPE_EQU:
        case INSTRUCTION_TYPE_NEQ:
        case INSTRUCTION_TYPE_LTH:
        case INSTRUCTION_TYPE_GTH:
        case INSTRUCTION_TYPE_GEQ:
        case INSTRUCTION_TYPE_LEQ:
            return avm_binary_op(avm);
        case INSTRUCTION_TYPE_PRINT:
            if (0 > instruction->operand_0 || instruction->operand_0 >= AVM_REGISTER_SIZE)
                return RESULT_INVALID_REGISTER_ACCESS;
            printf("%" PRIi64 "\n", avm->registers[instruction->operand_0]);
            avm->pip += 1;
            return RESULT_OK;
        case INSTRUCTION_TYPE_HALT:
            avm->halt = true;
            return RESULT_OK;
        case INSTRUCTION_TYPE_JUMP:
            if (0 > instruction->operand_0 || instruction->operand_0 > avm->program_size)
                return RESULT_INVALID_INSTRUCTION_ACCESS;
            avm->pip = instruction->operand_0;
            return RESULT_OK;
        case INSTRUCTION_TYPE_JUMP_IF:
            if (0 > instruction->operand_0 || instruction->operand_0 >= AVM_REGISTER_SIZE)
                return RESULT_INVALID_REGISTER_ACCESS;
            if (avm->registers[instruction->operand_0] != 0)
            {
                if (0 > instruction->operand_0 || instruction->operand_0 > avm->program_size)
                    return RESULT_INVALID_INSTRUCTION_ACCESS;
                avm->pip = instruction->operand_1;
                return RESULT_OK;
            }
            avm->pip += 1;
            return RESULT_OK;
    }

    return RESULT_INVALID_INSTRUCTION;
}

static result_t avm_run(avm_t* avm)
{
    result_t result = RESULT_OK;

    while (!avm->halt && avm->pip < avm->program_size)
    {
        result = avm_instruction_execute(avm);
        if (result != RESULT_OK)
            avm->halt = true;
    }

    return result;
}

int main()
{
    avm_t avm = {0};

    avm.halt = false;

    /*
     * push r0 0
     * push r1 1
     * push r2 10
     * print r0
     * add r0 r0 r1
     * lth r3 r0 r2
     * jump_if r3 3
     */

    instruction_t program[] = {
        INST2(SET, 0, 0),
        INST2(SET, 1, 1),
        INST2(SET, 2, 10),
        INST1(PRINT, 0),
        INST3(ADD, 0, 0, 1),
        INST3(LTH, 3, 0, 2),
        INST2(JUMP_IF, 3, 3),
        INST0(HALT),
    };

    avm.program = program;
    avm.program_size = sizeof(program) / sizeof(program[0]);

    return avm_run(&avm);
}