#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static constexpr uint64_t AVM_MEMORY_SIZE = 1024;
static constexpr uint8_t AVM_REGISTER_SIZE = 16;
static constexpr uint64_t AVM_INSTRUCTION_SERIAL_SIZE = 1 + 3 * sizeof(int64_t);


typedef enum : uint8_t
{
    RESULT_OK = 0,
    RESULT_DIVISION_BY_ZERO,
    RESULT_INVALID_INSTRUCTION,
    RESULT_INVALID_REGISTER,
    RESULT_INVALID_REGISTER_ACCESS,
    RESULT_INVALID_MEMORY_ACCESS,
    RESULT_INVALID_INSTRUCTION_ACCESS,
    RESULT_INVALID_SHIFT_OFFSET,
    RESULT_IO_ERROR,
    RESULT_INVALID_FILE_FORMAT,
} result_t;

typedef enum : uint8_t
{
    INSTRUCTION_TYPE_NOP = 0,
    INSTRUCTION_TYPE_HALT,

    INSTRUCTION_TYPE_MOVE,

    INSTRUCTION_TYPE_SET,
    INSTRUCTION_TYPE_LOAD,

    INSTRUCTION_TYPE_STORE,

    INSTRUCTION_TYPE_PRINT,

    INSTRUCTION_TYPE_JUMP,
    INSTRUCTION_TYPE_JUMP_IF,

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

    INSTRUCTION_TYPE_AND,
    INSTRUCTION_TYPE_OR,
    INSTRUCTION_TYPE_XOR,
    INSTRUCTION_TYPE_NOT,
    INSTRUCTION_TYPE_SHL,
    INSTRUCTION_TYPE_SHR,
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
    int64_t memory[AVM_MEMORY_SIZE];
    int64_t registers[AVM_REGISTER_SIZE];
    instruction_t* program;
    uint64_t program_size;
    uint64_t pip;
    bool halt;
} avm_t;

static bool target_ok(const int64_t target, const uint32_t size) { return 0 <= target && (uint64_t)target < size; }

static bool register_ok(const int64_t _register) { return target_ok(_register, AVM_REGISTER_SIZE); }

static bool memory_ok(const int64_t address) { return target_ok(address, AVM_MEMORY_SIZE); }

static const char* result_to_cstr(const result_t result)
{
    switch (result)
    {
        case RESULT_OK: return "RESULT_OK";
        case RESULT_DIVISION_BY_ZERO: return "RESULT_DIVISION_BY_ZERO";
        case RESULT_INVALID_INSTRUCTION: return "RESULT_INVALID_INSTRUCTION";
        case RESULT_INVALID_REGISTER: return "RESULT_INVALID_REGISTER";
        case RESULT_INVALID_REGISTER_ACCESS: return "RESULT_INVALID_REGISTER_ACCESS";
        case RESULT_INVALID_MEMORY_ACCESS: return "RESULT_INVALID_MEMORY_ACCESS";
        case RESULT_INVALID_INSTRUCTION_ACCESS: return "RESULT_INVALID_INSTRUCTION_ACCESS";
        case RESULT_INVALID_SHIFT_OFFSET: return "RESULT_INVALID_SHIFT_OFFSET";
        case RESULT_IO_ERROR: return "RESULT_IO_ERROR";
        case RESULT_INVALID_FILE_FORMAT: return "RESULT_INVALID_FILE_FORMAT";
    }

    return "RESULT_UNKNOWN";
}

