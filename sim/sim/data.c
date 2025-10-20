#define _CRT_SECURE_NO_WARNINGS
#include "data.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// MEMORY FUNCTIONS


// Initialize all memory lines to 0
void memory_init(Memory* memory) {
    for (int i = 0; i < DATA_MEM_DEPTH; i++) {
        memory->data[i] = 0;
    }
}

// loads instruction from memory file
void load_instruction(const char* filename, Memory* memory) {
    FILE* file = fopen(filename, "r");
    if (file) {
        char line[9];
        int address = 0;
        // while there are more lines
        while (address < DATA_MEM_DEPTH && fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\r\n")] = '\0';
            // check if line in valid
            if (strlen(line) != 8) continue;
            memory->data[address] = (int32_t)strtoll(line, NULL, 16);
            address += 1;
        }
        fclose(file);
    }
}

// Read an instruction from memory
const int8_t* read_instruction_from_memory(const Memory* memory, int address) {
    static int8_t instr[4];
    // Check if address is out of bounds
    if (address >= DATA_MEM_DEPTH || address < 0) { return NULL; }
    else {
        int32_t word = memory->data[address];
        instr[0] = (word >> 24) & 0xFF;
        instr[1] = (word >> 16) & 0xFF;
        instr[2] = (word >> 8) & 0xFF;
        instr[3] = word & 0xFF;
        return instr;
    }
}

// Write to Memory out file
void write_memory_out(const char* filename, const Memory* memory) {
    // Find the last non-zero entry in the data memory
    int last_non_zero_index = -1;
    // Go over Data until last non zero
    for (int i = 0; i < DATA_MEM_DEPTH; i++) {
        if (memory->data[i] != 0) {
            last_non_zero_index = i;
        }
    }
    FILE* file = fopen(filename, "w");
    // check validity of file
    if (file)
    {
        // write to file until last non zero index
        for (int i = 0; i <= last_non_zero_index; i++) {
            fprintf(file, "%08X\n", memory->data[i]);
        }
        fclose(file);
    }
}

// Write a word to memory
void write_data_to_memory(Memory* memory, int address, int32_t value) {
    if (address >= DATA_MEM_DEPTH || address < 0) { return; }
    else { memory->data[address] = value; }
}

// Read a word from memory
int32_t read_data_from_memory(const Memory* memory, int address) {
    if (address >= DATA_MEM_DEPTH || address < 0) { return 0; }
    else { return memory->data[address]; }
}


// REGISTER FUNCTIONS


// Initialize the registers to 0
void registers_init(Registers* registers) {
    for (int i = 0; i < NUM_REGISTERS; i++)
    {
        registers->regs[i] = 0;
    }
    registers->imm = 0;
}

// Get the value of a register
int32_t get_register(const Registers* registers, int reg_index) {
    if (reg_index >= NUM_REGISTERS || reg_index < 0) { return 0; }
    return registers->regs[reg_index];
}

// Set the value of a register
void set_register(Registers* registers, int reg_index, int32_t value) {
    if (reg_index < 0 || reg_index >= NUM_REGISTERS) { return; }

    // Handle $imm during decode stage
    if (registers->imm && reg_index == REG_IMM) {
        registers->regs[reg_index] = value;
        return;
    }

    // Make sure not to write to $zero or $imm outside of decode
    if (reg_index == REG_ZERO || reg_index == REG_IMM) {
        return;
    }
    registers->regs[reg_index] = value;
}


// IO FUNCTIONS


// map register by names
char* io_names_for_output(int reg) {
    switch (reg) {
    case 0:
        return "irq0enable";
    case 1:
        return "irq1enable";
    case 2:
        return "irq2enable";
    case 3:
        return "irq0status";
    case 4:
        return "irq1status";
    case 5:
        return "irq2status";
    case 6:
        return "irqhandler";
    case 7:
        return "irqreturn";
    case 8:
        return "clks";
    case 9:
        return "leds";
    case 10:
        return "display7seg";
    case 11:
        return "timerenable";
    case 12:
        return "timercurrent";
    case 13:
        return "timermax";
    case 14:
        return "diskcmd";
    case 15:
        return "disksector";
    case 16:
        return "diskbuffer";
    case 17:
        return "diskstatus";
    case 18:
        return "reserved1";
    case 19:
        return "reserved2";
    case 20:
        return "monitoraddr";
    case 21:
        return "monitordata";
    case 22:
        return "monitorcmd";
    default:
        return "unknown";
    }
}

