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
    OPERAND_DIRECTION_NONE,

    DESTINATION,
    SOURCE,
};

enum OperandWideness
{
    OPERAND_WIDENESS_NONE,

    BYTE_WIDE,
    WORD_WIDE,
};

struct Operand
{
    enum OperandId id;
    enum OperandDirection dir;
    enum OperandWideness wideness;
    char str[32];
    u16 value;
};

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
    u8 value;
    u8 len;
};

struct Instruction
{
    enum MnemonicId mnemonic_id;
    char mnemonic_str[8];
    struct Field fields[FIELD_ID_COUNT]; 
    struct Operand operands[2];
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

char *sr_field_enc[] = {
    "es",
    "cs",
    "ss",
    "ds",
};

#define OPCODE1(bits) {FIELD_ID_OPCODE1, 0b##bits, sizeof(#bits) - 1}
#define OPCODE2(bits) {FIELD_ID_OPCODE2, 0b##bits, sizeof(#bits) - 1}

#define D {FIELD_ID_D, 0, 1}
#define W {FIELD_ID_W, 0, 1}
#define S {FIELD_ID_S, 0, 1}

#define MOD {FIELD_ID_MOD, 0, 2}
#define REG {FIELD_ID_REG, 0, 3}
#define RM {FIELD_ID_RM, 0, 3}
#define SR {FIELD_ID_SR, 0, 2}

#define END {FIELD_ID_END, 0, 0}


#define PREFIX(prefix_id, prefix_str, opcode) {prefix_id, #prefix_str, 0b##opcode}
#define INST(mnemonic_id, mnemonic_str, ...) {mnemonic_id, #mnemonic_str, __VA_ARGS__}

struct Instruction instruction_decode(u8 **memory);

#endif // DECODE_H