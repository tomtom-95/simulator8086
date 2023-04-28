#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "decode.h"
#include "instruction_table.h"
#include "simulate.h"


struct Instruction instruction_decode(u8 **memory)
{
    struct Instruction
    instruction = {MNEMONIC_ID_NONE, "", {FIELD_ID_NONE, 0, 0}, {{OPERAND_ID_NONE}, {OPERAND_ID_NONE}}};
    for (size_t i = 0; i < ArrayCount(instruction_table); ++i)
    {
        u8 *tmp_memory = *memory;
        u8 field_offset = 0;

        size_t j = 0;
        while (instruction_table[i].fields[j].id != FIELD_ID_END)
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

            field_offset += field.len;
            if (field_offset == 8)
            {
                tmp_memory += 1;
                field_offset = 0; 
            }
            j++;
        } /* while (instruction_table[i].fields[j].id != FIELD_ID_END) */

        if (instruction_table[i].fields[j].id == FIELD_ID_END)
        {
            instruction.mnemonic_id = instruction_table[i].mnemonic_id;
            strlcpy(instruction.mnemonic_str, instruction_table[i].mnemonic_str, 8);
            instruction.fields[FIELD_ID_END] = instruction_table[i].fields[j];
            instruction.operands[0] = instruction_table[i].operands[0];
            instruction.operands[1] = instruction_table[i].operands[1];
            *memory = tmp_memory;
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
                    (instruction.fields[FIELD_ID_D].value == 0) ? (operand -> dir = SOURCE) : (operand -> dir = DESTINATION);
                }

                u8 reg_field_value = instruction.fields[FIELD_ID_REG].value;
                (operand -> wideness == BYTE_WIDE) ? strlcpy(operand -> str, reg_field_enc[reg_field_value][0], 32):
                                                     strlcpy(operand -> str, reg_field_enc[reg_field_value][1], 32);
                break;
            }
            case OPERAND_ID_RM:
            {
                if (instruction.fields[FIELD_ID_D].id == FIELD_ID_D)
                {
                    (instruction.fields[FIELD_ID_D].value == 0) ? (operand -> dir = DESTINATION) : (operand -> dir = SOURCE);
                }

                switch (instruction.fields[FIELD_ID_MOD].value)
                {
                    case 0b00:
                    {
                        if (instruction.fields[FIELD_ID_RM].value == 0b110)
                        {
                            u16 direct_address = (u16)(((u16)(**memory)) | ((u16)(*(*memory + 1) << 8)));
                            snprintf(operand -> str, 32, "[%hu]", direct_address);
                            *memory += 2;
                        }
                        else
                        {
                            snprintf(operand -> str, 32, "[%s]", rm_field_enc[instruction.fields[FIELD_ID_RM].value]);
                        }
                        break;
                    }
                    case 0b01:
                    {
                        snprintf(operand -> str, 32, "[%s%+hhi]", rm_field_enc[instruction.fields[FIELD_ID_RM].value], (s8)(**memory));
                        *memory += 1;
                        break;
                    }
                    case 0b10:
                    {
                        s16 displacement = (s16)(((s16)(**memory)) | ((s16)(*(*memory + 1) << 8)));
                        snprintf(operand -> str, 32, "[%s%+hi]", rm_field_enc[instruction.fields[FIELD_ID_RM].value], displacement);
                        *memory += 2;
                        break;
                    }
                    case 0b11:
                    {
                        strlcpy(operand -> str, reg_field_enc[instruction.fields[FIELD_ID_RM].value][(operand -> wideness) - 1], 32);
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
                        *memory += 1;
                    }
                    else if (instruction.fields[FIELD_ID_S].value == 0 && operand -> wideness == WORD_WIDE)
                    {
                        s16 immediate = (s16)((**memory) | ((*(*memory + 1)) << 8));
                        snprintf(operand -> str, 32, "%+hi", immediate);
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
                        *memory += 1;
                    }
                } // if (instruction.fields[SR_FIELD_ID].id == SR_FIELD_ID)
                else
                {
                    if (operand -> wideness == BYTE_WIDE)
                    {
                        snprintf(operand -> str, 32, "%+hhi", (s8)(**memory));
                        *memory += 1;
                    }
                    else
                    {
                        s16 immediate = (s16)((u16)**memory) | (((u16)(*(*memory + 1))) << 8);
                        snprintf(operand -> str, 32, "%+hi", immediate);
                        *memory += 2;
                    }
                }
                break;
            }
            case OPERAND_ID_ACCUMULATOR:
            {
                strlcpy(operand -> str, reg_field_enc[0][(operand -> wideness) - 1], 32);
                break;
            }
            case OPERAND_ID_ADDRESS:
            {
                if (operand -> wideness == BYTE_WIDE)  
                {
                    snprintf(operand -> str, 32, "[%hhu]", **memory);
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

    size_t file_len = fread(memory, 1, MB(1), fd);
    if (ferror(fd) != 0)
    {
        printf("Error: cannot read the file\n");
        return 1;
    }
    /* NOTE: in reality I want way less than ((1024 * 1024) - 1) bytes occupied by code! */
    if (feof(fd) == 0)
    {
        printf("Error: file too big, it does not fit entirely into memory\n");
        return 1;
    }
    fclose(fd);

    u8 *file_end_p = memory + file_len;

    FILE *fp = fopen("result.asm", "w");
    if (fp == NULL)
    {
        printf("Error: cannot open myfile.asm for writing\n");
        return 1;
    }
    fprintf(fp, "%s", "bits 16\n\n"); 

    while (memory != file_end_p)
    {
        if (memory > file_end_p)
        {
            printf("Error: read beyond the file boundary\n");
            return 1;
        }

        struct Prefix prefixes[PREFIX_ID_COUNT] = {PREFIX_ID_NONE, "", 0};
        struct Instruction instruction = instruction_decode(&memory);
        if (instruction.mnemonic_id == MNEMONIC_ID_NONE)
        {
            printf("Error: instruction not read\n");
            return 1;
        }
        if (memory > file_end_p)
        {
            printf("Error: memory outside of boundary\n");
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
    } // while (memory != file_end_p)
    fclose(fp);

    return 0;
}