// Initialize all io registers to 0 and Halt to 1
void io_init(IORegisters* io_registers) {
    memset(io_registers->IORegistersArray, 0, sizeof(io_registers->IORegistersArray));
    io_registers->halt = 1;
}

// Read value from an io register
int32_t read_from_io(const IORegisters* io_registers, int reg, FILE* hwregtrace_file) {
    // if index is in bounds
    if (reg < NUM_IO_REGISTERS && reg > 0)
    {
        int32_t register_value = io_registers->IORegistersArray[reg];

        // check if file is valid
        if (hwregtrace_file) {
            // write hwregtrace
            write_hwregtrace(hwregtrace_file, io_registers->IORegistersArray[CLKS], "READ", io_names_for_output(reg), register_value);
        }
        return register_value;
    }
    else { return 0; }
}

// Write value to an io register
void write_to_io(IORegisters* io_registers, int reg, int32_t value, FILE* hwregtrace_file) {
    // if index is in bounds
    if (reg < NUM_IO_REGISTERS && reg > 0)
    {
        int bit_width = IO_REGISTER_SIZES[reg];
        // apply mask to limit the value
        if (bit_width > 0) {
            int32_t mask = (bit_width == 32) ? 0xFFFFFFFF : (1U << bit_width) - 1;
            io_registers->IORegistersArray[reg] = value & mask;
        }

        // check if file is valid
        if (hwregtrace_file) {
            write_hwregtrace(hwregtrace_file, io_registers->IORegistersArray[CLKS], "WRITE", io_names_for_output(reg), value);
        }
    }
}

// Increase the clock by 1
void increase_clock(IORegisters* io_registers) {
    io_registers->IORegistersArray[CLKS] += 1;
}

// Write the hwregtrace file
void write_hwregtrace(FILE* hwregtrace_file, int32_t cycle, const char* operation, const char* reg_name, int32_t value) {
    // print if files are valid
    if (hwregtrace_file && reg_name)
        fprintf(hwregtrace_file, "%08X %s %s %08X\n", cycle, operation, reg_name, value);
}

// Write the total cycle number
void write_total_cycles(const char* filename, const IORegisters* io_registers) {
    FILE* file = fopen(filename, "w");
    // check validity of file
    if (file)
    {
        // write cycles
        fprintf(file, "%08X\n", io_registers->IORegistersArray[CLKS]);
        fclose(file);
    }
}

// Update the timer registers and handle timer-related interrupts
void update_timer(IORegisters* io_registers) {
    // Check if the timer is enabled
    if (io_registers->IORegistersArray[TIMERENABLE] == 1) {
        io_registers->IORegistersArray[TIMERCURRENT] += 1;

        // If TIMERCURRENT == TIMERMAX
        if (io_registers->IORegistersArray[TIMERCURRENT] == io_registers->IORegistersArray[TIMERMAX])
        {
            io_registers->IORegistersArray[IRQ0STATUS] = 1;   // Trigger IRQ0
            io_registers->IORegistersArray[TIMERCURRENT] = 0; // Reset TIMERCURRENT
        }
    }
}


// DISK FUNCTIONS


// Initialize disk
void disk_init(const char* input_filename, const char* output_filename, Disk* disk) {
    disk->timer = 0;  // Set disk timer to initial state

    // Store file names in disk structure
    strncpy(disk->input_filename, input_filename, 255);
    disk->input_filename[255] = '\0';
    strncpy(disk->output_filename, output_filename, 255);
    disk->output_filename[255] = '\0';

    // Initialize input and output file streams
    FILE* input_file = fopen(input_filename, "r");
    FILE* output_file = fopen(output_filename, "w");

    char line[16];  // string for 8 hexa
    int word_count = 0;

    // print to output file
    while (fgets(line, sizeof(line), input_file)) {

        line[strcspn(line, "\r\n")] = '\0';
        // Check if line is valid
        if (strlen(line) != 8 || strspn(line, "0123456789ABCDEFabcdef") != 8) {
            continue;
        }

        fprintf(output_file, "%.8s\n", line);
        word_count++;

        // Halt if disk is full
        if (word_count >= TOTAL_WORDS) {
            break;
        }
    }

    fclose(input_file);
    fclose(output_file);

}

