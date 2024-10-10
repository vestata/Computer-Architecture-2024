.data
op1:     .word 0,0,0,0,1,0,0,0,1,1,1,1,1,0,1,0,0,0,1,0,0,0,0,0,1,1,0,1,0,0,1,1,1,1,1
k1_low:  .word 0xD3E80C15
k1_high: .word 0x2
n1:      .word 35
ans1:    .word 0x69

op2:     .word 0,1,0,1
k2_low:  .word 10
k2_high: .word 0
n2:      .word 4
ans2:    .word 0x62

op3:     .word 1,1,1,0,1,0,1,0,1,0
k3_low:  .word 25
k3_high: .word 0
n3:      .word 10
ans3:    .word 0x62

.text
main:
    lw a0, k1_low
    lw a1, k1_high
    la a2, op1
    lw a3, n1
    jal ra, kthCharacter
    li  a7, 10        # end program
    ecall

my_clz64:
    # Input: 
    # a0 (lower 32 bits of x)
    # a1 (upper 32 bits of x)
    # Output: 
    # a0 (clz)      

    or t0, a0, a1
    beqz t0, zero_case       # if x == 0
    
    li t1, 0                 # set n = 0
    
    bnez a1, shift32_skip
    addi t1, t1, 32          # n += 32
    mv a1, a0                # x <<= 32
    li a0, 0                 # x_low = 0
        
shift32_skip:
    # if (x_hi <= 0x0000ffff)
    li t2, 0xffff
    sltu t3, t2, a1          # t3(flag) = (0xfff < x_hi) 
    bnez t3, shift16_skip    # if (x_hi <= 0000ffff)
    addi t1, t1, 16          # n += 16
    slli a1, a1, 16          # x_hi <<= 16
    mv t3, a0
    srli t3, t3, 16
    or a1, a1, t3            # x_hi = (x_hi << 16) | (x_lo >> 16)
    slli a0, a0, 16          # 
    
shift16_skip:
    # if (x_hi <= 0x00ffffff)
    li t2, 0xffffff
    sltu t3, t2, a1
    bnez t3, shift8_skip
    addi t1, t1, 8           # n += 8
    slli a1, a1, 8           # x_hi <<= 8
    mv t3, a0                # set a extra register to preserve the bits 
                             # being shifted
    srli t3, t3, 8
    or a1, a1, t3            # x_hi = (x_hi << 8) | (x_lo >> 8)
    slli a0, a0, 8           # 

shift8_skip:
    # if (x_hi <= 0x0fffffff)
    li t2, 0xfffffff
    sltu t3, t2, a1
    bnez t3, shift4_skip
    addi t1, t1, 4           # n += 4
    slli a1, a1, 4           # x_hi <<= 4
    mv t3, a0
    srli t3, t3, 4
    or a1, a1, t3            # x_hi = (x_hi << 4) | (x_lo >> 4)
    slli a0, a0, 4           # 

shift4_skip:
    # if (x_hi <= 0x3fffffff)
    li t2, 0x3fffffff
    sltu t3, t2, a1
    bnez t3, shift2_skip
    addi t1, t1, 2           # n += 2
    slli a1, a1, 2           # x_hi <<= 2
    mv t3, a0
    srli t3, t3, 2
    or a1, a1, t3            # x_hi = (x_hi << 2) | (x_lo >> 2)
    slli a0, a0, 2           # 

shift2_skip:
    # if (x_hi <= 0x7fffffff)
    li t2, 0x7fffffff
    sltu t3, t2, a1
    bnez t3, clz_end
    addi t1, t1, 1           # n += 1
    slli a1, a1, 1           # x_hi <<= 1
    mv t3, a0
    srli t3, t3, 1
    or a1, a1, t3            # x_hi = (x_hi << 1) | (x_lo >> 1)
    slli a0, a0, 1           # 
    
clz_end:
    mv a0, t1                # return n   
    ret

zero_case:
    li a0 64                 # retrun 64      
    ret
    
ilog2:
    addi sp, sp, -4
    sw ra, 0(sp)
    jal ra, my_clz64
    lw ra, 0(sp) 
    addi sp, sp, 4
    li t0, 63
    sub a0, t0, a0
    
    ret

