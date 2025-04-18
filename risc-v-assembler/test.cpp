#include "utils.hpp"

int main() {
    stringstream data;
    string machine_line = "0x00000000 0x100005b7 lui x11, 0x10000";
    data << machine_line;

    string PC, MC, literal;
    data >> PC >> MC;
    getline(data, literal);
    literal = literal.substr(1, literal.length() - 1);

    cout << "\"" << PC << "\"" << endl;
    cout << "\"" << MC << "\"" << endl;
    cout << "\"" << literal << "\"" << endl;

}