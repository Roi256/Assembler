#ifndef DATA_H
#define DATA_H

#include <stdint.h>
#include <stdio.h>


// MEMORY DEFINITIONS
#define DATA_MEM_DEPTH 4096

// Struct for memory
typedef struct {
    int32_t data[DATA_MEM_DEPTH];    // Array for memory
} Memory;


// REGISTERS DEFINITIONS


// Number of registers
#define NUM_REGISTERS 16

// Registers
#define REG_ZERO 0   // $zero - Constant zero
#define REG_IMM  1   // $imm - Sign extended imm
#define REG_V0   2   // $v0 - Result value
#define REG_A0   3   // $a0 - Argument register
#define REG_A1   4   // $a1 - Argument register
#define REG_A2   5   // $a2 - Argument register
#define REG_A3   6   // $a3 - Argument register
#define REG_T0   7   // $t0 - Temporary register
#define REG_T1   8   // $t1 - Temporary register
#define REG_T2   9   // $t2 - Temporary register
#define REG_S0   10  // $s0 - Saved register
#define REG_S1   11  // $s1 - Saved register
#define REG_S2   12  // $s2 - Saved register
#define REG_GP   13  // $gp - Global pointer
#define REG_SP   14  // $sp - Stack pointer
#define REG_RA   15  // $ra - Return address

// Struct for registers
typedef struct {
    int32_t regs[NUM_REGISTERS]; // Array for register values 
    int imm;                     // A flag to update the imm
} Registers;


// IO DEFINITIONS


// Number of IO Registers
#define NUM_IO_REGISTERS 23

// IO Registers
#define IRQ0ENABLE      0  // Enable IRQ0
#define IRQ1ENABLE      1  // Enable IRQ1
#define IRQ2ENABLE      2  // Enable IRQ2
#define IRQ0STATUS      3  // Status of IRQ0
#define IRQ1STATUS      4  // Status of IRQ1
#define IRQ2STATUS      5  // Status of IRQ2
#define IRQHANDLER      6  // Address of interrupt handler
#define IRQRETURN       7  // Address to return after interrupt
#define CLKS            8  // Clock cycle counter
#define LEDS            9  // LED state
#define DISPLAY7SEG     10 // 7-segment display
#define TIMERENABLE     11 // Enable/Disable timer
#define TIMERCURRENT    12 // Current timer value
#define TIMERMAX        13 // Timer max value
#define DISKCMD         14 // Disk command (Read/Write)
#define DISKSECTOR      15 // Disk sector
#define DISKBUFFER      16 // Disk buffer
#define DISKSTATUS      17 // Disk status
#define RESERVED1       18 // Reserved for future use
#define RESERVED2       19 // Reserved for future use
#define MONITORADDR     20 // Monitor address
#define MONITORDATA     21 // Monitor data
#define MONITORCMD      22 // Monitor command

// Struct for io registers
typedef struct {
    int32_t IORegistersArray[NUM_IO_REGISTERS]; // Array of io registers
    int halt;                                   // Halt flag for stopping the simulator
} IORegisters;

// Define bit widths for each register
static const int IO_REGISTER_SIZES[NUM_IO_REGISTERS] = {
    1,  1,  1,  1,  1,  1,  12, 12, 32, 32, 32, 1, 32, 32, 2, 7, 12, 1, 32, 32, 16, 8, 1
};


// DISK DEFINITIONS


// Disk constants
#define SECTOR_SIZE 512      // Bytes per sector
#define NUM_OF_SECTORS 128   // Total sectors
#define WORD_SIZE 4          // 4 bytes per word
#define LINES_PER_SECTOR (SECTOR_SIZE / WORD_SIZE)  // 128 lines per sector
#define TOTAL_WORDS (NUM_OF_SECTORS * (SECTOR_SIZE / WORD_SIZE))  // Total words on disk

// Struct for Disk
typedef struct {
    int timer;
    char input_filename[256];
    char output_filename[256];
} Disk;


// INTERRUPTS DEFINITIONS


// IRQ2 object with all the events array
typedef struct {
    int* events_array;      // array for the irq2 events
    int num_of_events;      // Number of loaded events
    int size;               // size of the events array
    int index;              // next event
} IRQ2Data;

// Monitor constants
#define MONITOR_WIDTH  256   // Monitor width in pixels
#define MONITOR_HEIGHT 256   // Monitor height in pixels

// Monitor structure
typedef struct {
    uint8_t screen[MONITOR_HEIGHT][MONITOR_WIDTH]; // 256x256 pixels
} Monitor;

// Initialize all memory lines to 0
void memory_init(Memory* memory);
// loads instruction from memory file
void load_instruction(const char* filename, Memory* memory);
// Read an instruction from memory
const int8_t* read_instruction_from_memory(const Memory* memory, int address);
// Write to Memory out file
void write_memory_out(const char* filename, const Memory* memory);
// Write a word to memory
void write_data_to_memory(Memory* memory, int address, int32_t value);
// Read a word from memory
int32_t read_data_from_memory(const Memory* memory, int address);
// init the registers
void registers_init(Registers* registers);
// Gets the value of a Register
int32_t get_register(const Registers* registers, int reg_index);
// Sets the value of a Register
void set_register(Registers* registers, int reg_index, int32_t value);

// gets an io register index and return the name of the register
char* io_names_for_output(int reg);
// Initialize all io registers to 0 and Halt to 1
void io_init(IORegisters* io_registers);
// Reads a value from an io register and Prints the command to file
int32_t read_from_io(const IORegisters* io_registers, int reg, FILE* hwregtrace_file);
// Writes a value to an io register and Prints the command to file
void write_to_io(IORegisters* io_registers, int reg, int32_t value, FILE* hwregtrace_file);
//Increase the clock counter by 1
void increase_clock(IORegisters* io_registers);
// Prints to file current status to the hwregtrace file
void write_hwregtrace(FILE* hwregtrace_file, int32_t cycle, const char* operation, const char* reg_name, int32_t value);
// Prints to file the number of total cycles
void write_total_cycles(const char* filename, const IORegisters* io_registers);
// Updates the register that holdes the timer
void update_timer(IORegisters* io_registers);

// Initializes the disk structure and copies the input disk file to the output disk file.
void disk_init(const char* input_filename, const char* output_filename, Disk* disk);
// read sector from disk
void read_data_sector(Memory* memory, const IORegisters* io_registers, const Disk* disk);
// write sector from disk
void write_data_sector(const Memory* memory, const IORegisters* io_registers, const Disk* disk);
// Process disk command and update IRQ
void Process_disk_command(Memory* memory, IORegisters* io_registers, Disk* disk);

// Load IRQ2 from the irq2.txt file
void load_irq2(const char* filename, IRQ2Data* irq2);
// Check if we have IRQ2 in the current cycle
void check_irq2(IORegisters* io_registers, IRQ2Data* irq2, int cycle);
//Handle IRQ0, IRQ1 and IRQ2
void handle_all_interrupts(IORegisters* io_registers, int16_t* pc, int* in_interrupt);

// inits monitor
void init_monitor(Monitor* monitor);
// Writes a pixel to the screen
void write_pixel(Monitor* monitor, IORegisters* io_registers);
// Writes the screen to text output file
void write_monitor_text(const Monitor* monitor, const char* filename);
// Writes to yuv file the screen 
void write_yuv(const Monitor* monitor, const char* filename);
#endif
