.text
addi x1 x0 2
addi x2 x0 2
# SB-type (Branch) Instructions
beq x1, x2, label1
bne x3, x4, label2
blt x5, x6, label3
bge x7, x8, label4

# Labels for branch instructions
label1:
    add x9, x10, x11
label2:
    sub x12, x13, x14
label3:
    and x15, x16, x17
label4:
    or x18, x19, x20
label5:
    xor x21, x22, x23
