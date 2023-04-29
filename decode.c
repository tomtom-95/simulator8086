#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "decode.h"
#include "instruction_table.h"
#include "simulate.h"


struct Instruction instruction_decode(u8 **memory, size_t *count)
{
    struct Instruction
    instruction = {MNEMONIC_ID_NONE, "", {FIELD_ID_NONE, 0, 0}, {{OPERAND_ID_NONE}, {OPERAND_ID_NONE}}};
    for (size_t i = 0; i < ArrayCount(instruction_table); ++i)
    {
        u8 *tmp_memory = *memory;
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
            else
            {
                instruction.fields[field.id] = field;
                instruction.fields[field.id].value = (((*tmp_memory) >> (8 - field.len - field_offset)) & mask); 
            }

            if (field.id == FIELD_ID_END)
            {
                instruction.mnemonic_id = instruction_table[i].mnemonic_id;
                strlcpy(instruction.mnemonic_str, instruction_table[i].mnemonic_str, 8);
                memcpy(instruction.operands, instruction_table[i].operands, sizeof(instruction.operands));
                *memory = tmp_memory;
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

        if (instruction.fields[FIELD_ID_W].id == FIELD_ID_W)
        {
            (instruction.fields[FIELD_ID_W].value == 0) ? (operand -> wideness = BYTE_WIDE) : (operand -> wideness = WORD_WIDE); 
        }

        u8 wideness = operand -> wideness;

        switch (operand -> id)
        {
            case OPERAND_ID_NONE:
            {
                break;
            }
            case OPERAND_ID_REG:
            {
                u8 reg_field_value = instruction.fields[FIELD_ID_REG].value;
                u8 reg_name = reg_field_enc[reg_field_value][wideness - 1][0];
                strlcpy(operand -> str, reg_field_str[reg_field_value][wideness - 1], 32);

                if (instruction.fields[FIELD_ID_D].id == FIELD_ID_D)
                {
                    (instruction.fields[FIELD_ID_D].value == 0) ? (operand -> dir = SOURCE) : (operand -> dir = DESTINATION);
                }
                operand -> value = GeneralRegisters[reg_name];

                break;
            }
            case OPERAND_ID_RM:
            {
                if (instruction.fields[FIELD_ID_D].id == FIELD_ID_D)
                {
                    (instruction.fields[FIELD_ID_D].value == 0) ? (operand -> dir = DESTINATION) : (operand -> dir = SOURCE);
                }

                u8 rm_field_value = instruction.fields[FIELD_ID_RM].value;
                u8 base_name = rm_field_enc[rm_field_value][0];
                u8 index_name = rm_field_enc[rm_field_value][1];
                switch (instruction.fields[FIELD_ID_MOD].value)
                {
                    case 0b00:
                    {
                        if (rm_field_value == 0b110)
                        {
                            u16 direct_address = (u16)(((u16)(**memory)) | ((u16)(*(*memory + 1) << 8)));
                            snprintf(operand -> str, 32, "[%hu]", direct_address);
                            operand -> value = direct_address;
                            *memory += 2;
                        }
                        else
                        {
                            snprintf(operand -> str, 32, "[%s]", rm_field_str[rm_field_value]);
                            // TODO: this does not consider the override segment prefix case
                            operand -> value = SegmentRegisters[REGISTER_DS] + GeneralRegisters[base_name] + GeneralRegisters[index_name];
                        }
                        break;
                    }
                    case 0b01:
                    {
                        snprintf(operand -> str, 32, "[%s%+hhi]", rm_field_str[rm_field_value], (s8)(**memory));
                        operand -> value = SegmentRegisters[REGISTER_DS] + GeneralRegisters[base_name] + GeneralRegisters[index_name] + (s16)(**memory);
                        *memory += 1;
                        break;
                    }
                    case 0b10:
                    {
                        s16 displacement = (s16)(((s16)(**memory)) | ((s16)(*(*memory + 1) << 8)));
                        snprintf(operand -> str, 32, "[%s%+hi]", rm_field_str[rm_field_value], displacement);
                        operand -> value = SegmentRegisters[REGISTER_DS] + GeneralRegisters[base_name] + GeneralRegisters[index_name] + displacement;
                        *memory += 2;

                        break;
                    }
                    case 0b11:
                    {
                        u8 reg_name = reg_field_enc[rm_field_value][(operand -> wideness) - 1][0];
                        strlcpy(operand -> str, reg_field_str[rm_field_value][(operand -> wideness) - 1], 32);
                        operand -> value = GeneralRegisters[reg_name];
                        break;
                    }
                } // switch (instruction.fields[MOD_FIELD_ID].value)
                break;
            }
            case OPERAND_ID_IMMEDIATE:
            {
                if (instruction.fields[FIELD_ID_S].id == FIELD_ID_S)
                {
                    if (instruction.fields[FIELD_ID_S].value == 0 && operand -> wideness == BYTE_WIDE)
                    {
                        snprintf(operand -> str, 32, "%+hhi", (s8)(**memory));
                        operand -> value = (s8)(**memory);
                        *memory += 1;
                    }
                    else if (instruction.fields[FIELD_ID_S].value == 0 && operand -> wideness == WORD_WIDE)
                    {
                        s16 immediate = (s16)((**memory) | ((*(*memory + 1)) << 8));
                        snprintf(operand -> str, 32, "%+hi", immediate);
                        operand -> value = immediate;
                        *memory += 2;
                    }
                    else if (instruction.fields[FIELD_ID_S].value == 1 && operand -> wideness == BYTE_WIDE)
                    {
                        printf("something is wrong (also saviomerda)\n");
                    }
                    else if (instruction.fields[FIELD_ID_S].value == 1 && operand -> wideness == WORD_WIDE)
                    {
                        // sign extension
                        snprintf(operand -> str, 32, "%+hi", (s16)(**memory));
                        operand -> value = (s16)(**memory);
                        *memory += 1;
                    }
                } // if (instruction.fields[SR_FIELD_ID].id == SR_FIELD_ID)
                else
                {
                    if (operand -> wideness == BYTE_WIDE)
                    {
                        snprintf(operand -> str, 32, "%+hhi", (s8)(**memory));
                        operand -> value = (s8)(**memory);
                        *memory += 1;
                    }
                    else
                    {
                        s16 immediate = (s16)((u16)**memory) | (((u16)(*(*memory + 1))) << 8);
                        snprintf(operand -> str, 32, "%+hi", immediate);
                        operand -> value = immediate;
                        *memory += 2;
                    }
                }
                break;
            }
            case OPERAND_ID_ACCUMULATOR:
            {
                strlcpy(operand -> str, reg_field_str[0][(operand -> wideness) - 1], 32);
                operand -> value = GeneralRegisters[REGISTER_AX];
                break;
            }
            case OPERAND_ID_ADDRESS:
            {
                if (operand -> wideness == BYTE_WIDE)  
                {
                    snprintf(operand -> str, 32, "[%hhu]", **memory);
                    operand -> value = 
                    *memory += 1;
                }
                else
                {
                    u16 immediate = (u16)((u16)(**memory) | ((u16)((u16)(*(*memory + 1)) << 8)));
                    snprintf(operand -> str, 32, "[%hu]", immediate);
                    *memory += 2;
                }
                break;
            }
            case OPERAND_ID_SR:
            {
                snprintf(operand -> str, 32, "%s", sr_field_enc[instruction.fields[FIELD_ID_SR].value]);
                break;
            }
            case OPERAND_ID_IPINC8:
            {
                snprintf(operand -> str, 32, "$%+hhi+2", (s8)(**memory));
                *memory += 1;
                break;
            }
            case OPERAND_ID_DATA8:
            {
                snprintf(operand -> str, 32, "%hhu", (u8)(**memory));
                *memory += 1;
                break;
            }
            case OPERAND_ID_DX:
            {
                snprintf(operand -> str, 32, "%s", "dx");
                break;
            }
        }
    }

    return instruction;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s {8086 machine code file}\n", argv[0]);
        return 1;
    }