// Read data sector
void read_data_sector(Memory* memory, const IORegisters* io_registers, const Disk* disk) {

    // Read data from disk sector
    FILE* disk_file = fopen(disk->input_filename, "r");
    // check if file is valid
    if (!disk_file) { return; }

    char current_line[16];  // string for 8 hexa
    int i = 0;

    // get sector and buffer
    int32_t sector = io_registers->IORegistersArray[DISKSECTOR];
    int32_t buffer = io_registers->IORegistersArray[DISKBUFFER];

    // find the sector
    if (fseek(disk_file, sector * LINES_PER_SECTOR * 10, SEEK_SET) != 0) {
        fclose(disk_file);
        return;
    }

    while (i < LINES_PER_SECTOR && fgets(current_line, sizeof(current_line), disk_file)) {
        // Strip starting newline character
        current_line[strcspn(current_line, "\r\n")] = '\0';

        // Check if line is valid
        if (strlen(current_line) != 8 || strspn(current_line, "0123456789ABCDEFabcdef") != 8) {
            continue;
        }

        int32_t word = (int32_t)strtoll(current_line, NULL, 16);
        // Place the value in memory at the designated index
        write_data_to_memory(memory, buffer + i, word);
        i += 1;
    }

    fclose(disk_file);
}

// write data sector
void write_data_sector(const Memory* memory, const IORegisters* io_registers, const Disk* disk) {
    int32_t sector = io_registers->IORegistersArray[DISKSECTOR];
    int32_t buffer = io_registers->IORegistersArray[DISKBUFFER];

    // Write data to disk sector
    FILE* disk_file = fopen(disk->output_filename, "r+");
    if (!disk_file) {
        return;
    }

    // find the sector
    if (fseek(disk_file, sector * LINES_PER_SECTOR * 10, SEEK_SET) != 0) {
        fclose(disk_file);
        return;
    }

    // Initialize the sector with zeros to prevent leftover data
    for (int i = 0; i < LINES_PER_SECTOR; i++) {
        fprintf(disk_file, "00000000\n");
    }

    // Return file pointer to the beginning of the sector
    fseek(disk_file, sector * LINES_PER_SECTOR * 10, SEEK_SET);

    for (int i = 0; i < LINES_PER_SECTOR; i++) {
        int32_t word = read_data_from_memory(memory, buffer + i);

        // print output
        if (fprintf(disk_file, "%08X\n", word) < 0) {
            fclose(disk_file);
            return;
        }
    }

    fclose(disk_file);
}

// Process disk operation commands
void Process_disk_command(Memory* memory, IORegisters* io_registers, Disk* disk) {
    if (io_registers->IORegistersArray[DISKSTATUS] == 1) {
        if (disk->timer > 0) {
            disk->timer -= 1;

            if (disk->timer == 0) {
                // Reset diskcmd
                io_registers->IORegistersArray[DISKCMD] = 0;
                // Mark disk as ready
                io_registers->IORegistersArray[DISKSTATUS] = 0;
                // Trigger IRQ1
                io_registers->IORegistersArray[IRQ1STATUS] = 1;
            }
        }
        return;
    }

    if (io_registers->IORegistersArray[DISKCMD] != 0) {
        switch (io_registers->IORegistersArray[DISKCMD]) {
        case 1:
            // Read sector operation
            read_data_sector(memory, io_registers, disk);
            break;

        case 2:
            // Write sector operation
            write_data_sector(memory, io_registers, disk);
            break;

        default:
            break;
        }

        disk->timer = 1024; // start timer
        io_registers->IORegistersArray[DISKSTATUS] = 1; // Mark disk as busy
    }
}


// INTERRUPT FUNCTIONS


