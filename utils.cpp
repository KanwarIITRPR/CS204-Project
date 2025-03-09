#include <bits/stdc++.h>
using namespace std;

map<string, char> hex_dict = {
    {"0000", '0'},
    {"0001", '1'},
    {"0010", '2'},
    {"0011", '3'}, 
    {"0100", '4'},
    {"0101", '5'},
    {"0110", '6'},
    {"0111", '7'},
    {"1000", '8'},
    {"1001", '9'},
    {"1010", 'A'},
    {"1011", 'B'},
    {"1100", 'C'},
    {"1101", 'D'},
    {"1110", 'E'},
    {"1111", 'F'}
};

int GetDecimalNumber(const string &s) {
    bool is_hex = (s.substr(0, 2) == "0x");
    if (is_hex) return stoi(s, nullptr, 16);
    for (char digit : s) {
        if (!isdigit(digit)) {
            cerr << "Invalid Number";
            return (int) nan;
        }
    }
    return stoi(s);
}

string DecimalToBinary(int decimalNumber, int length = -1) {
    if (!decimalNumber) return "0";

    string binary = "";
    while (decimalNumber) {
        binary = ((decimalNumber % 2 == 0) ? "0" : "1") + binary;
        decimalNumber /= 2;
    }

    while (binary.size() < length) binary = "0" + binary;
    return binary;
}

string BinaryToHex(string binaryNumber) {
    binaryNumber = string(binaryNumber.length() % 4 ? 4 - binaryNumber.length() % 4 : 0, '0') + binaryNumber;
    string hexadecimal;
    for (size_t i = 0; i < binaryNumber.length(); i += 4) {
        string group = binaryNumber.substr(i, 4);
        hexadecimal += hex_dict[group];
    }
    return hexadecimal;
}
