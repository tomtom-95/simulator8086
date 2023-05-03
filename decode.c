#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "decode.h"
#include "instruction_table.h"

struct Instruction instruction_fetch(u8 *memory, struct PrefixContest *prefix_contest)
{
    u32 code_segment_base_address = ((u32)(segment_registers_start[REGISTER_CS])) << 4;

    for (;;)
    {
        u8 is_prefix = 0;
        for (size_t i = 0; i < ArrayCount(prefix_table); ++i)
        {
            if (*(memory + code_segment_base_address + InstructionPointer) == prefix_table[i].value)
            {
                switch (prefix_table[i].id)
                {
                    case PREFIX_ID_REP:
                    case PREFIX_ID_REPNE:
                    {
                        prefix_contest -> prefix_id_rep = prefix_table[i].id;
                        break;
                    }
                    case PREFIX_ID_LOCK:
                    {
                        prefix_contest -> prefix_id_lock = prefix_table[i].id;
                        break;
                    }
                    case PREFIX_ID_SEGMENT_ES:
                    case PREFIX_ID_SEGMENT_CS:
                    case PREFIX_ID_SEGMENT_DS:
                    case PREFIX_ID_SEGMENT_SS:
                    {
                        prefix_contest -> prefix_id_segment_override = prefix_table[i].id;
                        break;
                    }
                }

                InstructionPointer++;
                is_prefix = 1;
                break;
            }
        }

        if (is_prefix == 0)
        {
            break;
        }
    }

    struct Instruction none_instruction = {MNEMONIC_ID_NONE, "", {FIELD_ID_NONE}, {OPERAND_ID_NONE}, {OPERAND_ID_NONE}};
    u8 fetching_completed = 0;

    struct Instruction instruction;
    for (size_t i = 0; i < ArrayCount(instruction_table); ++i)
    {
        instruction = none_instruction;

        instruction.mnemonic_id = instruction_table[i].mnemonic_id;
        strlcpy(instruction.mnemonic_str, instruction_table[i].mnemonic_str, 16);
        instruction.destination_operand = instruction_table[i].destination_operand;
        instruction.source_operand = instruction_table[i].source_operand;

        u16 tmp_instruction_pointer = InstructionPointer;
        u8 offset = 0;
        for (size_t j = 0; j < ArrayCount(instruction_table[i].fields); ++j)
        {
            struct Field field = instruction_table[i].fields[j];
            u8 mask = ~((~0) << field.len);
            u8 instruction_byte = *(memory + code_segment_base_address + tmp_instruction_pointer);
            if (field.id == FIELD_ID_OPCODE1 || field.id == FIELD_ID_OPCODE2)
            {
                if (((instruction_byte >> (8 - field.len - offset)) & mask) == field.value)
                {
                    instruction.fields[field.id] = field;
                }
                else
                {
                    break;
                }
            }
            else if (field.id == FIELD_ID_IMPLW)
            {
                instruction.fields[field.id] = field;
            }
            else if (field.id == FIELD_ID_END)
            {
                instruction.fields[field.id] = field;
                fetching_completed = 1;
                break;
            }
            else
            {
                instruction.fields[field.id].id = field.id;
                instruction.fields[field.id].len = field.len;
                instruction.fields[field.id].value = (((instruction_byte) >> (8 - field.len - offset)) & mask);
            }

            // ugly special case for setting destination and source operand
            if (instruction.fields[field.id].id == FIELD_ID_D)
            {
                if (instruction.fields[field.id].value == 0)
                {
                    instruction.destination_operand.id = OPERAND_ID_RM;
                    instruction.source_operand.id = OPERAND_ID_REG;
                }
                else
                {
                    instruction.destination_operand.id = OPERAND_ID_REG;
                    instruction.source_operand.id = OPERAND_ID_RM;
                }
            }

            offset += field.len;
            if (offset == 8)
            {
                offset = 0;
                tmp_instruction_pointer++;
            }
        }

        if (fetching_completed == 1)
        {
            InstructionPointer = tmp_instruction_pointer;
            break;
        }
        else
        {
            continue;
        }
    }

    if (fetching_completed == 1)
    {
        return instruction;
    }
    else
    {
        printf("error in fetching\n");
        exit(EXIT_FAILURE);
    }
}

