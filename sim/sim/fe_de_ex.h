#ifndef FE_DE_EX_H
#define FE_DE_EX_H

#include <stdint.h>
#include "data.h"

// Maximum value for the PC
#define PC_MAX 4095

// Opcode definitions - according to SIMP ISA
#define OP_ADD   0   // R[rd] = R[rs] + R[rt]
#define OP_SUB   1   // R[rd] = R[rs] - R[rt]  
#define OP_MUL   2   // R[rd] = R[rs] * R[rt]
#define OP_AND   3   // R[rd] = R[rs] & R[rt]
#define OP_OR    4   // R[rd] = R[rs] | R[rt]
#define OP_XOR   5   // R[rd] = R[rs] ^ R[rt]
#define OP_SLL   6   // R[rd] = R[rs] << R[rt]
#define OP_SRA   7   // R[rd] = R[rs] >> R[rt]
#define OP_SRL   8   // R[rd] = R[rs] >> R[rt]
#define OP_BEQ   9   // if (R[rs] == R[rt]) pc = R[rd]
#define OP_BNE   10  // if (R[rs] != R[rt]) pc = R[rd]
#define OP_BLT   11  // if (R[rs] < R[rt]) pc = R[rd]
#define OP_BGT   12  // if (R[rs] > R[rt]) pc = R[rd]
#define OP_BLE   13  // if (R[rs] <= R[rt]) pc = R[rd]
#define OP_BGE   14  // if (R[rs] >= R[rt]) pc = R[rd]
#define OP_JAL   15  // R[rd] = next instruction address, pc = R[rs]
#define OP_LW    16  // R[rd] = MEM[R[rs] + R[rt]]
#define OP_SW    17  // MEM[R[rs] + R[rt]] = R[rd]
#define OP_RETI  18  // PC = IORegister[7]
#define OP_IN    19  // R[rd] = IORegister[R[rs] + R[rt]]
#define OP_OUT   20  // IORegister[R[rs] + R[rt]] = R[rd]
#define OP_HALT  21  // Halt execution, exit simulator

// Structure to represent a decoded instruction
typedef struct {
    int8_t opcode;       // 8 bits (bits 31:24)
    int8_t rd;           // 4 bits (bits 23:20)
    int8_t rs;           // 4 bits (bits 19:16)
    int8_t rt;           // 4 bits (bits 15:12)
    int8_t reserved;     // 3 bits (bits 11:9) - set to 0
    int8_t is_bigimm;    // 1 bit (bit 8)
    int8_t imm8;         // 8 bits (bits 7:0) - immediate value for non-bigimm
    int32_t immediate;   // 32 bits - full immediate for bigimm instructions
} Instruction;


// PC+= 1
void increase_pc(int16_t* pc);

// Fetch functions
const int8_t* instruction_fetch(const Memory* memory, int16_t* pc);

// Decode functions
void instruction_decode(const int8_t* instruction_line, Instruction* decoded_instruction, int16_t pc, const Memory* memory, Registers* registers);

// Execute functions
void instruction_execute(const Instruction* decoded_instruction, Registers* registers, int16_t* pc, Memory* memory, int* in_interrupt, FILE* hwregtrace_file, IORegisters* io);


#endif