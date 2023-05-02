#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "decode.c"

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

    u8 *memory = malloc((MB(1) + 2 * GENERAL_REGISTER_COUNT + 2 * SEGMENT_REGISTER_COUNT) * sizeof(*memory));
    if (memory == NULL)
    {
        printf("Error: cannot allocate memory\n");
        return 1;
    }
    general_registers_start = memory + MB(1);
    segment_registers_start = (u16 *)general_registers_start + GENERAL_REGISTER_COUNT;

    segment_registers_start[REGISTER_CS] = 0x0008;
    segment_registers_start[REGISTER_DS] = 0x1008; 
    segment_registers_start[REGISTER_SS] = 0x2008;
    segment_registers_start[REGISTER_ES] = 0x3008;

    InstructionPointer = 0;
    StackPointer = KB(64) - 1;

    size_t file_len = fread(memory + (((u32)segment_registers_start[REGISTER_CS]) << 4) + InstructionPointer, 1, KB(64), fd);
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

    while (InstructionPointer != file_len)
    {
        //struct Prefix prefixes[PREFIX_ID_COUNT] = {PREFIX_ID_NONE, "", 0};
        struct Instruction instruction = instruction_decode(memory);
        if (instruction.mnemonic_id == MNEMONIC_ID_NONE)
        {
            printf("Error: instruction not read\n");
            return 1;
        }

        if (InstructionPointer > file_len)
        {
            printf("Error: read beyond the file boundary\n");
            return 1;
        }

        /*
        ----------------
        Printing on file
        ----------------
        */

        u8 field_w_value = instruction.fields[FIELD_ID_W].value;
        
        struct Operand *source_operand;
        struct Operand *destination_operand;
        if (instruction.operands[0].dir == DESTINATION)
        {
            destination_operand = &instruction.operands[0];
            source_operand = &instruction.operands[1];
        }
        else
        {
            destination_operand = &instruction.operands[1];
            source_operand = &instruction.operands[0];
        }

        for (size_t i = 0; i < 2; ++i)
        {
            if ((instruction.operands[i].id == OPERAND_ID_RM || instruction.operands[i].id == OPERAND_ID_ADDRESS))
            {
                struct Field field_mod = instruction.fields[FIELD_ID_MOD];
                if ((field_mod.id == FIELD_ID_NONE) || (field_mod.id == FIELD_ID_MOD && field_mod.value != 0b11))
                {
                    (field_w_value == 0) ? (strlcat(instruction.mnemonic_str, " byte ", 16)):
                                           (strlcat(instruction.mnemonic_str, " word ", 16));
                }
            }
        }
        if (instruction.operands[0].id != OPERAND_ID_NONE && instruction.operands[1].id != OPERAND_ID_NONE)
        {
            strlcat(destination_operand -> str, ", ", 32);
        }
        fprintf(fp, "%s %s%s\n", instruction.mnemonic_str, destination_operand -> str, source_operand -> str);

        /*
        ----------
        Simulation
        ----------
        */

        switch (instruction.mnemonic_id)
        {
            case MOV:
            {
                (field_w_value == 0) ?
                    (*(destination_operand -> location) = *(source_operand -> location)):
                    (*((u16 *)(destination_operand -> location)) = *((u16 *)(source_operand -> location)));
                break;
            }
        }
    } // while (memory != file_end_p)
    fclose(fp);

    return 0;
}