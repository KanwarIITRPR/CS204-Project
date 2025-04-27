#include "utils.hpp"

int main() {
    stringstream data;
    string machine_line = "0x00000008 0xfff00893 addi x17, x0, -1";
    data << machine_line;

    string PC, MC, literal;
    data >> PC >> MC;
    getline(data, literal);
    literal = literal.substr(1, literal.length() - 1);

    cout << "\"" << PC << "\"" << endl;
    cout << "\"" << MC << "\"" << endl;
    cout << "\"" << literal << "\"" << endl;

    uint32_t bit_code = GetDecimalNumber("0xfff00893");
    cout << DecimalToBinary(bit_code) << endl;

    uint8_t opcode, rd, rs1, funct3;
    uint32_t immediate;

    opcode = bit_code & 0b1111111;
    cout << DecimalToBinary(opcode, 7) << endl;
    bit_code = bit_code >> 7;
    rd = bit_code & 0b11111;
    cout << DecimalToBinary(rd, 5) << endl;
    bit_code = bit_code >> 5;
    funct3 = bit_code & 0b111;
    cout << DecimalToBinary(funct3, 3) << endl;
    bit_code = bit_code >> 3;
    rs1 = bit_code & 0b11111;
    cout << DecimalToBinary(rs1, 5) << endl;
    bit_code = bit_code >> 5;
    immediate = bit_code;
    uint32_t result = (int32_t)(immediate << 20) >> 20;
    cout << immediate << " " << DecimalToBinary(immediate) << endl;
    immediate = immediate << 20;
    cout << immediate << " " << DecimalToBinary(immediate) << endl;
    immediate = immediate >> 20;
    cout << immediate << " " << DecimalToBinary(immediate) << endl;
    cout << result << " " << DecimalToBinary(result) << endl;

    switch (2) {
        case 1:
            cout << 1 << endl;
        case 2:
            cout << 2 << endl;
        case 3:
            cout << 3 << endl;
    }
}