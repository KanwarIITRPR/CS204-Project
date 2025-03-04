// #include <iostream>
// #include <fstream>
// #include <sstream>
// #include <vector>
// #include <unordered_map>
// using namespace std;

// // Structure to store instruction details
// struct Instruction {
//     string opcode;
//     string format;
//     string rd, rs1, rs2;
//     string imm;
// };

// // RISC-V opcode mapping (for format identification)
// unordered_map<string, string> instructionFormat = {
//     {"add", "R"}, {"sub", "R"}, {"and", "R"}, {"or", "R"}, {"xor", "R"}, 
//     {"sll", "R"}, {"srl", "R"}, {"sra", "R"}, {"slt", "R"}, {"mul", "R"}, 
//     {"div", "R"}, {"rem", "R"},

//     {"addi", "I"}, {"andi", "I"}, {"ori", "I"}, {"lb", "I"}, {"ld", "I"},
//     {"lh", "I"}, {"lw", "I"}, {"jalr", "I"},

//     {"sb", "S"}, {"sw", "S"}, {"sd", "S"}, {"sh", "S"},

//     {"beq", "SB"}, {"bne", "SB"}, {"bge", "SB"}, {"blt", "SB"},

//     {"auipc", "U"}, {"lui", "U"},

//     {"jal", "UJ"}
// };

// // Function to parse the .asm file
// vector<Instruction> parseAsm(const string& filename) {
//     ifstream file(filename);
//     vector<Instruction> instructions;
//     string line;

//     if (!file.is_open()) {
//         cerr << "Error: Cannot open file " << filename << endl;
//         return instructions;
//     }

//     while (getline(file, line)) {
//         size_t commentPos = line.find('#');
//         if (commentPos != string::npos) {
//             line = line.substr(0, commentPos);
//         }
//         line.erase(0, line.find_first_not_of(" \t"));
//         line.erase(line.find_last_not_of(" \t") + 1);

//         if (line.empty()) continue;

//         istringstream ss(line);
//         Instruction instr;
//         ss >> instr.opcode;
//         instr.format = instructionFormat[instr.opcode];

//         if (instr.format == "R") {
//             ss >> instr.rd;  ss.ignore(1, ',');
//             ss >> instr.rs1; ss.ignore(1, ',');
//             ss >> instr.rs2;
//         } 
//         else if (instr.format == "I") {
//             ss >> instr.rd;  ss.ignore(1, ',');
//             ss >> instr.rs1; ss.ignore(1, ',');
//             ss >> instr.imm;
//         } 
//         else if (instr.format == "S") {
//             ss >> instr.rs2; ss.ignore(1, ',');
//             string offsetReg;
//             ss >> offsetReg;
//             size_t openParen = offsetReg.find('(');
//             size_t closeParen = offsetReg.find(')');
//             if (openParen != string::npos && closeParen != string::npos) {
//                 instr.imm = offsetReg.substr(0, openParen);
//                 instr.rs1 = offsetReg.substr(openParen + 1, closeParen - openParen - 1);
//             }
//         } 
//         else if (instr.format == "SB") {
//             ss >> instr.rs1; ss.ignore(1, ',');
//             ss >> instr.rs2; ss.ignore(1, ',');
//             ss >> instr.imm;
//         } 
//         else if (instr.format == "U" || instr.format == "UJ") {
//             ss >> instr.rd;  ss.ignore(1, ',');
//             ss >> instr.imm;
//         }

//         if (!instr.rd.empty() && instr.rd[0] == 'x') instr.rd = instr.rd.substr(1);
//         if (!instr.rs1.empty() && instr.rs1[0] == 'x') instr.rs1 = instr.rs1.substr(1);
//         if (!instr.rs2.empty() && instr.rs2[0] == 'x') instr.rs2 = instr.rs2.substr(1);

//         instructions.push_back(instr);
//     }

//     file.close();
//     return instructions;
// }

// // Function to extract bits based on instruction format
// void extractBits(const vector<Instruction>& instructions) {
//     cout << "Extracted Bit Fields:\n";
//     for (const auto& instr : instructions) {
//         cout << "Instruction: " << instr.opcode << " (" << instr.format << ") ";

//         try {
//             if (instr.format == "R") {
//                 cout << " | Funct7: --- | Rs2: " << instr.rs2 
//                      << " | Rs1: " << instr.rs1 << " | Funct3: --- | Rd: "  
//                      << instr.rd << " | Opcode: ---" << endl;
//             } 
//             else if (instr.format == "I") {
//                 cout << " | Immediate[11:0]: " << instr.imm << " | Rs1: " << instr.rs1 
//                      << " | Funct3: --- | Rd: " << instr.rd << " | Opcode: ---" << endl;
//             } 
//             else if (instr.format == "S") {
//                 cout << " | Imm[11:5]: --- | Rs2: " << instr.rs2 << " | Rs1: " << instr.rs1 
//                      << " | Funct3: --- | Imm[4:0]: --- | Opcode: ---" << endl;
//             } 
//             else if (instr.format == "SB") {
//                 cout << " | Imm[12,10:5]: --- | Rs2: " << instr.rs2 << " | Rs1: " << instr.rs1 
//                      << " | Funct3: --- | Imm[4:1,11]: --- | Opcode: ---" << endl;
//             }
//             else if (instr.format == "U" || instr.format == "UJ") {
//                 cout << " | Rd: " << instr.rd << " | Immediate: " << instr.imm 
//                      << " | Opcode: ---" << endl;
//             }
//         } catch (const exception& e) {
//             cerr << "Error processing instruction '" << instr.opcode << "': " << e.what() << endl;
//         }
//     }
// }