// Get all irq2 from irq2.txt
void load_irq2(const char* filename, IRQ2Data* irq2) {
    FILE* file = fopen(filename, "r");
    if (!file) { exit(1); }  // if file not found

    // create new event and initialize the irq2 object
    int new_event;
    irq2->events_array = NULL;
    irq2->num_of_events = 0;
    irq2->size = 0;
    irq2->index = 0;

    // add new event to irq2 events array
    while (fscanf(file, "%d", &new_event) == 1) {
        if (irq2->num_of_events >= irq2->size) {
            irq2->size += 1;
            irq2->events_array = realloc(irq2->events_array, irq2->size * sizeof(int));
            if (!irq2->events_array) {
                fclose(file);
                exit(1);
            }
        }
        irq2->events_array[irq2->num_of_events++] = new_event;
    }
    fclose(file);
}

// Check if we have IRQ2 in this cycle
void check_irq2(IORegisters* io_registers, IRQ2Data* irq2, int cycle) {
    // if we are still in the size of the events array and the cycle of the interrupt is the current cycle
    if (irq2->index < irq2->num_of_events) {
        if (cycle == irq2->events_array[irq2->index]) {
            io_registers->IORegistersArray[IRQ2STATUS] = 1;
            irq2->index++;
        }
    }
}

// Handle interrupts (1,2,3)
void handle_all_interrupts(IORegisters* io_registers, int16_t* pc, int* in_interrupt) {
    int irq0 = io_registers->IORegistersArray[IRQ0ENABLE] & io_registers->IORegistersArray[IRQ0STATUS];
    int irq1 = io_registers->IORegistersArray[IRQ1ENABLE] & io_registers->IORegistersArray[IRQ1STATUS];
    int irq2 = io_registers->IORegistersArray[IRQ2ENABLE] & io_registers->IORegistersArray[IRQ2STATUS];

    int irq = 0;
    if (irq0 || irq1 || irq2)
        irq = 1;
    else
        irq = 0;
    // if irq != 0 and 
    if (irq != 0 && *in_interrupt == 0) {
        io_registers->IORegistersArray[IRQRETURN] = *pc;
        *pc = io_registers->IORegistersArray[IRQHANDLER] & 0x0FFF;
        *in_interrupt = 1;
    }
}

// Initialize the monitor's screen to all zeros
void init_monitor(Monitor* monitor) {
    memset(monitor->screen, 0, sizeof(monitor->screen));
}

// Write a pixel to the screen
void write_pixel(Monitor* monitor, IORegisters* io_registers) {
    // Get the offset
    int16_t offset = io_registers->IORegistersArray[MONITORADDR];

    // Find row and column
    int16_t row = offset / MONITOR_WIDTH;
    int16_t col = offset % MONITOR_WIDTH;

    // Make sure that the row and column are correct
    if (row >= MONITOR_HEIGHT || col >= MONITOR_WIDTH) { return; }

    // Write a pixel to the screen
    monitor->screen[row][col] = (int8_t)(io_registers->IORegistersArray[MONITORDATA]);

    // Notice that the command is complete
    io_registers->IORegistersArray[MONITORCMD] = 0;
}

// Write the monitor's screen to a text file
void write_monitor_text(const Monitor* monitor, const char* filename) {
    // Find the last pixel that is not zero
    int last_non_zero_row = -1;
    int last_non_zero_col = -1;

    // Go over the size of the monitor
    for (int row = 0; row < MONITOR_HEIGHT; row++) {
        for (int col = 0; col < MONITOR_WIDTH; col++) {
            // Update to the latest pixel that is not zero
            if (monitor->screen[row][col] != 0) {
                last_non_zero_row = row;
                last_non_zero_col = col;
            }
        }
    }

    FILE* file = fopen(filename, "w");
    // Make sure that file is valid
    if (!file) { return; }

    // Write the monitor until the last pixel that we found
    for (int row = 0; row <= last_non_zero_row; row++) {
        for (int col = 0; col < MONITOR_WIDTH; col++) {
            fprintf(file, "%02X\n", monitor->screen[row][col]);

            // Check if we need to stop writing
            if (row == last_non_zero_row && col == last_non_zero_col)
            {
                fclose(file);
                return;
            }
        }
    }
    fclose(file);
}

// Write the monitor's screen to a YUV binary file
void write_yuv(const Monitor* monitor, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) { return; }

    // Write the screen to the file row by row
    for (int row = 0; row < MONITOR_HEIGHT; row++) {
        fwrite(monitor->screen[row], 1, MONITOR_WIDTH, file);
    }

    fclose(file);
}
