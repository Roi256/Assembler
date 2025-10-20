#define _CRT_SECURE_NO_WARNINGS
#include "fe_de_ex.h"
#include <stdio.h>
#include "data.h"


// FETCH FUNCTIONS


// Fetch instruction
const int8_t* instruction_fetch(const Memory* memory, int16_t* pc) {
    // Check if PC is valid
    if (*pc > PC_MAX) { return NULL; }

    // Fetch the instruction at the current PC
    const int8_t* instruction = read_instruction_from_memory(memory, *pc);

    if (instruction == NULL) { return NULL; } // Check if instruction is valid
    return instruction;
}

// Increase PC by 1
void increase_pc(int16_t* pc) {
    if (*pc < PC_MAX)
    {
        (*pc)++;
    }
}


// DECODE FUNCTIONS


// Decode instruction
void instruction_decode(const int8_t* instruction_line, Instruction* decoded_instruction, int16_t pc, const Memory* memory, Registers* registers) {
    // Decode the opcode (bits 31:24) - byte 0
    decoded_instruction->opcode = instruction_line[0];

    // Decode rd (bits 23:20) - upper 4 bits of byte 1
    decoded_instruction->rd = (instruction_line[1] >> 4) & 0x0F;

    // Decode rs (bits 19:16) - lower 4 bits of byte 1
    decoded_instruction->rs = instruction_line[1] & 0x0F;

    // Decode rt (bits 15:12) - upper 4 bits of byte 2
    decoded_instruction->rt = (instruction_line[2] >> 4) & 0x0F;

    // Decode reserved (bits 11:9) - bits 3:1 of byte 2
    decoded_instruction->reserved = (instruction_line[2] >> 1) & 0x07;

    // Decode bigimm flag (bit 8) - bit 0 of byte 2
    decoded_instruction->is_bigimm = instruction_line[2] & 0x01;

    // Decode imm8 (bits 7:0) - byte 3
    decoded_instruction->imm8 = instruction_line[3];

    // Turn on imm flag (for updating the $imm register during decoding stage)
    registers->imm = 1;

    // Handle immediate value based on bigimm flag
    if (decoded_instruction->is_bigimm) {
        // For bigimm instructions, read the next word from memory
        if (pc + 1 < DATA_MEM_DEPTH) {
            const uint8_t* next_word = read_instruction_from_memory(memory, pc + 1);
            if (next_word) {
                // Reconstruct 32-bit immediate from 4 bytes (big endian)
                decoded_instruction->immediate = (int32_t)(
                    (next_word[0] << 24) |
                    (next_word[1] << 16) |
                    (next_word[2] << 8) |
                    next_word[3]
                    );
            }
            else {
                decoded_instruction->immediate = 0;
            }
        }
        else {
            decoded_instruction->immediate = 0;
        }
        // Set $imm register to the full 32-bit immediate
        set_register(registers, REG_IMM, decoded_instruction->immediate);
    }
    else {
        // if not bigimm:

        // Check if the sign bit is set
        if (decoded_instruction->imm8 & (1 << (8 - 1))) {
            // If sign bit is set, extend it to fill the value
            decoded_instruction->immediate = decoded_instruction->imm8 | (~((1 << 8) - 1));
        }
        // If sign bit is not set, do nothing
        decoded_instruction->immediate = decoded_instruction->imm8;

        // Set $imm register to immediate
        set_register(registers, REG_IMM, decoded_instruction->immediate);
    }
    // Turn off the imm flag
    registers->imm = 0;
}

// EXECUTE FUNCTIONS

