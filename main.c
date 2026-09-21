#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

typedef enum
{
    INSTRUCTION_TYPE_NOP=0,
    INSTRUCTION_TYPE_PUSH,
    INSTRUCTION_TYPE_ADD,
    INSTRUCTION_TYPE_PRINT,
    INSTRUCTION_TYPE_HALT,
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
    int64_t registers[16];
    instruction_t* program;
    uint64_t pip; // program instruction pointer
    bool halt;
} avm_t; // arctic virtual machine

static void avm_run(avm_t* avm)
{
    while (!avm->halt)
    {
        const instruction_t* instruction = &avm->program[avm->pip];

        switch (instruction->type)
        {
            case INSTRUCTION_TYPE_NOP:
                avm->pip += 1;
                break;
            case INSTRUCTION_TYPE_PUSH:
                avm->registers[instruction->operand_0] = instruction->operand_1;
                avm->pip += 1;
                break;
            case INSTRUCTION_TYPE_ADD:
                const int64_t left = avm->registers[instruction->operand_1];
                const int64_t right = avm->registers[instruction->operand_2];
                avm->registers[instruction->operand_0] = left + right;
                avm->pip += 1;
                break;
            case INSTRUCTION_TYPE_PRINT:
                printf("%" PRIi64 "\n", avm->registers[instruction->operand_0]);
                avm->pip += 1;
                break;
            case INSTRUCTION_TYPE_HALT:
                avm->halt = true;
                break;
        }
    }
}

static instruction_t instructions_push(const int64_t target, const int64_t value)
{
    return (instruction_t) {
        .type = INSTRUCTION_TYPE_PUSH,
        .operand_0 = target,
        .operand_1 = value,
    };
}

static instruction_t instructions_add(const int64_t target, const int64_t left, const int64_t right)
{
    return (instruction_t) {
        .type = INSTRUCTION_TYPE_ADD,
        .operand_0 = target,
        .operand_1 = left,
        .operand_2 = right,
    };
}

static instruction_t instructions_print(const int64_t target)
{
    return (instruction_t) {
        .type = INSTRUCTION_TYPE_PRINT,
        .operand_0 = target,
    };
}

static instruction_t instructions_halt()
{
    return (instruction_t) {
        .type = INSTRUCTION_TYPE_HALT,
    };
}

// stack -> register
int main()
{
    instruction_t program[] = {
        instructions_push(0, 10),
        instructions_push(1, 5),
        instructions_add(2, 0, 1),
        instructions_print(2),
        instructions_halt(),
    };

    avm_t avm = {0};

    avm.halt = false;
    avm.program = program;

    avm_run(&avm);

    return 0;
}
