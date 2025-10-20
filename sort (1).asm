add $t2, $zero, $imm, 0x100         # $t2 = 0x100, base address of the array
add $t0, $zero, $zero, 0            # $t0 = 0, outer loop index (i)

outer_loop:
    add $v0, $zero, $imm, 14        # $v0 = 14
    bgt $imm, $t0, $v0, sort_complete  # if i > 14, done sorting
    add $t1, $zero, $zero, 0        # $t1 = 0, inner loop index (j)

    sub $s2, $v0, $t0, 0            # $s2 = 14 - i => max j value

inner_loop:
    bgt $imm, $t1, $s2, inner_complete  # if j > (14 - i), exit inner loop

    add $a0, $t2, $t1, 0            # $a0 = base + j => addr[j]
    add $a1, $a0, $imm, 1           # $a1 = base + j + 1 => addr[j+1]

    lw $s0, $a0, $zero, 0           # $s0 = array[j]
    lw $s1, $a1, $zero, 0           # $s1 = array[j+1]

    ble $imm, $s0, $s1, no_swap     # if array[j] <= array[j+1], skip

    sw $s1, $a0, $zero, 0           # array[j] = array[j+1]
    sw $s0, $a1, $zero, 0           # array[j+1] = original array[j]

no_swap:
    add $t1, $t1, $imm, 1           # j++
    beq $imm, $zero, $zero, inner_loop

inner_complete:
    add $t0, $t0, $imm, 1           # i++
    beq $imm, $zero, $zero, outer_loop

sort_complete:
    halt $zero, $zero, $zero, 0     # Done