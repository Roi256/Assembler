# disktest.asm - Simplified Disk Sector Summation Program for SIMP

# Memory layout for sector buffers:
# 0x200-0x27F: Sector 0 data (128 words)
# 0x280-0x2FF: Sector 1 data (128 words)  
# 0x300-0x37F: Sector 2 data (128 words)
# 0x380-0x3FF: Sector 3 data (128 words)
# 0x400-0x47F: Sector 4 results (128 words)

main:
    # Step 1: Read all 4 sectors sequentially
    
    # Read sector 0 to address 0x200
    add $t0, $zero, $imm, 0        # Sector number = 0
    add $t1, $zero, $imm, 0x200    # Buffer address = 0x200
    beq $imm, $zero, $zero, read_sector_0
    
read_sector_0:
    out $t0, $zero, $imm, 15       # Set DISKSECTOR = 0
    out $t1, $zero, $imm, 16       # Set DISKBUFFER = 0x200  
    add $t2, $zero, $imm, 1        # Read command = 1
    out $t2, $zero, $imm, 14       # Execute DISKCMD = 1 (read)
    
wait_sector_0:
    in $t2, $zero, $imm, 17        # Check DISKSTATUS
    bne $imm, $t2, $zero, wait_sector_0  # Wait until status = 0 (ready)
    
    # Read sector 1 to address 0x280
    add $t0, $zero, $imm, 1        # Sector number = 1
    add $t1, $zero, $imm, 0x280    # Buffer address = 0x280
    out $t0, $zero, $imm, 15       # Set DISKSECTOR = 1
    out $t1, $zero, $imm, 16       # Set DISKBUFFER = 0x280
    add $t2, $zero, $imm, 1        # Read command = 1  
    out $t2, $zero, $imm, 14       # Execute DISKCMD = 1 (read)
    
wait_sector_1:
    in $t2, $zero, $imm, 17        # Check DISKSTATUS
    bne $imm, $t2, $zero, wait_sector_1  # Wait until status = 0
    
    # Read sector 2 to address 0x300
    add $t0, $zero, $imm, 2        # Sector number = 2
    add $t1, $zero, $imm, 0x300    # Buffer address = 0x300
    out $t0, $zero, $imm, 15       # Set DISKSECTOR = 2
    out $t1, $zero, $imm, 16       # Set DISKBUFFER = 0x300
    add $t2, $zero, $imm, 1        # Read command = 1
    out $t2, $zero, $imm, 14       # Execute DISKCMD = 1 (read)
    
wait_sector_2:
    in $t2, $zero, $imm, 17        # Check DISKSTATUS  
    bne $imm, $t2, $zero, wait_sector_2  # Wait until status = 0
    
    # Read sector 3 to address 0x380
    add $t0, $zero, $imm, 3        # Sector number = 3
    add $t1, $zero, $imm, 0x380    # Buffer address = 0x380
    out $t0, $zero, $imm, 15       # Set DISKSECTOR = 3
    out $t1, $zero, $imm, 16       # Set DISKBUFFER = 0x380
    add $t2, $zero, $imm, 1        # Read command = 1
    out $t2, $zero, $imm, 14       # Execute DISKCMD = 1 (read)
    
wait_sector_3:
    in $t2, $zero, $imm, 17        # Check DISKSTATUS
    bne $imm, $t2, $zero, wait_sector_3  # Wait until status = 0
    
    # Step 2: Calculate sums for all 128 words
    add $s0, $zero, $zero, 0       # Loop counter i = 0
    add $s1, $zero, $imm, 128      # Loop limit = 128
    
sum_loop:
    # Check exit condition: i >= 128
    bge $imm, $s0, $s1, write_results
    
    # Calculate addresses for word i in each sector
    add $t0, $zero, $imm, 0x200    # Base address sector 0
    add $a0, $t0, $s0, 0           # sector0[i] address
    
    add $t0, $zero, $imm, 0x280    # Base address sector 1  
    add $a1, $t0, $s0, 0           # sector1[i] address
    
    add $t0, $zero, $imm, 0x300    # Base address sector 2
    add $a2, $t0, $s0, 0           # sector2[i] address
    
    add $t0, $zero, $imm, 0x380    # Base address sector 3
    add $a3, $t0, $s0, 0           # sector3[i] address
    
    # Load the 4 values
    lw $t0, $a0, $zero, 0          # $t0 = sector0[i]
    lw $t1, $a1, $zero, 0          # $t1 = sector1[i]
    lw $t2, $a2, $zero, 0          # $t2 = sector2[i]  
    lw $v0, $a3, $zero, 0          # $v0 = sector3[i]
    
    # Calculate sum
    add $t0, $t0, $t1, 0           # $t0 = sector0[i] + sector1[i]
    add $t0, $t0, $t2, 0           # $t0 += sector2[i]
    add $t0, $t0, $v0, 0           # $t0 += sector3[i] (final sum)
    
    # Store result in sector 4 buffer
    add $t1, $zero, $imm, 0x400    # Base address sector 4
    add $t1, $t1, $s0, 0           # sector4[i] address  
    sw $t0, $t1, $zero, 0          # sector4[i] = sum
    
    # Increment counter and continue
    add $s0, $s0, $imm, 1          # i++
    beq $imm, $zero, $zero, sum_loop
    
write_results:
    # Step 3: Write sector 4 to disk
    add $t0, $zero, $imm, 4        # Sector number = 4
    add $t1, $zero, $imm, 0x400    # Buffer address = 0x400
    out $t0, $zero, $imm, 15       # Set DISKSECTOR = 4
    out $t1, $zero, $imm, 16       # Set DISKBUFFER = 0x400
    add $t2, $zero, $imm, 2        # Write command = 2
    out $t2, $zero, $imm, 14       # Execute DISKCMD = 2 (write)
    
wait_write:
    in $t2, $zero, $imm, 17        # Check DISKSTATUS
    bne $imm, $t2, $zero, wait_write     # Wait until status = 0
    
    # Program complete
    halt $zero, $zero, $zero, 0

# Register usage summary:
# =======================
# $s0 - loop counter i (0 to 127)
# $s1 - loop limit (128)  
# $t0 - sector number, calculations, loaded values
# $t1 - buffer addresses, loaded values
# $t2 - disk commands, loaded values
# $v0 - loaded values from sector 3
# $a0-$a3 - memory addresses for current word in each sector

# Disk I/O registers:
# ===================
# 14 (DISKCMD): 1=read, 2=write
# 15 (DISKSECTOR): sector number (0-6 available)  
# 16 (DISKBUFFER): memory address for sector data
# 17 (DISKSTATUS): 0=ready, 1=busy

# Memory organization:
# ====================
# 0x200-0x27F: 128 words from sector 0
# 0x280-0x2FF: 128 words from sector 1
# 0x300-0x37F: 128 words from sector 2
# 0x380-0x3FF: 128 words from sector 3  
# 0x400-0x47F: 128 sum results for sector 4

# Algorithm flow:
# ===============
# 1. Read sectors 0,1,2,3 into consecutive memory buffers
# 2. For each word position i (0-127):
#    a. Load word i from all 4 sector buffers
#    b. Sum the 4 values  
#    c. Store sum in sector 4 buffer at position i
# 3. Write sector 4 buffer back to disk
# 4. Halt program

# Expected output:
# ================
# Sector 4 on disk will contain 128 words where:
# word[i] = sector0[i] + sector1[i] + sector2[i] + sector3[i]