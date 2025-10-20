#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>
#include <stdlib.h>    
#include <stdio.h>
#include <string.h>
#include "fe_de_ex.h"
#include "data.h"    



// MAIN PROGRAM FUNCTIONS

// Write to display7seg file
void write_to_display7seg_file(FILE* display7seg_file, const IORegisters* io) {
    static int32_t last_display7seg = 0;

    // Print to file if the register has changed
    if (last_display7seg != io->IORegistersArray[DISPLAY7SEG])
    {
        fprintf(display7seg_file, "%08X %08X\n", io->IORegistersArray[CLKS] - 1, io->IORegistersArray[DISPLAY7SEG]);
        // Update the last_display7seg to the new one
        last_display7seg = io->IORegistersArray[DISPLAY7SEG];
    }
}

// Write the current line to simulator trace file
void write_to_trace_file(FILE* file, int32_t cycle, int16_t pc, const int8_t* instruction_line, const Registers* registers) {
    fprintf(file, "%08X ", cycle);
    fprintf(file, "%03X ", pc);
    // Go over line and print instructions
    for (int i = 0; i < 4; i++) {
        fprintf(file, "%02X", (uint8_t)instruction_line[i]);
    }
    fprintf(file, " ");
    // Go over Registers and print them
    for (int i = 0; i <= 15; i++) {
        fprintf(file, "%08X", registers->regs[i]);
        if (i < 15) {
            fprintf(file, " ");
        }
    }
    fprintf(file, "\n");
}

// Write to leds file
void write_to_leds_file(FILE* leds_file, const IORegisters* io_registers) {
    static uint32_t last_leds = 0;

    // Print to file if the register has changed
    if (last_leds != io_registers->IORegistersArray[LEDS])
    {
        fprintf(leds_file, "%08X %08X\n", io_registers->IORegistersArray[CLKS] - 1, io_registers->IORegistersArray[LEDS]);
        // Update the last_leds to the new one
        last_leds = io_registers->IORegistersArray[LEDS];
    }
}

// fetch-decode-execute
void fetch_decode_execute(Registers* registers, Memory* memory, IORegisters* io_registers, IRQ2Data* irq2, Monitor* monitor, Disk* disk, const char* diskout_filename, const char* trace_filename, const char* hwregtrace_filename, const char* leds_filename, const char* display7seg_filename) {
    int16_t pc = 0;
    int in_interrupt = 0;   // 0 = not in interrupt, 1 = in interrupt
    Instruction decoded;

    // Open files
    FILE* display7seg_file = fopen(display7seg_filename, "w");
    if (!display7seg_file) {
        return;
    }
    FILE* trace_file = fopen(trace_filename, "w");
    if (!trace_file) {
        fclose(display7seg_file);
        return;
    }

    FILE* hwregtrace_file = fopen(hwregtrace_filename, "w");
    if (!hwregtrace_file) {
        fclose(display7seg_file);
        fclose(trace_file);
        return;
    }

    FILE* leds_file = fopen(leds_filename, "w");
    if (!leds_file) {
        fclose(display7seg_file);
        fclose(trace_file);
        fclose(hwregtrace_file);
        return;
    }


    while (io_registers->halt) {
        int16_t current_pc = pc;

        // Fetch the instruction
        const int8_t* instr_bytes = read_instruction_from_memory(memory, current_pc);

        // Make a copy to avoid pointer issues
        int8_t instruction_line[4];
        memcpy(instruction_line, instr_bytes, 4);

        // Decode the instruction
        instruction_decode(instruction_line, &decoded, current_pc, memory, registers);

        // Snapshot the register state
        Registers snapshot_registers = *registers;

        // Handle instruction (bigimm needs 2 cycles)
        if (decoded.is_bigimm) {
            check_irq2(io_registers, irq2, io_registers->IORegistersArray[CLKS]);
            increase_clock(io_registers); // 1st cycle
            instruction_execute(&decoded, registers, &pc, memory, &in_interrupt, hwregtrace_file, io_registers);
            check_irq2(io_registers, irq2, io_registers->IORegistersArray[CLKS]);
            increase_clock(io_registers); // 2nd cycle
            // Write to trace file the first word of bigimm
            write_to_trace_file(trace_file, io_registers->IORegistersArray[CLKS] - 1, current_pc, instruction_line, &snapshot_registers);
        }
        else {
            // no Bigimm
            instruction_execute(&decoded, registers, &pc, memory, &in_interrupt, hwregtrace_file, io_registers);
            check_irq2(io_registers, irq2, io_registers->IORegistersArray[CLKS]);
            increase_clock(io_registers);
            write_to_trace_file(trace_file, io_registers->IORegistersArray[CLKS] - 1, current_pc, instruction_line, &snapshot_registers);
        }

        // Update timer
        update_timer(io_registers);
        // Process
        Process_disk_command(memory, io_registers, disk);
        //check for all interrupts
        handle_all_interrupts(io_registers, &pc, &in_interrupt);

        // if needed write to monitor
        if (io_registers->IORegistersArray[MONITORCMD] == 1) {
            write_pixel(monitor, io_registers);
        }
        // Write to leds
        write_to_leds_file(leds_file, io_registers);
        // Write to display7seg
        write_to_display7seg_file(display7seg_file, io_registers);
    }

    // Add the timer to the clock cycles
    io_registers->IORegistersArray[CLKS] += disk->timer;

    // Close files
    fclose(display7seg_file);
    fclose(trace_file);
    fclose(hwregtrace_file);
    fclose(leds_file);
}