static result_t avm_binary_op(avm_t* avm)
{
    const instruction_t* instruction = &avm->program[avm->pip];

    const int64_t operand_0 = instruction->operand_0;
    const int64_t operand_1 = instruction->operand_1;
    const int64_t operand_2 = instruction->operand_2;

    if (!register_ok(operand_0) ||
        !register_ok(operand_1) ||
        !register_ok(operand_2))
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
            if (right == 0) return RESULT_DIVISION_BY_ZERO;
            avm->registers[operand_0] = left / right;
            break;
        case INSTRUCTION_TYPE_MOD:
            if (right == 0) return RESULT_DIVISION_BY_ZERO;
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
        case INSTRUCTION_TYPE_AND:
            avm->registers[operand_0] = left & right;
            break;
        case INSTRUCTION_TYPE_OR:
            avm->registers[operand_0] = left | right;
            break;
        case INSTRUCTION_TYPE_XOR:
            avm->registers[operand_0] = left ^ right;
            break;
        case INSTRUCTION_TYPE_SHL:
            if (0 > right || right >= 64) return RESULT_INVALID_SHIFT_OFFSET;
            avm->registers[operand_0] = (int64_t)((uint64_t)left << right);
            break;
        case INSTRUCTION_TYPE_SHR:
            if (0 > right || right >= 64) return RESULT_INVALID_SHIFT_OFFSET;
            avm->registers[operand_0] = (int64_t)((uint64_t)left >> right);
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
            if (!register_ok(instruction->operand_0)) return RESULT_INVALID_REGISTER_ACCESS;
            avm->registers[instruction->operand_0] = instruction->operand_1;
            avm->pip += 1;
            return RESULT_OK;
        case INSTRUCTION_TYPE_LOAD:
            if (!register_ok(instruction->operand_0)) return RESULT_INVALID_REGISTER_ACCESS;
            if (!memory_ok(instruction->operand_1)) return RESULT_INVALID_MEMORY_ACCESS;
            avm->registers[instruction->operand_0] = avm->memory[instruction->operand_1];
            avm->pip += 1;
            return RESULT_OK;
        case INSTRUCTION_TYPE_STORE:
            if (!register_ok(instruction->operand_0)) return RESULT_INVALID_REGISTER_ACCESS;
            if (!memory_ok(instruction->operand_1)) return RESULT_INVALID_MEMORY_ACCESS;
            avm->memory[instruction->operand_1] = avm->registers[instruction->operand_0];
            avm->pip += 1;
            return RESULT_OK;
        case INSTRUCTION_TYPE_MOVE:
            if (!register_ok(instruction->operand_0) ||
                !register_ok(instruction->operand_1))
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
        case INSTRUCTION_TYPE_AND:
        case INSTRUCTION_TYPE_OR:
        case INSTRUCTION_TYPE_XOR:
        case INSTRUCTION_TYPE_SHL:
        case INSTRUCTION_TYPE_SHR:
            return avm_binary_op(avm);
        case INSTRUCTION_TYPE_NOT:
            if (!register_ok(instruction->operand_0) ||
                !register_ok(instruction->operand_1))
                return RESULT_INVALID_REGISTER_ACCESS;
            avm->registers[instruction->operand_0] = ~avm->registers[instruction->operand_1];
            avm->pip += 1;
            return RESULT_OK;
        case INSTRUCTION_TYPE_PRINT:
            if (!register_ok(instruction->operand_0)) return RESULT_INVALID_REGISTER_ACCESS;
            printf("%" PRIi64 "\n", avm->registers[instruction->operand_0]);
            avm->pip += 1;
            return RESULT_OK;
        case INSTRUCTION_TYPE_HALT:
            avm->halt = true;
            return RESULT_OK;
        case INSTRUCTION_TYPE_JUMP:
            if (!target_ok(instruction->operand_0, avm->program_size))
                return RESULT_INVALID_INSTRUCTION_ACCESS;
            avm->pip = instruction->operand_0;
            return RESULT_OK;
        case INSTRUCTION_TYPE_JUMP_IF:
            if (!register_ok(instruction->operand_0))
                return RESULT_INVALID_REGISTER_ACCESS;
            if (avm->registers[instruction->operand_0] != 0)
            {
                if (!target_ok(instruction->operand_1, avm->program_size))
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

static int64_t avm_read_i64_le(const uint8_t* bytes)
{
    uint64_t value = 0;

    for (uint64_t index = 0; index < sizeof(int64_t); index += 1)
        value |= (uint64_t)bytes[index] << (index * 8);

    return (int64_t)value;
}

static uint8_t* avm_file_read(const char* path, uint64_t* size)
{
    FILE* file = fopen(path, "rb");

    if (file == nullptr)
        return nullptr;

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return nullptr;
    }

    const long file_size = ftell(file);

    if (file_size < 0)
    {
        fclose(file);
        return nullptr;
    }

    rewind(file);

    uint8_t* data = malloc((size_t)file_size);

    if (data == nullptr)
    {
        fclose(file);
        return nullptr;
    }

    if (fread(data, 1, (size_t)file_size, file) != (size_t)file_size)
    {
        free(data);
        fclose(file);
        return nullptr;
    }

    fclose(file);
    *size = (uint64_t)file_size;
    return data;
}

static result_t avm_program_load(avm_t* avm, const char* path)
{
    uint64_t serial_size = 0;

    uint8_t* serial = avm_file_read(path, &serial_size);

    if (serial == nullptr) return RESULT_IO_ERROR;

    if (serial_size == 0 || serial_size % AVM_INSTRUCTION_SERIAL_SIZE != 0)
    {
        free(serial);
        return RESULT_INVALID_FILE_FORMAT;
    }

    const uint64_t program_size = serial_size / AVM_INSTRUCTION_SERIAL_SIZE;

    const auto program = (instruction_t*)malloc((size_t)program_size * sizeof(instruction_t));

    if (program == nullptr)
    {
        free(serial);
        return RESULT_IO_ERROR;
    }

    for (uint64_t index = 0; index < program_size; index += 1)
    {
        const uint8_t* record = serial + index * AVM_INSTRUCTION_SERIAL_SIZE;

        if (record[0] > (uint8_t)INSTRUCTION_TYPE_SHR)
        {
            free(program);
            free(serial);
            return RESULT_INVALID_FILE_FORMAT;
        }

        program[index] = (instruction_t) {
            .type = (instruction_type_t)record[0],
            .operand_0 = avm_read_i64_le(record + 1),
            .operand_1 = avm_read_i64_le(record + 9),
            .operand_2 = avm_read_i64_le(record + 17),
        };
    }

    free(serial);

    avm->program = program;
    avm->program_size = program_size;

    return RESULT_OK;
}

int main()
{
    avm_t avm = {};

    result_t result = avm_program_load(&avm, "../main.avmb");

    if (result == RESULT_OK)
        result = avm_run(&avm);

    if (result != RESULT_OK)
        fprintf(stderr, "%s(%d) at pip=%" PRIu64 "\n",
                result_to_cstr(result), result, avm.pip);

    free(avm.program);

    return result;
}
