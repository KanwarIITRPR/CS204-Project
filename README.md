# RISC-V Assembler

## Overview

This RISC-V assembler is a comprehensive tool that converts RISC-V assembly code into machine code. 

It supports a wide range of standard RISC-V instruction formats and data directives, producing binary representations of each instruction along with their corresponding hexadecimal machine code and memory addresses.

## Table of Contents

- [Features](#features)
- [Project Structure](#project-structure)
- [How It Works](#how-it-works)
  - [Assembly Process](#assembly-process)
  - [Instruction Format Handling](#instruction-format-handling)
- [Installation](#installation)
- [Usage](#usage)
- [Output Format](#output-format)
- [Memory Layout](#memory-layout)
- [Error Handling](#error-handling)
- [Contributors](#contributors)

## Features

- **Complete RISC-V Instruction Support**:
  - **R-type**: add, sub, and, or, xor, sll, srl, sra, slt, mul, div, rem
  - **I-type**: addi, andi, ori, jalr
  - **Load instructions**: lb, ld, lh, lw
  - **S-type/Store instructions**: sb, sh, sw, sd
  - **SB-type/Branch instructions**: beq, bne, bge, blt
  - **U-type**: auipc, lui
  - **UJ-type**: jal

- **Data Directive Support**:
  - `.word`: 32-bit data
  - `.half`: 16-bit data
  - `.byte`: 8-bit data
  - `.dword`: 64-bit data
  - `.asciiz`: Null-terminated ASCII strings

- **Label Management**:
  - Support for defining and referencing labels
  - Two-pass assembly approach for resolving label addresses

- **Detailed Output**:
  - Memory addresses in hexadecimal
  - Machine code in hexadecimal
  - Original instruction or data directive
  - Binary encoding details for instructions
  - Section markers for text and data segments

## Project Structure

```
README.md          # This file
risc-v-assembler/
├── main.cpp       # Main driver program
├── commands.cpp   # Instruction encodings and format checking
└── utils.cpp      # Utility functions
example/
├── a.exe          # Compiled program
├── input.asm      # RISC-V assembly program to be assembled
└── output.mc      # Assembled machine code
attachments/
└── ok.svg         # Diagram explaining the working of the assembler
```

### File Descriptions

1. **main.cpp**: The main driver program that orchestrates the assembly process, including file I/O, two-pass assembly, and output generation.

2. **commands.cpp**: Defines opcode, function code, and other binary encodings for RISC-V instructions according to the RISC-V specification.

3. **utils.cpp**: Contains utility functions for binary/hexadecimal conversions, string manipulation, and other helper functions.

## How It Works


<img src="ok.svg" alt="Alt text" width="75%" height="75%">


### Assembly Process

The assembler uses a two-pass approach to resolve labels and generate machine code:



#### First Pass
1. Scan through the entire assembly file
2. Record the addresses of all labels
3. Track the current address in both text and data segments
4. Distinguish between `.text` and `.data` sections

#### Second Pass
1. Process each instruction and data directive
2. For instructions:
   - Tokenize the instruction
   - Generate binary code based on instruction format
   - Convert binary to hexadecimal
   - Output memory address, machine code, and instruction details
3. For data directives:
   - Process `.word`, `.half`, `.byte`, `.dword`, and `.asciiz` directives
   - Output memory address and data values

## Installation

### Build from Source
1. Clone this repository:
   ```bash
   git clone "https://github.com/yourusername/CS204-Project.git"
   cd risc-v-assembler
   ```

2. Compile the source code:
   ```bash
   g++ -std=c++11 main.cpp -o riscv-assembler
   ```

## Usage

```bash
./riscv-assembler <input_file> <output_file>
```

Where:
- `<input_file>` is the path to the RISC-V assembly source file
- `<output_file>` is the path where the assembled output will be written

### Example
```bash
./riscv-assembler program.asm program.out
```

### Input Assembly Format

The assembler expects a standard RISC-V assembly format (space separated) with the following sections:

```assembly
.text               # Code section
    add x1 x2 x3         # Instructions
    lb x4 0(x5)

.data               # Data section
    var1: .word 42       # Data declarations
```

## Output Format

The output file contains:
- Memory addresses (in hexadecimal)
- Machine code (in hexadecimal)
- Original instruction or data directive
- For instructions: binary encoding details (opcode, function codes, registers, etc.)
- Markers for the end of text and data sections

### Example Output
```
0x00000000 0x00A62023 , sw x10,0(x12) # 0100011-010-NULL-00000-01100-01010-NULL
0x00000004 0x00358393 , addi x7,x11,3 # 0010011-000-00011-01011-00111-NULL-NULL
...
End of text segment
0x10000000 0x00000001 , .word 1
0x10000004 0x00000002 , .word 2
...
End of data segment
```

## Contributors

- 2023csb1102	Aryan Singh
- 2023csb1126	Kanwarveer Singh Chadha
- 2023csb1147	Pratham Garg
