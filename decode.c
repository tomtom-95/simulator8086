#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "decode.h"
#include "instruction_table.h"

struct Instruction instruction_decode(u8 *memory)
{
    struct Instruction instruction = {MNEMONIC_ID_NONE, "", {FIELD_ID_NONE}, {{OPERAND_ID_NONE}, {OPERAND_ID_NONE}}};

    u32 code_segment_base_address = ((u32)(segment_registers_start[REGISTER_CS])) << 4;
    u32 data_segment_base_address = ((u32)(segment_registers_start[REGISTER_DS])) << 4;

    for (size_t i = 0; i < ArrayCount(instruction_table); ++i)
    {
        u8 *tmp_memory = memory + code_segment_base_address + InstructionPointer;
        u8 field_offset = 0;
        size_t j = 0;
        for (;;)
        {
            struct Field field = instruction_table[i].fields[j];

            u8 mask = 0;
            for (size_t k = 0; k < field.len; ++k)
            {
                mask |= (1 << k);
            }

            if (field.id == FIELD_ID_OPCODE1 || field.id == FIELD_ID_OPCODE2)
            {
                if (((*tmp_memory >> (8 - field.len - field_offset)) & mask) == field.value)
                {
                    instruction.fields[field.id] = field;
                }
                else
                {
                    break;
                }
            }
            else if (field.len == 0)
            {
                instruction.fields[field.id] = field;
            }
            else
            {
                instruction.fields[field.id] = field;
                instruction.fields[field.id].value = (((*tmp_memory) >> (8 - field.len - field_offset)) & mask); 
            }

            if (field.id == FIELD_ID_END)
            {
                instruction.mnemonic_id = instruction_table[i].mnemonic_id;
                strlcpy(instruction.mnemonic_str, instruction_table[i].mnemonic_str, 8);
                memcpy(&instruction.operands, &instruction_table[i].operands, sizeof(instruction.operands));
                InstructionPointer = tmp_memory - (memory + code_segment_base_address);
                break;
            }

            field_offset += field.len;
            if (field_offset == 8)
            {
                tmp_memory += 1;
                field_offset = 0; 
            }
            ++j;
        } /* while (instruction_table[i].fields[j].id != FIELD_ID_END) */

        if (instruction.mnemonic_id != MNEMONIC_ID_NONE)
        {
            break;
        }
    } /* for (size_t i = 0; i < ArrayCount(instruction_table); ++i) */

    /*
    --------------------------------
    Fetching finished
    From here it's the decoding part
    --------------------------------
    */

