#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

static constexpr uint64_t AVM_MEMORY_SIZE = 1024;
static constexpr uint8_t AVM_REGISTER_SIZE = 16;

typedef enum
{
    INSTRUCTION_TYPE_NOP=0,
    INSTRUCTION_TYPE_HALT,

    INSTRUCTION_TYPE_MOVE,

    INSTRUCTION_TYPE_LOAD_IMMEDIATE,
    INSTRUCTION_TYPE_LOAD,

    INSTRUCTION_TYPE_STORE,

    INSTRUCTION_TYPE_ADD,

    INSTRUCTION_TYPE_PRINT,
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
            case INSTRUCTION_TYPE_LOAD_IMMEDIATE:
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
                const int64_t left = avm->registers[operand_1];
                const int64_t right = avm->registers[operand_2];
                avm->registers[operand_0] = left + right;
                avm->pip += 1;
                break;
            case INSTRUCTION_TYPE_PRINT:
                printf("%" PRIi64 "\n", avm->registers[operand_0]);
                avm->pip += 1;
                break;
            case INSTRUCTION_TYPE_HALT:
                avm->halt = true;
                break;
        }
    }
}

static instruction_t instructions_load_immediate(const int64_t target, const int64_t value)
{
    return (instruction_t) {
        .type = INSTRUCTION_TYPE_LOAD_IMMEDIATE,
        .operand_0 = target,
        .operand_1 = value,
    };
}

static instruction_t instructions_load(const int64_t target, const int64_t reg)
{
    return (instruction_t) {
        .type = INSTRUCTION_TYPE_LOAD,
        .operand_0 = target,
        .operand_1 = reg,
    };
}

static instruction_t instructions_store(const int64_t target, const int64_t value)
{
    return (instruction_t) {
        .type = INSTRUCTION_TYPE_LOAD,
        .operand_0 = target,
        .operand_1 = value,
    };
}

static instruction_t instructions_move(const int64_t target, const int64_t value)
{
    return (instruction_t) {
        .type = INSTRUCTION_TYPE_MOVE,
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

int main()
{
    avm_t avm = {0};

    constexpr uint64_t value_address = 69;

    avm.memory[value_address] = 10;
    avm.memory[value_address + 1] = 5;

    avm.halt = false;

    instruction_t program[] = {
        instructions_load_immediate(0, value_address),
        instructions_load(1, 0),
        instructions_load_immediate(0, value_address + 1),
        instructions_load(2, 0),
        instructions_add(0, 1, 2),
        instructions_print(0),
        instructions_halt(),
    };

    avm.program = program;

    avm_run(&avm);

    return 0;
}
