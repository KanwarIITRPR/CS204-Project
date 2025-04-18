#include "assembler.hpp"

bool Assembler::IsValidRegister(string input_register, bool log_error) {
    if (register_ID.find(input_register) == register_ID.end()) {
        if (log_error) error_stream << "Invalid register name: " << input_register << endl;
        return false;
    } else return true;
}

bool Assembler::IsValidImmediate(string input_immediate, Format format, bool log_error) {
    uint8_t max_bits = immediate_bits.at(format);
    int32_t higher = pow(2, max_bits - 1) - 1;
    int32_t lower = -pow(2, max_bits - 1);

    int32_t immediate = GetDecimalNumber(input_immediate);
    if (!(lower <= immediate && immediate <= higher)) {
        if (log_error) error_stream << "Immediate value out of range: " << immediate << " (should be between " << lower << " and " << higher << ")" << endl;
        return false;
    } else return true;
}

bool Assembler::IsValidData(string input_data, string directive, bool log_error) {
    uint8_t max_bits = directive_size.at(directive) * 8;
    int32_t higher = pow(2, max_bits - 1) - 1;
    int32_t lower = -pow(2, max_bits - 1);

    int32_t data = GetDecimalNumber(input_data);
    if (!(lower <= data && data <= higher)) {
        if (log_error) error_stream << "Data value out of range: " << data << " (should be between " << lower << " and " << higher << ")" << endl;
        return false;
    } else return true;
}

vector<string> Assembler::TransformOffsetRegisterFormat(vector<string> tokens, bool log_error) {
    string coupled = tokens[2];
    size_t parenthesis_start = coupled.find_last_of("(");
    size_t parenthesis_end = coupled.find_first_of(")");

    if (parenthesis_start == string::npos || parenthesis_end == string::npos) {
        if (log_error) error_stream << "Too less parentheses: " << coupled << endl;
        return {};
    }
    
    if (parenthesis_start != coupled.find_first_of("(") || parenthesis_end != coupled.find_last_of(")")) {
        if (log_error) error_stream << "Too many parentheses: " << coupled << endl;
        return {};
    }
    
    if (parenthesis_end < parenthesis_start) {
        if (log_error) error_stream << "Inappropriate ordering of parentheses: " << coupled << endl;
        return {};
    }
    
    vector<string> new_tokens(tokens);
    string decoupled_register = coupled.substr(parenthesis_start + 1, parenthesis_end - parenthesis_start - 1);
    string decoupled_immediate = coupled.substr(0, parenthesis_start);

    new_tokens.push_back(decoupled_immediate);
    new_tokens[2] = decoupled_register;
    return new_tokens;
}

vector<string> Assembler::CorrectLabels(vector<string> tokens, uint32_t current_address, bool log_error) {
    string label = tokens[tokens.size() - 1];
    if (label_address.find(label) == label_address.end()) {
        if (log_error) error_stream << "Invalid label name: " << label << endl;
        return {};
    }

    tokens[tokens.size() - 1] = to_string((int32_t) (label_address[label] - current_address));
    return tokens;
}

bool Assembler::HasCorrectOperandsFormat(vector<string> tokens) {
    string operation = tokens[0];
    Format operation_format = GetFormat(operation);

    if (tokens.size() != argument_size.at(operation_format)) {
        error_stream << "Invalid number of arguments: " << tokens.size() - 1 << ", expected " << argument_size.at(operation_format) - 1 << endl;\
        return false;
    }

    switch (operation_format) {
        case Format::R:
            if (!(IsValidRegister(tokens[1], true) && IsValidRegister(tokens[2], true) && IsValidRegister(tokens[3], true))) return false;
            return true;
        case Format::I:
            if (!(IsValidRegister(tokens[1], true) && IsValidRegister(tokens[2], true) && IsValidImmediate(tokens[3], Format::I, true))) return false;
            return true;
        case Format::S:
            if (!(IsValidRegister(tokens[1], true) && IsValidRegister(tokens[2], true) && IsValidImmediate(tokens[3], Format::S, true))) return false;
            return true;
        case Format::SB:
            if (!(IsValidRegister(tokens[1], true) && IsValidRegister(tokens[2], true) && IsValidImmediate(tokens[3], Format::SB, true))) return false;
            return true;
        case Format::U:
            if (!(IsValidRegister(tokens[1], true) && IsValidImmediate(tokens[2], Format::U, true))) return false;
            return true;
        case Format::UJ:
            if (!(IsValidRegister(tokens[1], true) && IsValidImmediate(tokens[2], Format::UJ, true))) return false;
            return true;
        default:
            return false;
    }
}

