#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>  // Added for isalpha function

#define MAX_LINE 500 // Max chars in line
#define LABLE_MAX 50	// Max Labels chars
#define LABLE_NUM 2000 // Max Lables
#define MAX_LINES 10000 // Max lines
#define MAX_WORD_ENTRIES 100 // Maximum number of .word entries

// Object of type Lable
typedef struct Lable {
	char LableName[LABLE_MAX];
	int position;
}Lable;

// Object of type Instruction
typedef struct { 
	char opcode_str[10];     
	int opcode;              
	int rd, rs, rt;          
	int immediate;           
	int has_label;           
	int is_bigimm;           
	int line_number;         
} Instruction;

typedef struct {
	char OpCodeName[10];
	int code;
} OpcodeEntry;

// Structure to store .word entries
typedef struct {
	int address;
	int data;
} WordEntry;

OpcodeEntry opcode_table[] = {
	{"add", 0}, {"sub", 1}, {"mul", 2},
	{"and", 3}, {"or", 4}, {"xor", 5},
	{"sll", 6}, {"sra", 7}, {"srl", 8},
	{"beq", 9}, {"bne", 10}, {"blt", 11},
	{"bgt", 12}, {"ble", 13}, {"bge", 14},
	{"jal", 15}, {"lw", 16}, {"sw", 17},
	{"reti", 18}, {"in", 19}, {"out", 20},
	{"halt", 21}
};
int opcode_table_size = sizeof(opcode_table) / sizeof(OpcodeEntry);

typedef struct {
	char RegisterEntryName[10];
	int number;
} RegisterEntry;

RegisterEntry register_table[] = {
	{"$zero", 0}, { "$imm", 1 }, { "$v0", 2 },
	{ "$a0", 3 }, { "$a1", 4 }, { "$a2", 5 }, { "$a3", 6 },
	{ "$t0", 7 }, { "$t1", 8 }, { "$t2", 9 },
	{ "$s0", 10 }, { "$s1", 11 }, { "$s2", 12 },
	{ "$gp", 13 }, { "$sp", 14 }, { "$ra", 15 }
};
int register_table_size = sizeof(register_table) / sizeof(RegisterEntry);

// Global array to store .word entries
WordEntry word_entries[MAX_WORD_ENTRIES];
int word_entries_count = 0;

// Function declarations
int Get_labels(FILE* file, Lable lb[]);
int assembler_second_run(FILE* inputFile, Instruction lines[], Lable struct_lb[], FILE* outputFile, int label_num);
int estimate_if_bigimm(const char* line);
int is_label_immediate(const char* token);
int lookup_word(const char* word);
char* remove_Spaces(const char* word);
int lookup_label(const char* word, Lable struct_lb[], int label_num);
int line_to_hexa(char* line, Lable struct_lb[], FILE* outputFile, int label_num);
int parse_number(const char* token); // parse numbers (decimal/hex)
void collect_word_entries(FILE* inputFile); // collect all .word entries
void remove_comments(char* line); // remove comments from line

int main(int argc, char* argv[])
{
	FILE* inputFile;
	FILE* outputFile;
	Instruction lines[MAX_LINES];
	Lable struct_lb[LABLE_NUM];
	
	int label_num = 0;

	// validate number of arguments
	if (argc != 3) {
		printf("Usage: %s <input_file> <output_file>\n", argv[1]);
		return 1;
	}

	// Open input file
	inputFile = fopen(argv[1], "r");
	if (inputFile == NULL) {
		printf("Error: Could not open input file.\n");
		return 1;
	}

	// Collect all .word entries first
	collect_word_entries(inputFile);
	rewind(inputFile); // Reset file pointer to beginning

	// First pass: Get labels
	label_num = Get_labels(inputFile, struct_lb);
	rewind(inputFile); // Reset file pointer to beginning

	// Open output file
	outputFile = fopen(argv[2], "w");
	if (outputFile == NULL) {
		printf("Error: Could not open output file.\n");
		fclose(inputFile);
		return 1;
	}

	// Second pass: Generate machine code
	assembler_second_run(inputFile, lines, struct_lb, outputFile, label_num);

	// Close files
	fclose(inputFile);
	fclose(outputFile);

	return 0;
}

// Removes comments from a given line
void remove_comments(char* line) {
	char* comment = strchr(line, '#');
	if (comment) {
		*comment = '\0'; // Terminate the string at the comment position
	}
}

// Parse numbers
int parse_number(const char* token) {
	if (!token) return 0;

	// Remove any spaces from the token
	char* cleaned_token = remove_Spaces(token);
	int result;

	// Check if the token starts with 0x or 0X
	if (strlen(cleaned_token) > 2 && cleaned_token[0] == '0' &&
		(cleaned_token[1] == 'x' || cleaned_token[1] == 'X')) {
		// Parse as hexadecimal
		result = (int)strtol(cleaned_token, NULL, 16);
	}
	else {
		// Try to parse with base 0
		result = (int)strtol(cleaned_token, NULL, 0);
	}

	free(cleaned_token);
	return result;
}