kthCharacter:
    # Inputs:
    # a0: k_low (lower 32 bits of k)
    # a1: k_high (upper 32 bits of k)
    # a2: operations (pointer to array)
    # a3: operationsSize
    # Output:
    # a0: resulting character
    addi sp, sp, -28
    sw ra, 24(sp)             # save return address
    sw s0, 20(sp)             # save mutations
    sw s1, 16(sp)             # save op
    sw s2, 12(sp)             # save k_low
    sw s3, 8(sp)              # save k_high
    sw s4, 4(sp)              # save operations address
    sw s5, 0(sp)              # save operationsSize
    
    mv s4, a2
    mv s5, s3
    
    li s0, 0                  # set s0 to mutation = 0
    mv s2, a0
    mv s3, a1
    
    jal ra, ilog2             # call ilog2, result in a0
    mv s1, a0                 # set s1 = op

loop:
    bltz s1, loop_end         # branch if op < 0
        
    mv a0, s1                 # a0 = op
    jal ra, shift_left_64     # return a0, a1 for 64 bit data
    mv t0, a0                 # set t0 = shift_reult_low
    mv t1, a1                 # set t1 = shift_reult_high
    
    mv a0, t0                 # set a0 = shift_result_low
    mv a1, t1                 # set a1 = shift_result_high
    mv a2, s2                 # set a2 = k_low
    mv a3, s3                 # set a3 = k_high
    jal ra, blt_64            # blt 1LL << op, k, return boolean result 
                              # in a0
    beqz a0, skip_if
    
    mv a0, s2                 # a0 = k_low
    mv a1, s3                 # a1 = k_high
    mv a2, t0                 # a2 = shift_result_low
    mv a3, t1                 # a3 = shift_result_high
    jal ra, sub_64            # return result in a0, a1
    
    # update k
    mv s2, a0                 # s2 = k_low
    mv s3, a1                 # s3 = k_high
    
    # ??????
    # operation[op]
    slli t0, s1, 2
    add t1, s4, t0
    lw t2, 0(t1)
        
    add s0, s0, t2            # s0（mutations） += operations[op]
    
skip_if:
    addi s1, s1, -1           # op--
    j loop

loop_end:
    li t0, 26
    rem t1, s0, t0            # set t1 = mutations % 26
    li t0, 0x61               # set t0 = ascii of 'a'
    add a0, t0, t1            # a0 = 'a' + (mutations % 26)
        
    lw s5, 0(sp)              
    lw s4, 4(sp)              
    lw s3, 8(sp)              
    lw s2, 12(sp)             
    lw s1, 16(sp)             
    lw s0, 20(sp)             
    lw ra, 24(sp)             
    addi sp, sp, 28
    ret
    
# To handle left shift for 64-bit long long
shift_left_64:
    # Inputs(long long):
    # a0: shift amount(op)
    # Outputs(long long):
    # a0: lower 32 bits
    # a1: upper 32 bits

    li t4, 32
    blt a0, t4, shift_less_32      # check if shift over the                                                 # representation of 32-bit
    # left shift more than 31bits
    addi a0, a0, -32                # 
    li a1, 1                        #
    sll a1, a1, a0                  # set x_hi = 1 << (op - 32)
    li a0, 0                        # set x_low = 0
    j end

shift_less_32:
    # left shift more than 32 bits
    li a1, 1                        # temp set 1
    sll a0, a1, a0                  # set x_low = 1LL << op 
    li a1, 0                        # set x_hi = 0
    j end

end:
    ret
    

# To handle comaprison of two long long data
blt_64:
    # Inputs(two long long):
    # a0: x1_low
    # a1: x1_high
    # a2: x2_low
    # a3: x2_high
    # Outputs(long long):
    # a0: boolean

    bltu a1, a3, true
    beq a1, a3, equal
false:
    li a0, 0                       # x1_high > x2_high, return false
    ret

true:
    # x1 < x2, return true
    li a0, 1                  
    ret

equal:
    # if x1_high = x2_high, compare lower
    bltu a0, a2, true               # x1_low < x2_low, return true
    j false


# To handle substraction of two long long data
sub_64:
    # Inputs(two long long):
    # a0: x1_low
    # a1: x1_high
    # a2: x2_low
    # a3: x2_high
    # Outputs(long long):
    # a0: r_low
    # a1: r_high
    
    sltu t0, a0, a2             # t0 = (a0 < a2) ? 1 : 0, check if borrow
    sub a0, a0, a2              # a0 = a0 - a2
    sub a1, a1, a3              # a1 = a1 - a3
    sub a1, a1, t0              # a1 = a1 - t0
    ret