void instruction_decode(u8 *memory, struct Instruction *instruction)
{
    u32 code_segment_base_address = ((u32)(segment_registers_start[REGISTER_CS])) << 4;
    u32 data_segment_base_address = ((u32)(segment_registers_start[REGISTER_DS])) << 4;

    struct Operand *operands[2] = {&(instruction -> destination_operand), &(instruction -> source_operand)};
    for (size_t i = 0; i < 2; ++i)
    {
        struct Operand *operand = operands[i];

        u8 field_w_value;
        (instruction -> fields[FIELD_ID_W].id == FIELD_ID_W) ?
            (field_w_value = instruction -> fields[FIELD_ID_W].value):
            (field_w_value = instruction -> fields[FIELD_ID_IMPLW].value);

        u8 *next_instruction_byte = memory + code_segment_base_address + InstructionPointer;

        switch (operand -> id)
        {
            case OPERAND_ID_NONE:
            {
                break;
            }
            case OPERAND_ID_REG:
            {
                u8 field_reg_value = instruction -> fields[FIELD_ID_REG].value;
                strlcpy(operand -> decoding, reg_field_str[field_reg_value][field_w_value], 32);
                if (field_w_value == 0)
                {
                    (field_reg_value < 0b100) ?
                        (operand -> location = general_registers_start + 2 * reg_field_encoding[field_reg_value][field_w_value]):
                        (operand -> location = general_registers_start + 2 * reg_field_encoding[field_reg_value][field_w_value] + 1);
                }
                else
                {
                    operand -> location = general_registers_start + 2 * reg_field_encoding[field_reg_value][field_w_value];
                }
                break;
            }
            case OPERAND_ID_RM:
            {
                u8 field_rm_value = instruction -> fields[FIELD_ID_RM].value;
                u8 field_mod_value = instruction -> fields[FIELD_ID_MOD].value;
                u8 *offset_base_register = (u8 *)((u16 *)general_registers_start + rm_field_encoding[field_rm_value][0]);
                u8 *offset_index_register = (u8 *)((u16 *)general_registers_start + rm_field_encoding[field_rm_value][1]);
                switch (field_mod_value)
                {
                    case 0b00:
                    {
                        if (field_rm_value == 0b110)
                        {
                            u16 direct_address = *((u16 *)next_instruction_byte);
                            snprintf(operand -> decoding, 32, "[%hu]", direct_address);
                            operand -> location = memory + data_segment_base_address + direct_address;
                            InstructionPointer += 2;
                        }
                        else
                        {
                            snprintf(operand -> decoding, 32, "[%s]", rm_field_str[field_rm_value]);
                            operand -> location = memory + data_segment_base_address + *(offset_base_register) + *(offset_index_register);
                        }
                        break;
                    }
                    case 0b01:
                    {
                        s8 displacement = *next_instruction_byte;
                        snprintf(operand -> decoding, 32, "[%s%+hhi]", rm_field_str[field_rm_value], displacement);
                        operand -> location = memory + data_segment_base_address + *offset_base_register + *offset_index_register + displacement; 
                        InstructionPointer += 1;
                        break;
                    }
                    case 0b10:
                    {
                        s16 displacement = *((u16 *)next_instruction_byte);
                        snprintf(operand -> decoding, 32, "[%s%+hi]", rm_field_str[field_rm_value], displacement);
                        operand -> location = memory + data_segment_base_address + *offset_base_register + *offset_index_register + displacement;
                        InstructionPointer += 2;
                        break;
                    }
                    case 0b11:
                    {
                        strlcpy(operand -> decoding, reg_field_str[field_rm_value][field_w_value], 32);
                        if (field_w_value == 0)
                        {
                            (field_rm_value < 0b100) ?
                                (operand -> location = general_registers_start + 2 * reg_field_encoding[field_rm_value][field_w_value]):
                                (operand -> location = general_registers_start + 2 * reg_field_encoding[field_rm_value][field_w_value] + 1);
                        }
                        else
                        {
                            operand -> location = general_registers_start + 2 * reg_field_encoding[field_rm_value][field_w_value];
                        }
                        break;
                    }
                } // switch (instruction.fields[MOD_FIELD_ID].value)
                break;
            }
            case OPERAND_ID_IMMEDIATE:
            {
                if (instruction -> fields[FIELD_ID_S].id == FIELD_ID_S && instruction -> fields[FIELD_ID_S].value == 1 && field_w_value == 1)
                {
                    // sign extension
                    snprintf(operand -> decoding, 32, "%+hi", (s16)(*next_instruction_byte));
                    InstructionPointer += 1;
                }
                else
                {
                    (field_w_value == 0) ?
                        snprintf(operand -> decoding, 32, "%+hhi", (s8)(*next_instruction_byte)):
                        snprintf(operand -> decoding, 32, "%+hi", *((s16 *)next_instruction_byte));

                    InstructionPointer += field_w_value + 1;
                }
                operand -> location = next_instruction_byte;
                break;
            }
            case OPERAND_ID_ACCUMULATOR:
            {
                strlcpy(operand -> decoding, reg_field_str[0][field_w_value], 32);
                operand -> location = (u8 *)((u16 *)general_registers_start + REGISTER_AX);
                break;
            }
            case OPERAND_ID_ADDRESS:
            {
                (field_w_value == 0) ?
                    snprintf(operand -> decoding, 32, "[%hhu]", *next_instruction_byte):
                    snprintf(operand -> decoding, 32, "[%hu]", *((u16 *)(next_instruction_byte)));

                InstructionPointer += field_w_value + 1;
                operand -> location = next_instruction_byte;
                break;
            }
            case OPERAND_ID_SR:
            {
                u8 field_sr_value = instruction -> fields[FIELD_ID_SR].value;
                snprintf(operand -> decoding, 32, "%s", sr_field_str[field_sr_value]);
                operand -> location = (u8 *)((u16 *)segment_registers_start + field_sr_value);
                break;
            }
            case OPERAND_ID_IPINC8:
            {
                snprintf(operand -> decoding, 32, "$%+hhi+2", (s8)(*next_instruction_byte));
                operand -> location = next_instruction_byte;
                InstructionPointer += 1;
                break;
            }
            case OPERAND_ID_DATA8:
            {
                snprintf(operand -> decoding, 32, "%hhu", *next_instruction_byte);
                operand -> location = next_instruction_byte;
                InstructionPointer += 1;
                break;
            }
            case OPERAND_ID_DX:
            {
                snprintf(operand -> decoding, 32, "%s", "dx");
                operand -> location = (u8 *)((u16 *)general_registers_start + REGISTER_DX);
                break;
            }
        }
    }

    return;
}
