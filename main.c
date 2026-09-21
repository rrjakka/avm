#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

static constexpr uint64_t AVM_MEMORY_SIZE = 1024;
static constexpr uint8_t AVM_REGISTER_SIZE = 16;

typedef enum
{
    INSTRUCTION_TYPE_NOP = 0,
    INSTRUCTION_TYPE_HALT,

    INSTRUCTION_TYPE_MOVE,

    INSTRUCTION_TYPE_PUSH,
    INSTRUCTION_TYPE_LOAD,

    INSTRUCTION_TYPE_STORE,

    INSTRUCTION_TYPE_ADD,
    INSTRUCTION_TYPE_SUB,
    INSTRUCTION_TYPE_MUL,
    INSTRUCTION_TYPE_DIV,
    INSTRUCTION_TYPE_MOD,

    INSTRUCTION_TYPE_EQU,
    INSTRUCTION_TYPE_NEQ,

    INSTRUCTION_TYPE_PRINT,

    INSTRUCTION_TYPE_JUMP,
    INSTRUCTION_TYPE_JUMP_IF,
} instruction_type_t;

typedef struct
{
    instruction_type_t type;
    int64_t operand_0;
    int64_t operand_1;
    int64_t operand_2;
} instruction_t;

typedef struct
{
    int64_t registers[AVM_REGISTER_SIZE];
    instruction_t* program;
    uint64_t pip; // program instruction pointer
    bool halt;

    int64_t memory[AVM_MEMORY_SIZE];
} avm_t; // arctic virtual machine

static void avm_binary_op(avm_t* avm)
{
    const instruction_t* instruction = &avm->program[avm->pip];

    const int64_t operand_0 = instruction->operand_0;
    const int64_t operand_1 = instruction->operand_1;
    const int64_t operand_2 = instruction->operand_2;


    const int64_t left = avm->registers[operand_1];
    const int64_t right = avm->registers[operand_2];
    avm->registers[operand_0] = left + right;

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
        default:
            // TODO: return RESULT_INVALID_OP
            break;
    }

    avm->pip += 1;
}

static void avm_run(avm_t* avm)
{
    while (!avm->halt)
    {
        const instruction_t* instruction = &avm->program[avm->pip];

        const int64_t operand_0 = instruction->operand_0;
        const int64_t operand_1 = instruction->operand_1;
        const int64_t operand_2 = instruction->operand_2;

        switch (instruction->type)
        {
            case INSTRUCTION_TYPE_NOP:
                avm->pip += 1;
                break;
            case INSTRUCTION_TYPE_PUSH:
                avm->registers[operand_0] = operand_1;
                avm->pip += 1;
                break;
            case INSTRUCTION_TYPE_LOAD:
                avm->registers[operand_0] = avm->memory[avm->registers[operand_1]];
                avm->pip += 1;
                break;
            case INSTRUCTION_TYPE_STORE:
                avm->memory[avm->registers[operand_0]] = avm->registers[operand_1];
                avm->pip += 1;
                break;
            case INSTRUCTION_TYPE_MOVE:
                avm->registers[operand_0] = avm->registers[operand_1];
                avm->pip += 1;
                break;
            case INSTRUCTION_TYPE_ADD:
            case INSTRUCTION_TYPE_SUB:
            case INSTRUCTION_TYPE_MUL:
            case INSTRUCTION_TYPE_DIV:
            case INSTRUCTION_TYPE_MOD:
            case INSTRUCTION_TYPE_EQU:
            case INSTRUCTION_TYPE_NEQ:
                avm_binary_op(avm);
                break;
            case INSTRUCTION_TYPE_PRINT:
                printf("%" PRIi64 "\n", avm->registers[operand_0]);
                avm->pip += 1;
                break;
            case INSTRUCTION_TYPE_HALT:
                avm->halt = true;
                break;
            case INSTRUCTION_TYPE_JUMP:
                avm->pip = operand_0;
                break;
            case INSTRUCTION_TYPE_JUMP_IF:
                if (avm->registers[operand_0] != 0)
                    avm->pip = operand_1;
                else
                    avm->pip += 1;
                break;
        }
    }
}

static instruction_t instruction_push(const int64_t target, const int64_t value)
{
    return (instruction_t){
        .type = INSTRUCTION_TYPE_PUSH,
        .operand_0 = target,
        .operand_1 = value,
    };
}

static instruction_t instruction_load(const int64_t address, const int64_t reg)
{
    return (instruction_t){
        .type = INSTRUCTION_TYPE_LOAD,
        .operand_0 = address,
        .operand_1 = reg,
    };
}

static instruction_t instruction_store(const int64_t target, const int64_t address)
{
    return (instruction_t){
        .type = INSTRUCTION_TYPE_STORE,
        .operand_0 = target,
        .operand_1 = address,
    };
}

static instruction_t instruction_move(const int64_t target, const int64_t value)
{
    return (instruction_t){
        .type = INSTRUCTION_TYPE_MOVE,
        .operand_0 = target,
        .operand_1 = value,
    };
}

static instruction_t instruction_add(const int64_t target, const int64_t left, const int64_t right)
{
    return (instruction_t){
        .type = INSTRUCTION_TYPE_ADD,
        .operand_0 = target,
        .operand_1 = left,
        .operand_2 = right,
    };
}

static instruction_t instruction_equ(const int64_t target, const int64_t left, const int64_t right)
{
    return (instruction_t){
        .type = INSTRUCTION_TYPE_EQU,
        .operand_0 = target,
        .operand_1 = left,
        .operand_2 = right,
    };
}

static instruction_t instruction_neq(const int64_t target, const int64_t left, const int64_t right)
{
    return (instruction_t){
        .type = INSTRUCTION_TYPE_NEQ,
        .operand_0 = target,
        .operand_1 = left,
        .operand_2 = right,
    };
}

static instruction_t instruction_print(const int64_t target)
{
    return (instruction_t){
        .type = INSTRUCTION_TYPE_PRINT,
        .operand_0 = target,
    };
}

static instruction_t instruction_halt()
{
    return (instruction_t){
        .type = INSTRUCTION_TYPE_HALT,
    };
}

static instruction_t instruction_jump(const int64_t address)
{
    return (instruction_t){
        .type = INSTRUCTION_TYPE_JUMP,
        .operand_0 = address,
    };
}

static instruction_t instruction_jump_if(const int64_t reg, const int64_t address)
{
    return (instruction_t){
        .type = INSTRUCTION_TYPE_JUMP_IF,
        .operand_0 = reg,
        .operand_1 = address,
    };
}

int main()
{
    avm_t avm = {0};

    avm.halt = false;

    constexpr uint64_t ADDRESS = 69;

    avm.memory[ADDRESS] = 0;

    instruction_t program[] = {
        instruction_load(0, ADDRESS),
        instruction_push(1, 1),
        instruction_push(2, 10),
        instruction_add(3, 0, 1),
        instruction_store(ADDRESS, 3),
        instruction_neq(4, 0, 2),
        instruction_print(3),
        instruction_jump_if(4, 0),
        instruction_halt(),
    };

    avm.program = program;

    avm_run(&avm);

    return 0;
}
