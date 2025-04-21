.data
n: .byte 5

.text
main:
    # Storing n the value for which we have to calculate the factorial in x11
    lui x11, 0x10000
    lb x11, 0(x11)
    
    # Initializing the value of x17 to display -1 if error was found
    addi x17, x0, -1

    jal x1, recursive_factorial
    jal x0, exit

# This is a wrapper function for run_factorial to initialize necessary variables to avoid re-assigning constant variables
# This function also conducts initial checks to avoid infinite recursion
recursive_factorial:
    # Initializing temporaries
    addi x18 x0 1
    jal x0 run_factorial

# Function Result is stored in x17
# Parameter for the function is the number for which we have to find the factorial (stored in x11)
# Temporaries used in the function are the comparator for n (1 stored in x18)
run_factorial:
    # Storing the function's current state
    addi x2 x2 -5
    sw x1 1(x2)
    sb x11 0(x2)

    # Checking for errors and base case
    addi x18 x0 1
    blt x11 x0 exit
    bge x18 x11 base_case
    
    # Performing Recursion
    addi x11 x11 -1
    jal x1 run_factorial

    # Retrieving the current function's variables before running any other command in the current function
    lb x11 0(x2)
    lw x1 1(x2)
    addi x2 x2 5

    # Running final return operation
    mul x17 x11 x17
    jal x0 exit_recursion

    base_case:
        addi x17 x0 1

    exit_recursion:
        jalr x0 x1 0
exit: