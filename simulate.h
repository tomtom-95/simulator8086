#include "decode.h"

struct SegmentRegisters
{
    u8 *register_cs;
    u8 *register_ds;
    u8 *register_ss;
    u8 *register_es;
};

enum GeneralRegisterName
{
    REGISTER_NONE,

    REGISTER_AX,
    REGISTER_BX,
    REGISTER_CX,
    REGISTER_DX,
    REGISTER_SP,
    REGISTER_BP,
    REGISTER_SI,
    REGISTER_DI,

    GENERAL_REGISTER_COUNT,
};

enum SegmentRegisterName
{
    REGISTER_CS,
    REGISTER_DS,
    REGISTER_SS,
    REGISTER_ES,

    SEGMENT_REGISTER_COUNT,
};

enum FlagName
{
    FLAG_CF,
    FLAG_PF,
    FLAG_AF,
    FLAG_ZF,
    FLAG_SF,
    FLAG_OF,
    FLAG_IF,
    FLAG_DF,
    FLAG_TF,

    FLAG_COUNT,
};

u16 GeneralRegisters[GENERAL_REGISTER_COUNT];
u16 SegmentRegisters[SEGMENT_REGISTER_COUNT];
u16 InstructionPointer;
u16 StackPointer;
u16 Flags[FLAG_COUNT];

enum ByteOffset
{
    LOW_BYTE,
    HIGH_BYTE,
};

u8 reg_field_enc[][2][2] = {
    {{REGISTER_AX, LOW_BYTE}, {REGISTER_AX}},
    {{REGISTER_CX, LOW_BYTE}, {REGISTER_CX}},
    {{REGISTER_DX, LOW_BYTE}, {REGISTER_DX}},
    {{REGISTER_BX, LOW_BYTE}, {REGISTER_BX}},
    {{REGISTER_AX, HIGH_BYTE}, {REGISTER_SP}},
    {{REGISTER_CX, HIGH_BYTE}, {REGISTER_BP}},
    {{REGISTER_DX, HIGH_BYTE}, {REGISTER_SI}},
    {{REGISTER_BX, HIGH_BYTE}, {REGISTER_DI}},
};

u8 rm_field_enc[][2] = {
    {REGISTER_BX, REGISTER_SI},
    {REGISTER_BX, REGISTER_DI},
    {REGISTER_BP, REGISTER_SI},
    {REGISTER_BP, REGISTER_DI},
    {REGISTER_SI, REGISTER_NONE},
    {REGISTER_DI, REGISTER_NONE},
    {REGISTER_BP, REGISTER_NONE},
    {REGISTER_BX, REGISTER_NONE},
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
