.text
# R-type Instructions
add x1, x2, x3
sub x4, x5, x6
and x7, x8, x9
or x10, x11, x12
xor x13, x14, x15
sll x16, x17, x18
srl x19, x20, x21
sra x22, x23, x24
slt x25, x26, x27

# I-type Instructions
addi x1, x2, 10
andi x3, x4, 15
ori x5, x6, 20
lw x15, 16(x16)

# S-type Instructions
sw x1, 20(x2)
sh x3, 24(x4)
sb x5, 28(x6)

# SB-type (Branch) Instructions
beq x1, x2, label1
bne x3, x4, label2
blt x5, x6, label3
bge x7, x8, label4

# U-type Instructions
lui x1, 4096
auipc x2, 8192

# UJ-type Instructions
jal x1, label5

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
