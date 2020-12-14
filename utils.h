#include <string>
#include <iostream>

using namespace std;

string SICXE_COMMANDS[] = { "ADD", "ADDF", "ADDR", "AND", "CLEAR", "COMP", "COMPF", "DIV", "DIVF", "DIVR", "FIX", "FLOAT", "HIO", "J", "JEQ", "JGT", "JLT", "JSUB", "LDA", "LDB", "LDCH", "LDF", "LDL", "LDS", "LDT", "LDX", "LPS", "MUL", "MULF", "MULR", "NORM", "OR", "RD", "RMO", "RSUB", "SIO", "SSK", "STA", "STB", "STCH", "STF", "STI", "STL", "STS", "STSW", "STT", "STX", "SUB", "SUBF", "SUBR", "SVC", "TD", "TIO", "TIX", "TIXR", "WD" };
string SICXE_DIRECTIVES[] = { "BYTE", "EQU", "WORD", "RESW", "START", "END" };

bool isCommand(string opcode) {
    if (opcode.length() > 0) {
        if (opcode[0] == '+') {
            opcode.erase(0, 1);
        }
        for (int i = 0; i < 56; i++) {
            if (opcode == SICXE_COMMANDS[i]) return true;
        }
        for (int i = 0; i < 6; i++) {
            if (opcode == SICXE_DIRECTIVES[i]) return true;
        }
    }
    return false;
}