// Writes the registers values
void write_registers_to_file(const char* filename, const Registers* registers) {
    FILE* file = fopen(filename, "w");
    // if file is valid
    if (file)
    {
        // Go over the registers from 2 to 15 and print the values to file
        for (int i = 2; i <= 15; i++) {
            fprintf(file, "%08X\n", registers->regs[i]);
        }
        fclose(file);
    }
}

int main(int argc, char* argv[]) {
    // check if the number of input files is valid
    if (argc != 14) {
        return 0;
    }

    // Get all input and output files
    const char* memin = argv[1];      // Instruction memory input file
    const char* diskin = argv[2];      // Disk content input file
    const char* irq2in = argv[3];      // IRQ2 events input file
    const char* memout = argv[4];     // Data memory output file
    const char* regout = argv[5];      // Registers output file
    const char* trace = argv[6];       // Instruction trace output file
    const char* hwregtrace = argv[7];  // Hardware register trace output file
    const char* cycles = argv[8];      // Clock cycle count output file
    const char* leds = argv[9];       // LED state output file
    const char* display7seg = argv[10];// 7-segment display output file
    const char* diskout = argv[11];    // Disk content output file
    const char* monitor_txt = argv[12];// Monitor text output file
    const char* monitor_yuv = argv[13];// Monitor YUV binary output file

    // call init of registers
    Registers registers;
    registers_init(&registers);

    // call init of data memory
    Memory memory;
    memory_init(&memory);
    load_instruction(memin, &memory);

    // call init of io registers
    IORegisters io_registers;
    io_init(&io_registers);

    // call init of diskout
    Disk disk;
    disk_init(diskin, diskout, &disk);

    // call init of monitor
    Monitor monitor;
    init_monitor(&monitor);

    // call init of IRQ2 events
    IRQ2Data irq2;
    load_irq2(irq2in, &irq2);

    // Call the fetch_decode_execute loop
    fetch_decode_execute(&registers, &memory, &io_registers, &irq2, &monitor, &disk, diskout, trace, hwregtrace, leds, display7seg);

    // Write all output files
    write_memory_out(memout, &memory);
    write_registers_to_file(regout, &registers);
    write_monitor_text(&monitor, monitor_txt);
    write_yuv(&monitor, monitor_yuv);
    write_total_cycles(cycles, &io_registers);
    return 0;
}