void Assembler::InitialParse() {
    uint32_t current_address = TEXT_ADDRESS;
    bool text_mode = true;

    string current_instruction;
    vector<string> tokens;
    while (lexer.GetNextInstruction(current_instruction)) {
        tokens = lexer.Tokenize(current_instruction, false);
        if (tokens.empty()) continue;
        
        if (tokens[0] == ".data")  {
            current_address = DATA_ADDRESS;
            text_mode = false;
            continue;            
        } else if (tokens[0] == ".text") {
            current_address = TEXT_ADDRESS;
            text_mode = true;
            continue;
        }
        
        if (tokens[0].find(":") != string::npos) {
            int label_end = tokens[0].find(":");
            string label = tokens[0].substr(0, label_end);
            label_address[label] = current_address;
            
            tokens.erase(tokens.begin());
            if (tokens.empty()) continue;
        }

        if (IsValidDirective(tokens[0])) {
            if (text_mode) error_stream << "Data directives should be placed in \'.data\': " << current_instruction << endl;
            current_address += directive_size.at(tokens[0]) * (tokens.size() - 1);
        } else if (IsValidOperation(tokens[0])) {
            if (!text_mode) error_stream << "Operations should be placed in \'.text\': " << current_instruction << endl;
            current_address += INSTRUCTION_SIZE;
        } else error_stream << "Unrecognized command: " << current_instruction << endl;
    }

    lexer.ResetInputFile();
}

void Assembler::Assemble() {
    InitialParse();
    cout << "Intial Parse Complete" << endl;
    for (auto label : label_address) cout << label.first << " ~ " << label.second << endl;
    cout << endl;
    
    uint32_t current_address = TEXT_ADDRESS;
    bool text_mode = true;
    
    string current_instruction;
    vector<string> tokens;
    while (lexer.GetNextInstruction(current_instruction)) {
        tokens = lexer.Tokenize(current_instruction);
        if (tokens.empty()) continue;
        
        if (tokens[0] == ".data")  {
            current_address = DATA_ADDRESS;
            text_mode = false;
            continue;            
        } else if (tokens[0] == ".text") {
            current_address = TEXT_ADDRESS;
            text_mode = true;
            continue;
        }
        
        if (!text_mode) current_address = ExtractData(tokens, current_address);
        else current_address = ExtractText(tokens, current_address);
    }

    for (auto data: data_map) cout << hex << data.first << " (" << data.second.first << " ~ " << data.second.second << ")" << endl;
    cout << endl << endl;
    for (auto text: text_map) cout << hex << text.first << " (" << text.second.first << " ~ " << text.second.second << ")" << endl;

    ShowOutput();
    cout << "Program Assembled!" << endl;
}

uint32_t Assembler::ExtractData(vector<string> tokens, uint32_t current_address) {
    string data_directive = tokens[0];
    for (size_t i = 1; i < tokens.size(); i++) {
        if (!IsValidData(tokens[i], data_directive, true)) continue;
        data_map[current_address] = {directive_size.at(data_directive), GetDecimalNumber(tokens[i])};
        current_address += directive_size.at(data_directive);
    }
    return current_address;
}

