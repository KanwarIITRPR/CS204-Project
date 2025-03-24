.text

    sw x10, 0(x12)         # Store F0 in memory
    addi x12, x12, 4       # Move to next memory location
    sw x11, 0(x12)         # Store F1 in memory
    addi x12, x12, 4       # Move to next memory location

    addi x20, x20, -2      # Adjust n (since we stored first two values)
    blez x20, exit      # If n <= 2, exit

fibonacci_loop:
    add x13, x10, x11      # Compute F(n) = F(n-1) + F(n-2)
    sw x13, 0(x12)         # Store in memory
    addi x12, x12, 4       # Move to next memory location

    addi x10, x11 0           # Shift F(n-1) to F(n-2)
    addi x11, x13 0           # Shift F(n) to F(n-1)

    addi x20, x20, -1      # Decrease n
    bnez x20, fibonacci_loop  # Repeat if n > 0

exit:
    li a7, 10              # Exit system call
    ecall