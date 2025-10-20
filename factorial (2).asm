# factorial.asm - Recursive factorial implementation for SIMP processor

.word 0x100 5    # Store n=5 at address 0x100

add $sp, $zero, $imm, 4095        # Initialize stack pointer to top of memory
lw $a0, $zero, $imm, 0x100        # Load n from memory[0x100] into $a0
jal $ra, $imm, $zero, factorial   # Call factorial(n)
sw $v0, $imm, $zero, 0x101        # Store the result in memory[0x101]
halt $zero, $zero, $zero, 0       # halt program

factorial:
    # Save return address and n on stack
    sub $sp, $sp, $imm, 2         # Allocate space on stack (2 words)
    sw $ra, $sp, $imm, 1          # Save return address to stack[1]
    sw $a0, $sp, $imm, 0          # Save input n to stack[0]
    beq $imm, $a0, $zero, base_case # if (a0 == 0), goto base_case

    # Recursive case: calculate factorial(n-1)
    sub $a0, $a0, $imm, 1         # $a0 = n - 1
    jal $ra, $imm, $zero, factorial # Recursive call

    # Restore n from stack
    lw $a0, $sp, $imm, 0          # $a0 = MEM[$sp + 0] (restore n)

    # Multiply result by n
    mul $v0, $v0, $a0, 0          # $v0 = $v0 * $a0

jr:
    lw $ra, $sp, $imm, 1          # $ra = MEM[$sp + 1] (restore return address)
    add $sp, $sp, $imm, 2         # Deallocate stack space
    beq $ra, $zero, $zero, 0      # jump to $ra

base_case:
    add $v0, $zero, $imm, 1       # $v0 = 1
    beq $imm, $zero, $zero, jr    # jump to return code