uint32_t Assembler::ExtractText(vector<string> tokens, uint32_t current_address) {
    string operation = tokens[0];
    if (HasLabelOperand(operation)) tokens = CorrectLabels(tokens, current_address);
    else if (HasOffsetRegisterCoupling(operation)) tokens = TransformOffsetRegisterFormat(tokens);
    
    string current_instruction = operation + " ";
    for (size_t i = 1; i < tokens.size(); i++) current_instruction += tokens[i] + ((i == tokens.size() - 1) ? "" : ", ");
    
    if (!HasCorrectOperandsFormat(tokens)) {
        error_stream << "Incorrect format of instruction: " << current_instruction << endl;
        return current_address + INSTRUCTION_SIZE;
    }

    Format operation_format = GetFormat(operation);
    uint32_t machine_code;
    switch (operation_format) {
        case Format::R:
            machine_code = Get_R_Code(tokens);
            break;
        case Format::I:
            machine_code = Get_I_Code(tokens);
            break;
        case Format::S:
            machine_code = Get_S_Code(tokens);
            break;
        case Format::SB:
            machine_code = Get_SB_Code(tokens);
            break;
        case Format::U:
            machine_code = Get_U_Code(tokens);
            break;
        case Format::UJ:
            machine_code = Get_UJ_Code(tokens);
            break;
        default:
            return current_address + INSTRUCTION_SIZE;
    }

    text_map[current_address] = {machine_code, current_instruction};
    return current_address + INSTRUCTION_SIZE;
}

uint32_t Assembler::Get_R_Code(vector<string> tokens) {
    uint8_t operation_code = opcode.at(tokens[0]);
    uint8_t rd = register_ID.at(tokens[1]);
    uint8_t rs1 = register_ID.at(tokens[2]);
    uint8_t rs2 = register_ID.at(tokens[3]);
    uint8_t function_3 = funct3.at(tokens[0]);
    uint8_t function_7 = funct7.at(tokens[0]);

    uint32_t machine_code = function_7;
    machine_code = machine_code << REGISTER_LENGTH;
    machine_code += rs2;
    machine_code = machine_code << REGISTER_LENGTH;
    machine_code += rs1;
    machine_code = machine_code << FUNCT3_LENGTH;
    machine_code += function_3;
    machine_code = machine_code << REGISTER_LENGTH;
    machine_code += rd;
    machine_code = machine_code << OPCODE_LENGTH;
    machine_code += operation_code;
    return machine_code;
}

uint32_t Assembler::Get_I_Code(vector<string> tokens) {
    uint8_t operation_code = opcode.at(tokens[0]);
    uint8_t rd = register_ID.at(tokens[1]);
    uint8_t rs1 = register_ID.at(tokens[2]);
    uint16_t immediate = GetDecimalNumber(tokens[3]);
    uint8_t function_3 = funct3.at(tokens[0]);

    uint32_t machine_code = immediate;
    machine_code = machine_code << REGISTER_LENGTH;
    machine_code += rs1;
    machine_code = machine_code << FUNCT3_LENGTH;
    machine_code += function_3;
    machine_code = machine_code << REGISTER_LENGTH;
    machine_code += rd;
    machine_code = machine_code << OPCODE_LENGTH;
    machine_code += operation_code;
    return machine_code;
}

uint32_t Assembler::Get_S_Code(vector<string> tokens) {
    uint8_t operation_code = opcode.at(tokens[0]);
    uint8_t rs1 = register_ID.at(tokens[2]);
    uint8_t rs2 = register_ID.at(tokens[1]);
    string immediate = DecimalToBinary(GetDecimalNumber(tokens[3]), immediate_bits.at(Format::S));
    uint8_t function_3 = funct3.at(tokens[0]);

    uint8_t upper_immediate = BinaryToDecimal(immediate.substr(0, 7));
    uint8_t lower_immediate = BinaryToDecimal(immediate.substr(7, 5));

    uint32_t machine_code = upper_immediate;
    machine_code = machine_code << REGISTER_LENGTH;
    machine_code += rs2;
    machine_code = machine_code << REGISTER_LENGTH;
    machine_code += rs1;
    machine_code = machine_code << FUNCT3_LENGTH;
    machine_code += function_3;
    machine_code = machine_code << S_LOWER_IMMEDIATE_LENGTH;
    machine_code += lower_immediate;
    machine_code = machine_code << OPCODE_LENGTH;
    machine_code += operation_code;
    return machine_code;
}

