#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
using namespace std;

extern ifstream fin;
extern ofstream fout;

extern string instruction;
extern vector<string> tokens;
extern string machineCode;
extern vector<string> machineCodeDivision;

extern int current_address;
extern map<string, int> label_address;

vector<string> Tokenize(bool skip_label);

void ExtractLabelAddresses();

void ProcessCode();

void ProcessData();
