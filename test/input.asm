.data
length: .byte 13
array: .byte 1, 2, 4, 6, 7, 8, 10, 11, 13, 15, 17, 18, 20
to_find: .byte 10
.text
main:
    # Loading the base address of th array into x10, length into x11 and final address into x12
    lui x11, 0x10000
    lb x12, 0(x11)
    addi x11, x11, 1
    
    add x13, x11, x12
    lb x13, 0(x13)

    # Call the function
    jal x1, BinarySearch

    # End the program
    jal x0, exit

BinarySearch:
    addi x10, x0, -1
    addi x14, x0, 0
    addi x15, x12, -1

    addi x2, x2, -4
    sw x1, 0(x2)

    jal x1, BinarySearch_Recursive

    lw x1, 0(x2)
    addi x2, x2, 4

    blt x15, x14, not_found
    add x10, x14, x15
    addi x20, x0, 2
    div x10, x10, x20
    add x10, x10, x11
    
    not_found:
        jalr x0, x1, 0

BinarySearch_Recursive:
    bge x15, x14, continue
    jalr x0, x1, 0

    continue:
        addi x20, x0, 2
        add x16, x14, x15
        div x16, x16, x20

        add x16, x16, x11
        lb x16, 0(x16)

        blt x13, x16, reduce_left
        blt x16, x13, reduce_right
        jalr x0, x1, 0

        reduce_left:
            addi x14, x16, 1
            jal x0, return
        reduce_right:
            addi x15, x16, -1
            jal x0, return

    return:
        addi x2, x2, -4
        sw x1, 0(x2)

        jal x1, BinarySearch_Recursive

        lw x1, 0(x2)
        addi x2, x2, -4

        jalr x0, x1, 0

# def BS(arr_add, l, r):
#   mid = (left + right) / 2
#   if (arr[mid] > req) return BS(arr_add, mid + 1, r);
#   else if (arr[mid] < req) return BS(arr_add, l, mid - 1);
#   else return mid
exit: