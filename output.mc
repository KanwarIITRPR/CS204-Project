.data
0x10000000 0x03
0x10000001 0x14
0x10000002 0x000000ab
0x10000006 0x03
0x10000007 0x61
0x10000008 0x00
0x10000009 END-OF-DATA-SEGMENT
.text
0x00000000 0x00A10083  lb x1, x2, 10  # 0000011 - 000 - NULL - 00001 - 00010 - NULL - 000000001010
0x00000004 0x00200113  addi x2, x0, 2  # 0010011 - 000 - NULL - 00010 - 00000 - NULL - 000000000010
0x00000008 0x04008463  beq x1, x0, 36  # 1100011 - 000 - NULL - NULL - 00001 - 00000 - 000000100100
0x0000000c 0x04208063  beq x1, x2, 32  # 1100011 - 000 - NULL - NULL - 00001 - 00010 - 000000100000
0x00000010 0x04419063  bne x3, x4, 32  # 1100011 - 001 - NULL - NULL - 00011 - 00100 - 000000100000
0x00000014 0x02019C63  bne x3, x0, 28  # 1100011 - 001 - NULL - NULL - 00011 - 00000 - 000000011100
0x00000018 0x0262CC63  blt x5, x6, 28  # 1100011 - 100 - NULL - NULL - 00101 - 00110 - 000000011100
0x0000001c 0x030000EF  jal x1, 24  # 1101111 - NULL - NULL - 00001 - NULL - NULL - 00000000000000011000
0x00000020 0xABCE00B7  lui x1, 703712  # 0110111 - NULL - NULL - 00001 - NULL - NULL - 10101011110011100000
0x00000024 0x05508093  addi x1, x1, -85  # 0010011 - 000 - NULL - 00001 - 00001 - NULL - 000001010101
0x00000028 0x03C18193  addi x3, x3, 60  # 0010011 - 000 - NULL - 00011 - 00011 - NULL - 000000111100
0x0000002c 0x0580006F  jal x0, 16  # 1101111 - NULL - NULL - 00000 - NULL - NULL - 00000000000000101100
0x00000030 0x00B504B3  add x9, x10, x11  # 0110011 - 000 - 0000000 - 01001 - 01010 - 01011 - NULL
0x00000034 0x40E68633  sub x12, x13, x14  # 0110011 - 000 - 0100000 - 01100 - 01101 - 01110 - NULL
0x00000038 0x011877B3  and x15, x16, x17  # 0110011 - 111 - 0000000 - 01111 - 10000 - 10001 - NULL
0x0000003c 0x0149E933  or x18, x19, x20  # 0110011 - 110 - 0000000 - 10010 - 10011 - 10100 - NULL
0x00000040 0x017B4AB3  xor x21, x22, x23  # 0110011 - 100 - 0000000 - 10101 - 10110 - 10111 - NULL
0x00000044 END-OF-TEXT-SEGMENT