    for (size_t i = 0; i < 2; ++i)
    {
        struct Operand *operand = &instruction.operands[i];

        u8 field_w_value = instruction.fields[FIELD_ID_W].value;
        u8 *next_instruction_byte = memory + code_segment_base_address + InstructionPointer;

        switch (operand -> id)
        {
            case OPERAND_ID_NONE:
            {
                break;
            }
            case OPERAND_ID_REG:
            {
                if (instruction.fields[FIELD_ID_D].id == FIELD_ID_D)
                {
                    u8 field_d_value = instruction.fields[FIELD_ID_D].value;
                    if (field_d_value == 0)
                    {
                        operand -> dir = SOURCE;
                        instruction.operands[i^1].dir = DESTINATION;
                    }
                    else
                    {
                        operand -> dir = DESTINATION;
                        instruction.operands[i^1].dir = SOURCE;
                    }
                }

                u8 field_reg_value = instruction.fields[FIELD_ID_REG].value;
                strlcpy(operand -> str, reg_field_str[field_reg_value][field_w_value], 32);
                operand -> location = general_registers_start + reg_field_encoding[field_reg_value][field_w_value];
                break;
            }
            case OPERAND_ID_RM:
            {
                u8 field_rm_value = instruction.fields[FIELD_ID_RM].value;
                u8 field_mod_value = instruction.fields[FIELD_ID_MOD].value;
                u8 *offset_base_register = (u8 *)((u16 *)general_registers_start + rm_field_encoding[field_rm_value][0]);
                u8 *offset_index_register = (u8 *)((u16 *)general_registers_start + rm_field_encoding[field_rm_value][1]);
                switch (field_mod_value)
                {
                    case 0b00:
                    {
                        if (field_rm_value == 0b110)
                        {
                            u16 direct_address = *((u16 *)next_instruction_byte);
                            snprintf(operand -> str, 32, "[%hu]", direct_address);
                            operand -> location = memory + data_segment_base_address + direct_address;
                            InstructionPointer += 2;
                        }
                        else
                        {
                            snprintf(operand -> str, 32, "[%s]", rm_field_str[field_rm_value]);
                            operand -> location = memory + data_segment_base_address + *(offset_base_register) + *(offset_index_register);
                        }
                        break;
                    }
                    case 0b01:
                    {
                        s8 displacement = *next_instruction_byte;
                        snprintf(operand -> str, 32, "[%s%+hhi]", rm_field_str[field_rm_value], displacement);
                        operand -> location = memory + data_segment_base_address + *offset_base_register + *offset_index_register + displacement; 
                        InstructionPointer += 1;
                        break;
                    }
                    case 0b10:
                    {
                        s16 displacement = *((u16 *)next_instruction_byte);
                        snprintf(operand -> str, 32, "[%s%+hi]", rm_field_str[field_rm_value], displacement);
                        operand -> location = memory + data_segment_base_address + *offset_base_register + *offset_index_register + displacement;
                        InstructionPointer += 2;
                        break;
                    }
                    case 0b11:
                    {
                        strlcpy(operand -> str, reg_field_str[field_rm_value][field_w_value], 32);
                        operand -> location = general_registers_start + reg_field_encoding[field_rm_value][field_w_value];
                        break;
                    }
                } // switch (instruction.fields[MOD_FIELD_ID].value)
                break;
            }
            case OPERAND_ID_IMMEDIATE:
            {
                if (instruction.fields[FIELD_ID_S].id == FIELD_ID_S && instruction.fields[FIELD_ID_S].value == 1 && field_w_value == 1)
                {
                    // sign extension
                    snprintf(operand -> str, 32, "%+hi", (s16)(*next_instruction_byte));
                    InstructionPointer += 1;
                }
                else
                {
                    (field_w_value == 0) ?
                        snprintf(operand -> str, 32, "%+hhi", (s8)(*next_instruction_byte)):
                        snprintf(operand -> str, 32, "%+hi", *((s16 *)next_instruction_byte));

                    InstructionPointer += field_w_value + 1;
                }
                operand -> location = next_instruction_byte;
                break;
            }
            case OPERAND_ID_ACCUMULATOR:
            {
                strlcpy(operand -> str, reg_field_str[0][field_w_value], 32);
                operand -> location = (u8 *)((u16 *)general_registers_start + REGISTER_AX);
                break;
            }
            case OPERAND_ID_ADDRESS:
            {
                (field_w_value == 0) ?
                    snprintf(operand -> str, 32, "[%hhu]", *next_instruction_byte):
                    snprintf(operand -> str, 32, "[%hu]", *((u16 *)(next_instruction_byte)));

                InstructionPointer += field_w_value + 1;
                operand -> location = next_instruction_byte;
                break;
            }
            case OPERAND_ID_SR:
            {
                u8 field_sr_value = instruction.fields[FIELD_ID_SR].value;
                snprintf(operand -> str, 32, "%s", sr_field_str[field_sr_value]);
                operand -> location = (u8 *)((u16 *)segment_registers_start + field_sr_value);
                break;
            }
            case OPERAND_ID_IPINC8:
            {
                snprintf(operand -> str, 32, "$%+hhi+2", (s8)(*next_instruction_byte));
                operand -> location = next_instruction_byte;
                InstructionPointer += 1;
                break;
            }
            case OPERAND_ID_DATA8:
            {
                snprintf(operand -> str, 32, "%hhi", (s8)(*next_instruction_byte));
                operand -> location = next_instruction_byte;
                InstructionPointer += 1;
                break;
            }
            case OPERAND_ID_DX:
            {
                snprintf(operand -> str, 32, "%s", "dx");
                operand -> location = (u8 *)((u16 *)general_registers_start + REGISTER_DX);
                break;
            }
        }
    }

    return instruction;
}