uint32_t Assembler::Get_SB_Code(vector<string> tokens) {
    uint8_t operation_code = opcode.at(tokens[0]);
    uint8_t rs1 = register_ID.at(tokens[1]);
    uint8_t rs2 = register_ID.at(tokens[2]);
    string immediate = DecimalToBinary(GetDecimalNumber(tokens[3]) >> 1, immediate_bits.at(Format::SB));
    uint8_t function_3 = funct3.at(tokens[0]);

    uint8_t upper_immediate = BinaryToDecimal(immediate[0] + immediate.substr(2, 6));
    uint8_t lower_immediate = BinaryToDecimal(immediate.substr(8, 4) + immediate[1]);

    uint32_t machine_code = upper_immediate;
    machine_code = machine_code << REGISTER_LENGTH;
    machine_code += rs2;
    machine_code = machine_code << REGISTER_LENGTH;
    machine_code += rs1;
    machine_code = machine_code << FUNCT3_LENGTH;
    machine_code += function_3;
    machine_code = machine_code << SB_LOWER_IMMEDIATE_LENGTH;
    machine_code += lower_immediate;
    machine_code = machine_code << OPCODE_LENGTH;
    machine_code += operation_code;
    return machine_code;
}

uint32_t Assembler::Get_U_Code(vector<string> tokens) {
    uint8_t operation_code = opcode.at(tokens[0]);
    uint8_t rd = register_ID.at(tokens[1]);
    uint32_t immediate = GetDecimalNumber(tokens[2]);

    uint32_t machine_code = immediate;
    machine_code = machine_code << REGISTER_LENGTH;
    machine_code += rd;
    machine_code = machine_code << OPCODE_LENGTH;
    machine_code += operation_code;
    return machine_code;
}

uint32_t Assembler::Get_UJ_Code(vector<string> tokens) {
    uint8_t operation_code = opcode.at(tokens[0]);
    uint8_t rd = register_ID.at(tokens[1]);
    string immediate = DecimalToBinary(GetDecimalNumber(tokens[2]) >> 1, immediate_bits.at(Format::UJ));
    
    uint32_t formatted_immediate = BinaryToDecimal(immediate[0] + immediate.substr(10, 10) + immediate[9] + immediate.substr(1, 8));

    uint32_t machine_code = formatted_immediate;
    machine_code = machine_code << REGISTER_LENGTH;
    machine_code += rd;
    machine_code = machine_code << OPCODE_LENGTH;
    machine_code += operation_code;
    return machine_code;
}

void Assembler::ShowOutput() {
    const int BYTE_SIZE_HEX = 2;

    fout << ".data\n";
    for (auto data: data_map) {
        fout << hex << "0x" << setw(ADDRESS_SIZE) << setfill('0') << data.first << " ";
        fout << hex << "0x" << setw(data.second.first * BYTE_SIZE_HEX) << setfill('0') << data.second.second << "\n";
    }
    fout << DATA_END << endl;

    fout << ".text\n";
    for (auto text: text_map) {
        fout << hex << "0x" << setw(ADDRESS_SIZE) << setfill('0') << text.first << " ";
        fout << hex << "0x" << setw(INSTRUCTION_SIZE * BYTE_SIZE_HEX) << setfill('0') << text.second.first << " " << text.second.second << "\n";
    }
    fout << TEXT_END << endl;

}