void instruction_execute(const Instruction* decoded_instruction, Registers* registers, int16_t* pc, Memory* memory, int* in_interrupt, FILE* hwregtrace_file, IORegisters* io_registers) {

    // Store current PC for trace and potential restoration
    uint16_t current_pc = *pc;

    // Get register values
    int32_t rs_value = (decoded_instruction->rs == REG_IMM) ? decoded_instruction->immediate : get_register(registers, decoded_instruction->rs);
    int32_t rt_value = (decoded_instruction->rt == REG_IMM) ? decoded_instruction->immediate : get_register(registers, decoded_instruction->rt);
    int32_t rd_value = (decoded_instruction->rd == REG_IMM) ? decoded_instruction->immediate : get_register(registers, decoded_instruction->rd);

    int16_t next_pc = *pc;
    if (decoded_instruction->is_bigimm) { next_pc += 2; } // bigimm instructions take 2 words
    else { next_pc += 1; } // normal instructions take 1 word

    // Switch for implementing the opcode
    switch (decoded_instruction->opcode) {

    case OP_ADD:
        if (decoded_instruction->rd != REG_ZERO && decoded_instruction->rd != REG_IMM) {
            set_register(registers, decoded_instruction->rd, rs_value + rt_value);
        }
        *pc = next_pc;
        break;

    case OP_SUB:
        if (decoded_instruction->rd != REG_ZERO && decoded_instruction->rd != REG_IMM) {
            set_register(registers, decoded_instruction->rd, rs_value - rt_value);
        }
        *pc = next_pc;
        break;

    case OP_MUL:
        if (decoded_instruction->rd != REG_ZERO && decoded_instruction->rd != REG_IMM) {
            set_register(registers, decoded_instruction->rd, rs_value * rt_value);
        }
        *pc = next_pc;
        break;

    case OP_AND:
        if (decoded_instruction->rd != REG_ZERO && decoded_instruction->rd != REG_IMM) {
            set_register(registers, decoded_instruction->rd, rs_value & rt_value);
        }
        *pc = next_pc;
        break;

    case OP_OR:
        if (decoded_instruction->rd != REG_ZERO && decoded_instruction->rd != REG_IMM) {
            set_register(registers, decoded_instruction->rd, rs_value | rt_value);
        }
        *pc = next_pc;
        break;

    case OP_XOR:
        if (decoded_instruction->rd != REG_ZERO && decoded_instruction->rd != REG_IMM) {
            set_register(registers, decoded_instruction->rd, rs_value ^ rt_value);
        }
        *pc = next_pc;
        break;

    case OP_SLL:
        if (decoded_instruction->rd != REG_ZERO && decoded_instruction->rd != REG_IMM) {
            set_register(registers, decoded_instruction->rd, rs_value << rt_value);
        }
        *pc = next_pc;
        break;

    case OP_SRA:
        if (decoded_instruction->rd != REG_ZERO && decoded_instruction->rd != REG_IMM) {
            set_register(registers, decoded_instruction->rd, ((int32_t)rs_value) >> rt_value);
        }
        *pc = next_pc;
        break;

    case OP_SRL:
        if (decoded_instruction->rd != REG_ZERO && decoded_instruction->rd != REG_IMM) {
            set_register(registers, decoded_instruction->rd, (uint32_t)rs_value >> rt_value);
        }
        *pc = next_pc;
        break;

    case OP_BEQ:
        *pc = (rs_value == rt_value) ? rd_value : next_pc;
        break;

    case OP_BNE:
        *pc = (rs_value != rt_value) ? rd_value : next_pc;
        break;

    case OP_BLT:
        *pc = ((int32_t)rs_value < (int32_t)rt_value) ? rd_value : next_pc;
        break;

    case OP_BGT:
        *pc = ((int32_t)rs_value > (int32_t)rt_value) ? rd_value : next_pc;
        break;

    case OP_BLE:
        *pc = ((int32_t)rs_value <= (int32_t)rt_value) ? rd_value : next_pc;
        break;

    case OP_BGE:
        *pc = ((int32_t)rs_value >= (int32_t)rt_value) ? rd_value : next_pc;
        break;

    case OP_JAL:
        if (decoded_instruction->rd != REG_ZERO && decoded_instruction->rd != REG_IMM) {
            set_register(registers, decoded_instruction->rd, next_pc);
        }
        *pc = rs_value;
        break;

    case OP_LW: {
        int32_t address = rs_value + rt_value;
        if (decoded_instruction->rd != REG_ZERO && decoded_instruction->rd != REG_IMM && address >= 0 && address < DATA_MEM_DEPTH) {
            set_register(registers, decoded_instruction->rd, read_data_from_memory(memory, address));
        }
        *pc = next_pc;
        break;
    }

    case OP_SW: {
        int32_t address = rs_value + rt_value;
        if (address >= 0 && address < DATA_MEM_DEPTH) {
            write_data_to_memory(memory, address, rd_value);
        }
        *pc = next_pc;
        break;
    }

    case OP_RETI:
        *pc = io_registers->IORegistersArray[IRQRETURN] & 0x0FFF;
        *in_interrupt = 0;
        break;

    case OP_IN:
        if (decoded_instruction->rd != REG_ZERO && decoded_instruction->rd != REG_IMM) {
            int32_t reg_index = rs_value + rt_value;
            if (reg_index >= 0 && reg_index < NUM_IO_REGISTERS) {
                set_register(registers, decoded_instruction->rd, (int32_t)read_from_io(io_registers, reg_index, hwregtrace_file));
            }
        }
        *pc = next_pc;
        break;

    case OP_OUT: {
        int32_t reg_index = rs_value + rt_value;
        if (reg_index >= 0 && reg_index < NUM_IO_REGISTERS) {
            write_to_io(io_registers, reg_index, (int32_t)rd_value, hwregtrace_file);
        }
        *pc = next_pc;
        break;
    }

    case OP_HALT:
        *pc = current_pc;
        io_registers->halt = 0;
        break;
    }
}