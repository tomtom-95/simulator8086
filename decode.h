#ifndef DECODE_H
#define DECODE_H

#include <stdint.h>

#define KB(num) (1024 * num)
#define MB(num) (1024 * KB(num))

#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

typedef u_int8_t u8;
typedef u_int16_t u16;
typedef u_int32_t u32;
typedef u_int64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

enum MnemonicId
{
    MNEMONIC_ID_NONE,

    MOV,
    PUSH,
    POP,
    XCHG,
    IN,
    OUT,
    XLAT,
    LEA,
    LDS,
    LES,
    LAHF,
    SAHF,
    PUSHF,
    POPF,
    ADD,
    ADC,
    INC,
    AAA,
    DAA,
    SUB,
    SBB,
    DEC,
    NEG,
    CMP,
    JZ,
    JL,
    JLE,
    JB,
    JBE,
    JP,
    JO,
    JS,
    JNE,
    JNL,
    JNLE,
    JNB,
    JNBE,
    JNP,
    JNO,
    JNS,
    LOOP,
    LOOPZ,
    LOOPNZ,
    JCXZ,

    MNEMONIC_ID_COUNT,
};

enum GeneralRegisterName
{
    REGISTER_AX,
    REGISTER_BX,
    REGISTER_CX,
    REGISTER_DX,
    REGISTER_SP,
    REGISTER_BP,
    REGISTER_SI,
    REGISTER_DI,
    REGISTER_ZERO,

    GENERAL_REGISTER_COUNT,
};

u8 *general_registers;

u8 reg_field_encoding[8][2] = {
    {2 * REGISTER_AX,     2 * REGISTER_AX},
    {2 * REGISTER_CX,     2 * REGISTER_CX},
    {2 * REGISTER_DX,     2 * REGISTER_DX},
    {2 * REGISTER_BX,     2 * REGISTER_BX},
    {2 * REGISTER_AX + 1, 2 * REGISTER_SP},
    {2 * REGISTER_CX + 1, 2 * REGISTER_BP},
    {2 * REGISTER_DX + 1, 2 * REGISTER_SI},
    {2 * REGISTER_BX + 1, 2 * REGISTER_DI},
};

u8 rm_field_encoding[8][2] = {
    {2 * REGISTER_BX, 2 * REGISTER_SI},
    {2 * REGISTER_BX, 2 * REGISTER_DI},
    {2 * REGISTER_BP, 2 * REGISTER_SI},
    {2 * REGISTER_BP, 2 * REGISTER_DI},
    {2 * REGISTER_SI, 2 * REGISTER_ZERO},
    {2 * REGISTER_DI, 2 * REGISTER_ZERO},
    {2 * REGISTER_BP, 2 * REGISTER_ZERO},
    {2 * REGISTER_BX, 2 * REGISTER_ZERO},
};


char *general_registers_names[] = {
    "ax",
    "bx",
    "cx",
    "dx",
    "sp",
    "bp",
    "si",
    "di",
};

char *reg_field_str[][2] = {
    {"al", "ax"},
    {"cl", "cx"},
    {"dl", "dx"},
    {"bl", "bx"},
    {"ah", "sp"},
    {"ch", "bp"},
    {"dh", "si"},
    {"bh", "di"},
};

char *rm_field_str[] = {
    "bx+si",
    "bx+di",
    "bp+si",
    "bp+di",
    "si",
    "di",
    "bp",
    "bx",
};

enum SegmentRegisterName
{
    REGISTER_ES,
    REGISTER_CS,
    REGISTER_SS,
    REGISTER_DS,

    SEGMENT_REGISTER_COUNT,
};

u16 *segment_registers;

char *sr_field_str[] = {
    "es",
    "cs",
    "ss",
    "ds",
};

// use for indexing Flags
enum FlagName
{
    FLAG_CF = 0,
    FLAG_PF = 2,
    FLAG_AF = 4,
    FLAG_ZF = 6,
    FLAG_SF = 7,
    FLAG_TF = 8,
    FLAG_IF = 9,
    FLAG_DF = 10,
    FLAG_OF = 11,

    FLAG_COUNT,
};
u16 Flags;

u16 InstructionPointer;
u16 StackPointer;

enum FieldId
{
    FIELD_ID_NONE,

    FIELD_ID_OPCODE1,
    FIELD_ID_OPCODE2,
    FIELD_ID_S,
    FIELD_ID_W,
    FIELD_ID_D,
    FIELD_ID_V,
    FIELD_ID_Z,
    FIELD_ID_MOD,
    FIELD_ID_REG,
    FIELD_ID_RM,
    FIELD_ID_SR,
    FIELD_ID_XXX,
    FIELD_ID_YYY,
    FIELD_ID_END,

    FIELD_ID_COUNT,
};

struct Field
{
    enum FieldId id;
    u8 len;
    u8 value;
};

enum OperandId
{
    OPERAND_ID_NONE,

    OPERAND_ID_REG,
    OPERAND_ID_RM,
    OPERAND_ID_IMMEDIATE,
    OPERAND_ID_ADDRESS,
    OPERAND_ID_ACCUMULATOR,
    OPERAND_ID_SR,
    OPERAND_ID_IPINC8,
    OPERAND_ID_DATA8,
    OPERAND_ID_DX,
};

enum OperandDirection
{
    DESTINATION,
    SOURCE,
};

struct Operand
{
    enum OperandId id;
    enum OperandDirection dir;
    char str[32];
    u8 *location;
};

struct Instruction
{
    enum MnemonicId mnemonic_id;
    char mnemonic_str[16];
    struct Field fields[FIELD_ID_COUNT]; 
    struct Operand operands[2];
};

union DataPointer
{
    u8 *memory_pointer;
    u16 *register_pointer;
};

enum PrefixId
{
    PREFIX_ID_NONE,

    PREFIX_ID_REP,
    PREFIX_ID_REPNE,
    PREFIX_ID_LOCK,
    PREFIX_ID_SEGMENT_ES,
    PREFIX_ID_SEGMENT_CS,
    PREFIX_ID_SEGMENT_SS,
    PREFIX_ID_SEGMENT_DS,

    PREFIX_ID_COUNT,
};

struct Prefix
{
    enum PrefixId prefix_id;
    char prefix_str[8];
    u8 prefix_value;
};

#define OPCODE1(bits) {FIELD_ID_OPCODE1, sizeof(#bits) - 1, 0b##bits}
#define OPCODE2(bits) {FIELD_ID_OPCODE2, sizeof(#bits) - 1, 0b##bits}

#define D {FIELD_ID_D, 1}
#define W {FIELD_ID_W, 1}
#define S {FIELD_ID_S, 1}

#define ImplW(bit) {FIELD_ID_W, 0, bit}

#define MOD {FIELD_ID_MOD, 2}
#define REG {FIELD_ID_REG, 3}
#define RM {FIELD_ID_RM, 3}
#define SR {FIELD_ID_SR, 2}

#define END {FIELD_ID_END}


#define PREFIX(prefix_id, prefix_str, opcode) {prefix_id, #prefix_str, 0b##opcode}
#define INST(mnemonic_id, mnemonic_str, ...) {mnemonic_id, #mnemonic_str, __VA_ARGS__}

struct Instruction instruction_decode(u8 *memory);

#endif // DECODE_H