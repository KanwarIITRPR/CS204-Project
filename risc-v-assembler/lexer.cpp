#include "lexer.hpp"

string Lexer::TrimString(string input_string) {
    int start = input_string.find_first_not_of(" \n\r\t");
    if (start == string::npos) return "";
    int end = input_string.find_last_not_of(" \n\r\t");
    return input_string.substr(start, end - start + 1);
}

bool Lexer::IsValidLabel(string label, bool log_error) {
    if (label.find_first_of(", \n\r\t") != string::npos) {
        if (log_error) error_stream << "Invalid label name: " << label << "\n";
        return false;
    } else return true;
}

vector<string> Lexer::Tokenize(string current_line, bool remove_labels) {
    // Removing comments
    size_t comment_start = current_line.find("#");
    if (comment_start != string::npos) current_line = current_line.substr(0, comment_start);
    
    // Trimming the string
    current_line = TrimString(current_line);
    
    // Removing labels
    size_t label_end = current_line.find(":");
    string main_line = current_line, label = "";
    if (label_end != string::npos) {
        label = current_line.substr(0, label_end + 1);
        if (!IsValidLabel(label)) return {};

        main_line = TrimString(current_line.substr(label_end + 1, current_line.length() - label_end - 1));
        if (remove_labels) current_line = main_line;
    }
    
    // Trimming the string
    current_line = TrimString(current_line);
    if (current_line.empty()) return {};
    
    // Checking for no commas between (operation and operands) & (directives and data)
    size_t first_separator_index = main_line.find_first_of(" \n\r\t");
    if (first_separator_index == string::npos) return {current_line};
    string operation = main_line.substr(0, first_separator_index);
    string operands = TrimString(main_line.substr(first_separator_index + 1, main_line.length() - first_separator_index - 1));

    if (operation.find(",") != string::npos) {
        error_stream << "Found ',' directly after operation/directive use: \"" << current_line << "\"\n";
        return {};
    }

    if (operands.find("\"") != string::npos) {
        int quote_count = 0;
        for (char current_char: operands) if (current_char == '"') quote_count++;

        if (quote_count < 2) {
            error_stream << "Incomplete string, missing \": " << current_line << endl;
            return {};
        } else if (quote_count > 2) {
            error_stream << "Only one string is allowed per .asciiz: " << current_line << endl;
            return {};
        } else if (operands.rfind("\"") != operands.length() - 1) {
            error_stream << "Invalid characters at the end of string: " << current_line << endl;
            return {};
        }

        if (remove_labels || label.empty()) return {operation, operands};
        else return {label, operation, operands};
    }

    // Replacing all commas with spaces to unify separator
    replace(operands.begin(), operands.end(), ',', ' ');

    // Separating the modified line with respect to spaces to generate tokens
    vector<string> tokens = {operation};
    if (!remove_labels && !label.empty()) tokens.insert(tokens.begin(), label);

    stringstream argument_stream(operands);
    string current_token;
    while (argument_stream >> current_token) tokens.push_back(current_token);

    return tokens;
}

bool Lexer::GetNextInstruction(string &current_line) {
    return getline(fin, current_line) ? true : false;
}

void Lexer::ResetInputFile() {
    fin.clear();
    fin.seekg(0);
}