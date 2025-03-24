.text
addi x3 x0 3   # value of n
addi x4 x0 1   
jal x1 fact
jal x0 exit

fact:
addi sp sp -8
sw x1 4(sp)
sw x3 0(sp)
bne x3 x4 fact1
addi x3 x0 1
lw x1 4(sp)
addi sp sp 8
jalr x0 x1 0
fact1:
addi x3 x3 -1
jal x1 fact
lw x6 0(sp)
lw x1 4(sp)
addi sp sp 8
mul x3 x3 x6
jalr x0 x1 0


exit: