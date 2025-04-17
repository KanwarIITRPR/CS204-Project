#include "operations.hpp"

bool IsValidOperation(string operation, bool log_error) {
    bool is_valid = opcode.find(operation) != opcode.end();
    if (log_error && !is_valid) error_stream << "Invalid operation: " << operation << "\n";
}

bool IsValidDirective(string directive, bool log_error) {
    bool is_valid = directive_size.find(directive) != directive_size.end();
    if (log_error && !is_valid) error_stream << "Invalid directive: " << directive << "\n";
}

Format GetFormat(string operation) {
    auto opcode_pair = opcode.find(operation);
    switch (opcode_pair -> second) {
        case 0b0110011:
            return Format::R;
        case 0b0010011:
            return Format::I;
        case 0b0000011:
            return Format::I;
        case 0b1100111:
            return Format::I;
        case 0b0100011:
            return Format::S;
        case 0b1100011:
            return Format::SB;
        case 0b0110111:
            return Format::U;
        case 0b0010111:
            return Format::U;
        case 0b1101111:
            return Format::UJ;
        default:
            return Format::INVALID;
    }
};

string GetFormatName(Format format) {
    switch (format) {
        case Format::R:
            return "R-Type";
        case Format::I:
            return "I-Type";
        case Format::S:
            return "S-Type";
        case Format::SB:
            return "SB-Type";
        case Format::U:
            return "U-Type";
        case Format::UJ:
            return "UJ-Type";
        default:
            return "INVALID";
    }
}

bool IsLoadOperation(string operation) {
    // if (!IsValidOperation(operation)) return false;
    return opcode.find(operation) -> second == 0b0000011;
}

bool HasOffsetRegisterCoupling(string operation) {
    switch (opcode.find(operation) -> second) {
        case 0b0000011:
            return true;
        case 0b0100011:
            return true;
        default:
            return false;
    }
}

bool HasLabelOperand(string operation) {
    switch (GetFormat(operation)) {
        case Format::SB:
            return true;
        case Format::UJ:
            return true;
        default:
            return false;
    }
}