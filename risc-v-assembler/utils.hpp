#ifndef UTILITIES_H
#define UTILITIES_H

#include <bits/stdc++.h>
using namespace std;

#define BYTE_SIZE 8

extern string std_input_file, std_output_file, parameters_fle;
extern string error_file, stats_file;
extern string fetch_output, decode_output, execute_output, memory_output, writeback_output;
extern string data_forwarding_output, stalling_output, flushing_output;

extern ofstream error_stream, stats_stream;
extern ifstream parameters_stream;
extern ofstream fetch_stream, decode_stream, execute_stream, memory_stream, writeback_stream;
extern ofstream data_forwarding_stream, stalling_stream, flushing_stream;

extern map<string, char> hex_dict;

long long GetDecimalNumber(string s);
string DecimalToBinary(int32_t decimal, int bits = 32);
int BinaryToDecimal(const string& binary);
string extendBits(const string& binary, int targetBits = 32);

uint32_t arithmeticRightShift(uint32_t x, uint32_t n);

void InitializeFileStreams();
void CloseFileStreams();

#endif