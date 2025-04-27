#include "utils.hpp"

string error_file = "./logs/errors.txt";
string stats_file = "./logs/stats.txt";
string std_input_file = "./test/input.asm";
string std_output_file = "./test/output.mc";
ofstream error_stream, stats_stream;

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

long long GetDecimalNumber(string s) {
    if (s.empty()) {
        error_stream << "Invalid Number (empty string)" << endl;
        return -1;
    }

    try {
        if (s.substr(0, 2) == "0x") return stoll(s, nullptr, 16);
        if (s.substr(0, 2) == "0b") return stoll(s, nullptr, 2);
        
        size_t start = 0;
        if (s[0] == '-' || s[0] == '+') start = 1;
        for (size_t i = start; i < s.size(); ++i) {
            if (!isdigit(s[i])) {
                error_stream << "Invalid Number (non-digit character): " << s << endl;
                return -1;
            }
        }
        
        return stoll(s);
    } catch (const invalid_argument&) {
        error_stream << "Invalid Number (malformed string): " << s << endl;
        return -1;
    } catch (const out_of_range&) {
        error_stream << "Invalid Number (value out of range): " << s << endl;
        return -1;
    }
}

string DecimalToBinary(int32_t decimal, int bits) {
    if (decimal >= 0) return bitset<32>(decimal).to_string().substr(32 - bits);
    else return bitset<32>(static_cast<uint32_t>(decimal)).to_string().substr(32 - bits);
}

int BinaryToDecimal(const string& binary) {
    if (binary.empty()) return 0;
    
    int significantBits = binary.length();
    int result = (binary[0] == '1') ? -pow(2, significantBits - 1) : 0;
    
    for (int i = 1; i < significantBits; i++) {
        if (binary[i] == '1') result += pow(2, significantBits - 1 - i);
    }
    
    return result;
}

string extendBits(const string& binary, int targetBits) {
    if (binary.length() >= targetBits) return binary;
    char signBit = binary[0];
    
    string extension(targetBits - binary.length(), signBit);
    return extension + binary;
}

uint32_t arithmeticRightShift(uint32_t x, uint32_t n) {
    uint32_t signBit = x & 0x80000000;
    uint32_t mask = (-(signBit >> 31)) & (~(0xFFFFFFFF >> n));
    return (x >> n) | mask;
}

void InitializeFileStreams() {
    error_stream.open(error_file);
    if (!error_stream.is_open()) {
        cout << "Couldn't open error logging file\n";
        return;
    }
    
    stats_stream.open(stats_file);
    if (!stats_stream.is_open()) {
        cout << "Couldn't open stats file\n";
        return;
    }
}

void CloseFileStreams() {
    error_stream.close();
    stats_stream.close();
}