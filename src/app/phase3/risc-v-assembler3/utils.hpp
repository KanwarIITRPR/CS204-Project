#ifndef UTILITIES_H
#define UTILITIES_H

#include <bits/stdc++.h>
using namespace std;

#define BYTE_SIZE 8

extern string error_file;
extern string std_input_file;
extern string std_output_file;
extern ofstream error_stream;

extern map<string, char> hex_dict;

long long GetDecimalNumber(string s);
string DecimalToBinary(int32_t decimal, int bits = 32);
int BinaryToDecimal(const string& binary);
string extendBits(const string& binary, int targetBits = 32);
string BinaryToHex(string binaryNumber);

uint32_t arithmeticRightShift(uint32_t x, uint32_t n);

string trimString(string str);

void InitializeFileStreams();
void CloseFileStreams();

#endif