// Go over the input file and collect all .word entries
void collect_word_entries(FILE* inputFile) {
	char line[MAX_LINE];
	word_entries_count = 0;

	while (fgets(line, sizeof(line), inputFile)) {
		// Remove comments first before processing
		remove_comments(line);

		// Skip empty lines and labels
		if (line[0] == '\0' || line[0] == '\n' || strstr(line, ":") != NULL) {
			continue;
		}

		// Check if this line contains .word
		if (strstr(line, ".word") != NULL) {
			if (word_entries_count >= MAX_WORD_ENTRIES) {
				printf("Warning: Maximum number of .word entries exceeded\n");
				break;
			}

			char address_str[50], data_str[50];
			if (sscanf(line, " .word %s %s", address_str, data_str) == 2) {
				word_entries[word_entries_count].address = parse_number(address_str);
				word_entries[word_entries_count].data = parse_number(data_str);
				word_entries_count++;
			}
		}
	}
}

// Goes over the code and adds the label names to the label struct array
int Get_labels(FILE* file, Lable lb[])
{
	char line[MAX_LINE];
	int labelCounter = 0;
	int counter = 0;
	char foundLabels[LABLE_MAX];
	int FoundCharsCounter = 0;
	int CharCounter = 0;
	int isLabel = 0;
	char* tempLine;
	char t;

	while (fgets(line, sizeof(line), file)) {
		remove_comments(line);
		
		int i = 0;
		// Skip spaces if there are any
		while (line[i] == ' ' || line[i] == '\t') { i++; }

		// If line is empty after removing spaces and comments, skip it
		if (line[i] == '\0' || line[i] == '\n') { continue; }

		// Skip .word lines when counting labels
		if (strstr(line, ".word") != NULL) { continue; }

		// Check for label in line
		if (strstr(line, ":") != NULL) {
			isLabel = 0;
			CharCounter = i;  // Start from the first character that is not space
			FoundCharsCounter = 0;

			// Extract label name
			do {
				t = line[CharCounter];
				if (t != ':' && t != ' ' && t != '\t') {
					foundLabels[FoundCharsCounter] = t;
					FoundCharsCounter++;
				}
				CharCounter++;
			} while (t != ':' && CharCounter < MAX_LINE);

			foundLabels[FoundCharsCounter] = '\0';

			// Check if the line is only a label
			while ((line[CharCounter] == ' ') || (line[CharCounter] == '\t')) { CharCounter++; }

			if ((line[CharCounter] == '\n') || (line[CharCounter] == '\0')) { isLabel = 1; }
				
			tempLine = foundLabels;
			strcpy(lb[labelCounter].LableName, tempLine);

			if (isLabel == 1) {
				lb[labelCounter].position = counter;
				counter--;  // Don't increase counter for label only lines
			}
			else {
				lb[labelCounter].position = counter;
			}
			labelCounter++;
		}

		// Check if there are only spaces in line
		CharCounter = 0;
		while (line[CharCounter] == ' ' || line[CharCounter] == '\t') { CharCounter++; }
		if (line[CharCounter] == '\n' || line[CharCounter] == '\0') { continue; }

		// Count instruction lines
		if (estimate_if_bigimm(line)) {
			counter += 2;
		}
		else {
			counter++;
		}
	}
	return(labelCounter);
}

// Find if given line needs bigimm: label or out-of-range imm
int estimate_if_bigimm(const char* line) {
	char temp_line[MAX_LINE];
	strcpy(temp_line, line);

	// Remove comments from the temporary line
	remove_comments(temp_line);

	char* token = strtok(temp_line, ", ");
	int count = 0;
	while (token) {
		count++;
		if (count == 5) {  // 5th operand = immediate
			if (is_label_immediate(token)) {
				return 1;
			}
			int imm = parse_number(token); // call parse_number function to get the actual number
			if (imm < -128 || imm > 127) {
				return 1;
			}
			return 0;
		}
		token = strtok(NULL, ", ");
	}
	return 0;
}

// Finds if given labele is immediate
int is_label_immediate(const char* token) {
	if (!token) return 0;
	int i = 0;
	// Goes over the word and checks if the word has Hexa or word is not a number
	while (token[i]) {
		if (token[i] && token[i] == '0' && token[i + 1] == 'x') {
			i += 2;
		}
		else if (isalpha(token[i])) {
			return 1;
		}
		else {
			i++;
		}
	}
	return 0;
}

