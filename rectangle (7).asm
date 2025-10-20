# rectangle.asm - Draw Filled White Rectangle on SIMP Monitor

# Test data - small rectangle for verification
.word 0x100 0x0A05    # A: x=5, y=10 (0x05, 0x0A)
.word 0x101 0x0000    # B: dont care
.word 0x102 0x0F14    # C: x=20, y=15 (0x14, 0x0F) 
.word 0x103 0x0000    # D: dont care

main:                               
    # Step 1: Read coordinates from memory
    add $t0, $zero, $imm, 0x100         # Read A coordinates (top-left corner)
    lw $s0, $t0, $zero, 0               # $s0 = A coordinates
    add $t0, $zero, $imm, 0x102         # Read C coordinates (bottom-right corner)
    lw $s1, $t0, $zero, 0               # $s1 = C coordinates
    
    # Step 2: Extract coordinates from packed format
    and $a0, $s0, $imm, 0xFF            # A_x = bits 7:0
    srl $a1, $s0, $imm, 8               # Shift right by 8 bits
    and $a1, $a1, $imm, 0xFF            # A_y = bits 15:8 only
    and $a2, $s1, $imm, 0xFF            # C_x = bits 7:0
    srl $a3, $s1, $imm, 8               # Shift right by 8 bits
    and $a3, $a3, $imm, 0xFF            # C_y = bits 15:8 only
    
    # Step 3: Calculate rectangle parameters
    # Calculate width = C_x - A_x + 1
    sub $t0, $a2, $a0, 0                # $t0 = C_x - A_x
    add $t0, $t0, $imm, 1               # $t0 = width = C_x - A_x + 1
    
    # Calculate height = C_y - A_y + 1
    sub $t1, $a3, $a1, 0                # $t1 = C_y - A_y
    add $t1, $t1, $imm, 1               # $t1 = height = C_y - A_y + 1
    
    # Step 4: Calculate starting offset in frame buffer
    add $t2, $zero, $imm, 256           # $t2 = 256 (screen width)
    mul $v0, $a1, $t2, 0                # $v0 = A_y * 256
    add $v0, $v0, $a0, 0                # $v0 = start_offset = A_y * 256 + A_x
    
    # Step 5: Prepare drawing constants
    add $s0, $zero, $imm, 255           # $s0 = white color (255)
    add $s1, $zero, $imm, 1             # $s1 = write command (1)
    
    # Step 6: Initialize outer loop (rows)
    add $a1, $zero, $zero, 0            # $a1 = row_counter = 0
    
row_loop:
    bge $imm, $a1, $t1, draw_complete   # Check exit condition: row_counter >= height
    add $a0, $zero, $zero, 0            # $a0 = col_counter = 0
    
col_loop:
    bge $imm, $a0, $t0, end_row         # Check exit condition: col_counter >= width
    
    # Calculate current pixel offset = start_offset + row*256 + col
    mul $a2, $a1, $t2, 0                # $a2 = row_counter * 256
    add $a2, $a2, $v0, 0                # $a2 = start_offset + row*256
    add $a2, $a2, $a0, 0                # $a2 = final_offset = start_offset + row*256 + col
    
    # Draw the pixel using I/O registers
    # 1. Set pixel address
    out $a2, $zero, $imm, 20            # MONITORADDR = offset
    
    # 2. Set pixel color (white = 255)
    out $s0, $zero, $imm, 21            # MONITORDATA = 255
    
    # 3. Execute write command
    out $s1, $zero, $imm, 22            # MONITORCMD = 1
    
    # Increment column counter
    add $a0, $a0, $imm, 1               # col_counter++
    
    beq $imm, $zero, $zero, col_loop    # Return to start of inner loop
    
end_row:
    add $a1, $a1, $imm, 1               # row_counter++
    beq $imm, $zero, $zero, row_loop    # Return to start of outer loop

draw_complete:
    halt $zero, $zero, $zero, 0         # Program finished - halt processor



