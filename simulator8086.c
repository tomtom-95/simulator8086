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

    u8 *memory = malloc((MB(1) + 2 * GENERAL_REGISTER_COUNT + 2 * SEGMENT_REGISTER_COUNT + 2) * sizeof(*memory));
    if (memory == NULL)
    {
        printf("Error: cannot allocate memory\n");
        return 1;
    }
    general_registers_start = memory + MB(1);
    segment_registers_start = (u16 *)general_registers_start + GENERAL_REGISTER_COUNT;
    flags_start = segment_registers_start + SEGMENT_REGISTER_COUNT;

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
        struct PrefixContest prefix_contest = {PREFIX_ID_NONE};
        struct Instruction instruction = instruction_fetch(memory, &prefix_contest);
        instruction_decode(memory, &instruction);

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

        u8 field_w_value;
        (instruction.fields[FIELD_ID_W].id == FIELD_ID_W) ?
            (field_w_value = instruction.fields[FIELD_ID_W].value):
            (field_w_value = instruction.fields[FIELD_ID_IMPLW].value);

        /*
        ----------------
        Printing on file
        ----------------
        */

        if (prefix_contest.prefix_id_rep != PREFIX_ID_NONE)
        {
            fprintf(fp, "%s ", prefix_table[prefix_contest.prefix_id_rep].decoding);
        }
        if (prefix_contest.prefix_id_lock != PREFIX_ID_NONE)
        {
            fprintf(fp, "%s ", prefix_table[prefix_contest.prefix_id_lock].decoding);
        }

        // TODO: the logic on printing byte and word is good for nasm but it's a bit ugly (sometimes byte and word are
        // are printed when not necessary) add some logic for clean a bit the assembly produced
        struct Operand *operands[2] = {&instruction.destination_operand, &instruction.source_operand};
        for (size_t i = 0; i < 2; ++i)
        {
            if ((operands[i] -> id == OPERAND_ID_RM || operands[i] -> id == OPERAND_ID_ADDRESS))
            {
                struct Field field_mod = instruction.fields[FIELD_ID_MOD];
                if ((field_mod.id == FIELD_ID_NONE) || (field_mod.id == FIELD_ID_MOD && field_mod.value != 0b11))
                {
                    (field_w_value == 0) ?
                        (strlcat(instruction.mnemonic_str, " byte", 16)):
                        (strlcat(instruction.mnemonic_str, " word", 16));
                }
                if (prefix_contest.prefix_id_segment_override != PREFIX_ID_NONE)
                {
                    char tmp[32];
                    strlcpy(tmp, prefix_table[prefix_contest.prefix_id_segment_override].decoding, 32);
                    strlcat(tmp, operands[i] -> decoding, 32);
                    strlcpy(operands[i] -> decoding, tmp, 32);
                }
            }
        }
        if (operands[0] -> id != OPERAND_ID_NONE && operands[1] -> id != OPERAND_ID_NONE)
        {
            strlcat(instruction.destination_operand.decoding, ", ", 32);
        }
        fprintf(fp, "%s %s%s\n", instruction.mnemonic_str, operands[0] -> decoding, operands[1] -> decoding);

        /*
        ----------
        Simulation
        ----------
        */

        printf("%s\n", "before instruction:");
        printf("ax: %x\n", *((u16 *)general_registers_start + REGISTER_AX));
        printf("bx: %x\n", *((u16 *)general_registers_start + REGISTER_BX));
        printf("cx: %x\n", *((u16 *)general_registers_start + REGISTER_CX));
        printf("dx: %x\n", *((u16 *)general_registers_start + REGISTER_DX));
        printf("sp: %x\n", *((u16 *)general_registers_start + REGISTER_SP));
        printf("bp: %x\n", *((u16 *)general_registers_start + REGISTER_BP));
        printf("si: %x\n", *((u16 *)general_registers_start + REGISTER_SI));
        printf("di: %x\n", *((u16 *)general_registers_start + REGISTER_DI));

        switch (instruction.mnemonic_id)
        {
            // NOTE: did I really understand how binary operation works?
            case MOV:
            {
                memcpy(operands[0] -> location, operands[1] -> location, field_w_value + 1);
                break;
            }
            case ADD:
            {
                u16 add = (*((u16 *)(operands[0] -> location))) + (*((u16 *)(operands[1] -> location)));
                memcpy(operands[0] -> location, &add, field_w_value + 1);
                // ZF check
                ((*(operands[0] -> location) == 0) && (*((operands[0] -> location) + field_w_value) == 0)) ?
                    (BitSet(*flags_start, FLAG_ZF)):
                    (BitClear(*flags_start, FLAG_ZF));
                // SF check
                (((*((u8 *)(operands[0] -> location) + field_w_value)) & 0x80) != 0) ? 
                    (BitSet(*flags_start, FLAG_SF)):
                    (BitClear(*flags_start, FLAG_SF));
                break;
            }
            case SUB:
            {
                u16 sub = (*((u16 *)(operands[0] -> location))) - (*((u16 *)(operands[1] -> location)));
                memcpy(operands[0] -> location, &sub, field_w_value + 1);
                // ZF check
                ((*(operands[0] -> location) == 0) && (*((operands[0] -> location) + field_w_value) == 0)) ?
                    (BitSet(*flags_start, FLAG_ZF)):
                    (BitClear(*flags_start, FLAG_ZF));
                // SF check
                (((*((u8 *)(operands[0] -> location) + field_w_value)) & 0x80) != 0) ? 
                    (BitSet(*flags_start, FLAG_SF)):
                    (BitClear(*flags_start, FLAG_SF));
                break;
            }
            case CMP:
            {
                // TODO
                u16 sub = (*((u16 *)(operands[0] -> location))) - (*((u16 *)(operands[1] -> location)));
                break;
            }
        }

        printf("%s\n", "after instruction:");
        printf("ax: %x\n", *((u16 *)general_registers_start + REGISTER_AX));
        printf("bx: %x\n", *((u16 *)general_registers_start + REGISTER_BX));
        printf("cx: %x\n", *((u16 *)general_registers_start + REGISTER_CX));
        printf("dx: %x\n", *((u16 *)general_registers_start + REGISTER_DX));
        printf("sp: %x\n", *((u16 *)general_registers_start + REGISTER_SP));
        printf("bp: %x\n", *((u16 *)general_registers_start + REGISTER_BP));
        printf("si: %x\n", *((u16 *)general_registers_start + REGISTER_SI));
        printf("di: %x\n", *((u16 *)general_registers_start + REGISTER_DI));
        printf("%s\n", "flag register:");
        printf("%x\n", *flags_start);

    } // while (memory != file_end_p)
    fclose(fp);

    return 0;
}