// Second run of the assembler. Finds labels in the immediate and changes them for the address
int assembler_second_run(FILE* inputFile, Instruction lines[], Lable struct_lb[], FILE* outputFile, int label_num) {
	char line[MAX_LINE];
	int linesCounter = 0;

	// Gets the lines from input file
	while (fgets(line, sizeof(line), inputFile)) {
		remove_comments(line);

		// Checks for empty lines => skip line
		if (line[0] == '\0' || line[0] == '\n' || strstr(line, ":") != NULL) {
			continue;
		}
		// Checks if line has .word => skip line
		if (strstr(line, ".word") != NULL) {
			continue; 
		}
		linesCounter += line_to_hexa(line, struct_lb, outputFile, label_num);
	}

	// Sort .word entries by address to handle them in order
	for (int i = 0; i < word_entries_count - 1; i++) {
		for (int j = i + 1; j < word_entries_count; j++) {
			if (word_entries[i].address > word_entries[j].address) {
				WordEntry temp = word_entries[i];
				word_entries[i] = word_entries[j];
				word_entries[j] = temp;
			}
		}
	}

	// Process all .word entries in sorted order
	for (int i = 0; i < word_entries_count; i++) {
		// Add blanks until we reach the .word address
		while (linesCounter < word_entries[i].address) {
			fprintf(outputFile, "00000000\n");
			linesCounter++;
		}
		// Add the .word data at the correct address
		if (word_entries[i].address == linesCounter) {
			fprintf(outputFile, "%08X\n", word_entries[i].data);
			linesCounter++;
		}
	}

	return linesCounter;
}

// Checks what the given word is
int lookup_word(const char* word) {
	if (word[0] == '$') {
		for (int i = 0; i < register_table_size; i++) {
			if (strcmp(register_table[i].RegisterEntryName, word) == 0) {
				return register_table[i].number;
			}
		}
		return -1;
	}
	else {
		for (int i = 0; i < opcode_table_size; i++) {
			if (strcmp(opcode_table[i].OpCodeName, word) == 0) {
				return opcode_table[i].code;
			}
		}
		return -1;
	}
}

// Gets a word and removes spaces before the actual word
char* remove_Spaces(const char* word) {
	int j = 0;
	for (int i = 0; i < LABLE_MAX && word[i] != '\0'; i++) {
		if (word[i] != ' ' && word[i] != '\t') {
			j++;
		}
	}
	char* result = malloc(j + 1);

	int k = 0;
	for (int i = 0; i < LABLE_MAX && word[i] != '\0'; i++) {
		if (word[i] != ' ' && word[i] != '\t') {
			result[k++] = word[i];
		}
	}
	result[k] = '\0';
	return result;
}

// Returns the address of the given Label
int lookup_label(const char* word, Lable struct_lb[], int label_num) {
	char* result = remove_Spaces(word);
	for (int i = 0; i < label_num; i++) {
		if (strcmp(struct_lb[i].LableName, result) == 0) {
			free(result);
			return struct_lb[i].position;
		}
	}
	free(result);
	return -999;
}

// Changes line to hexa in the correct format and writes the line in the output file
int line_to_hexa(char* line, Lable struct_lb[], FILE* outputFile, int label_num) {
	char* token;
	int word_index = 0;
	unsigned int encoded = 0;
	int values[5] = { 0 };
	int immediate = 0;
	int is_label = 0;
	int bigimm = 0;
	int counter = 0;

	remove_comments(line);

	// Skip leading spaces
	while (*line == ' ' || *line == '\t') { line++; }

	// Tokenize and parse
	token = strtok(line, " ,\n");
	if (token == NULL) {
		return 0;
	}

	while (token != NULL && counter < 5) {
		if (word_index < 4) {
			values[word_index] = lookup_word(token);
		}
		else {
			int label_address = lookup_label(token, struct_lb, label_num);
			if (label_address != -999) {
				immediate = label_address;
				is_label = 1;
			}
			else {
				immediate = parse_number(token); // Use our improved parse_number function
				is_label = 0;
			}
		}
		word_index++;
		token = strtok(NULL, " ,\n");
		counter++;
	}

	// Determine if bigimm is needed
	if (is_label || immediate < -128 || immediate > 127)
		bigimm = 1;

	// Encode the instruction
	encoded |= (values[0] & 0xFF) << 24;       // opcode
	encoded |= (values[1] & 0xF) << 20;        // rd
	encoded |= (values[2] & 0xF) << 16;        // rs
	encoded |= (values[3] & 0xF) << 12;        // rt
	encoded |= 0 << 9;                         // reserved
	encoded |= bigimm << 8;                    // bigimm flag
	encoded |= (bigimm ? 0 : (immediate & 0xFF));  // imm8 (if not bigimm)

	fprintf(outputFile, "%08X\n", encoded);

	if (bigimm) {
		fprintf(outputFile, "%08X\n", immediate & 0xFFFFFFFF);
		return 2;
	}
	return 1;
}