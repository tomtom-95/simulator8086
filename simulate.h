#include "decode.h"

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

    GENERAL_REGISTER_COUNT,
};

enum GeneralRegisterForSimulation
{
    AL, AX,
    CL, CX,
    DL, DX,
    BL, BX,
    AH, SP,
    CH, BP,
    DH, SI,
    BH, DI,

    GENERAL_REGISTER_FOR_SIMULATION_COUNT,
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

u16 GeneralRegistersforSimulation[GENERAL_REGISTER_FOR_SIMULATION_COUNT];

u16 GeneralRegisters[GENERAL_REGISTER_COUNT];
u16 SegmentRegisters[SEGMENT_REGISTER_COUNT];
u16 InstructionPointer;
u16 Flags[FLAG_COUNT];