    FILE *fd = fopen(argv[1], "r");
    if (fd == NULL)
    {
        printf("Error: cannot open the file\n");
        return 1;
    }

    u8 *memory = malloc(MB(1) * sizeof(*memory));
    if (memory == NULL)
    {
        printf("Error: cannot allocate memory\n");
        return 1;
    }

    SegmentRegisters[REGISTER_CS] = 0x00080;
    SegmentRegisters[REGISTER_DS] = SegmentRegisters[REGISTER_CS] + KB(64);
    SegmentRegisters[REGISTER_SS] = SegmentRegisters[REGISTER_DS] + KB(64);
    SegmentRegisters[REGISTER_ES] = SegmentRegisters[REGISTER_SS] + KB(64);

    StackPointer = KB(64) - 1;
    InstructionPointer = 0;

    size_t file_len = fread(memory[SegmentRegisters[REGISTER_CS]], 1, KB(64), fd);
    if (ferror(fd) != 0)
    {
        printf("Error: cannot read the file\n");
        return 1;
    }

    if (feof(fd) == 0)
    {
        printf("Error: file too big, it does not fit entirely into the code segment\n");
        return 1;
    }
    fclose(fd);

    FILE *fp = fopen("result.asm", "w");
    if (fp == NULL)
    {
        printf("Error: cannot open myfile.asm for writing\n");
        return 1;
    }
    fprintf(fp, "%s", "bits 16\n\n"); 

    u16 count = file_len;
    while (count)
    {
        //struct Prefix prefixes[PREFIX_ID_COUNT] = {PREFIX_ID_NONE, "", 0};
        struct Instruction instruction = instruction_decode(memory, &count);
        if (instruction.mnemonic_id == MNEMONIC_ID_NONE)
        {
            printf("Error: instruction not read\n");
            return 1;
        }

        if (count < 0)
        {
            printf("Error: read beyond the file boundary\n");
            return 1;
        }

        /*
        ----------------
        Printing on file
        ----------------
        */

        struct Operand source_operand = {OPERAND_ID_NONE, OPERAND_DIRECTION_NONE, OPERAND_WIDENESS_NONE, ""};
        struct Operand destination_operand = {OPERAND_ID_NONE, OPERAND_DIRECTION_NONE, OPERAND_WIDENESS_NONE, ""};
        for (size_t i = 0; i < 2; ++i)
        {
            if (instruction.operands[i].dir == SOURCE)
            {
                source_operand = instruction.operands[i];
            }
            if (instruction.operands[i].dir == DESTINATION)
            {
                destination_operand = instruction.operands[i];
            }
        }
        instruction.operands[SOURCE - 1] = source_operand;
        instruction.operands[DESTINATION - 1] = destination_operand;

        for (size_t i = 0; i < 2; ++i)
        {
            if (instruction.operands[i].id == OPERAND_ID_RM || instruction.operands[i].id == OPERAND_ID_ADDRESS)
            {
                struct Field field_mod = instruction.fields[FIELD_ID_MOD];
                if ((field_mod.id == FIELD_ID_NONE) || (field_mod.id == FIELD_ID_MOD && field_mod.value != 0b11))
                {
                    char tmp[32] = "";
                    strlcpy(tmp, instruction.operands[DESTINATION - 1].str, 32);
                    (instruction.operands[i].wideness == BYTE_WIDE) ? (strlcpy(instruction.operands[DESTINATION - 1].str, "byte ", 32)):
                                                                      (strlcpy(instruction.operands[DESTINATION - 1].str, "word ", 32));
                    strlcat(instruction.operands[DESTINATION - 1].str, tmp, 32);
                }
            }
        }
        if (instruction.operands[DESTINATION - 1].id != OPERAND_ID_NONE && instruction.operands[SOURCE - 1].id != OPERAND_ID_NONE)
        {
            strlcat(instruction.operands[DESTINATION - 1].str, ", ", 32);
        }
        fprintf(fp, "%s %s%s\n", instruction.mnemonic_str, instruction.operands[DESTINATION - 1].str, instruction.operands[SOURCE - 1].str);

        /*
        ----------
        Simulation
        ----------
        */

        switch (instruction.mnemonic_id)
        {
            case MOV:
            {
                struct Operand dest_operand = instruction.operands[DESTINATION - 1];
                u8 wideness = dest_operand.wideness;
                if (dest_operand.id == OPERAND_ID_REG)
                {
                    u8 reg_field_value = instruction.fields[FIELD_ID_REG].value;
                    u8 reg_name = reg_field_enc[reg_field_value][wideness - 1][0];
                    u8 reg_offset = reg_field_enc[reg_field_value][wideness - 1][1];

                    (wideness == BYTE_WIDE) ?
                        (GeneralRegisters[reg_name] = (GeneralRegisters[reg_name] & (0xff00 >> (8 * reg_offset))) | instruction.operands[SOURCE - 1].value):
                        (GeneralRegisters[reg_name] = instruction.operands[SOURCE - 1].value);
                }
                else if (dest_operand.id == OPERAND_ID_RM && instruction.fields[FIELD_ID_MOD].value == 0b11)
                {
                    u8 rm_field_value = instruction.fields[FIELD_ID_RM].value;
                    u8 rm_name = reg_field_enc[rm_field_value][wideness - 1][0];
                    u8 rm_offset = reg_field_enc[rm_field_value][wideness - 1][1];

                    (wideness == BYTE_WIDE) ?
                        (GeneralRegisters[rm_name] = (GeneralRegisters[rm_name] & (0xff00 >> (8 * rm_offset))) | instruction.operands[SOURCE - 1].value):
                        (GeneralRegisters[rm_name] = instruction.operands[SOURCE - 1].value);
                }
                else if (dest_operand.id == OPERAND_ID_RM)
                {
                    printf("TODO\n");
                }
                break;
            }
        } // switch (instruction.mnemonic_id)

    } // while (memory != file_end_p)
    fclose(fp);

    return 0;
}
