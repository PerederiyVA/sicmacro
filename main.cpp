#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>

using namespace std;

#ifdef _DEBUG // Enable debug features
void debugOutput(string stringToOutput) {
    cout << "DEBUG: " << stringToOutput << endl;
}
#else
void debugOutput(string stringToOutput) {
}
#endif



//const char *OPCODES[4] = { "START", "", "", "" }

void processLine(string line, unsigned int lineNumber);
void defineMacro();
void expandMacro();

int main(int argc, char *argv[])
{
    std::filesystem::path sourceFilepath;
    std::filesystem::path destFilepath;

    cout << "SIC/XE Macroassmbler" << endl;

    #ifdef _DEBUG
    cout << "Debug build" << endl;
    #endif

    if (argc < 2) {
        cout << "Please provide path to source file!";
        return 1;
    }

    if (argc == 2) {
        sourceFilepath = argv[1];
        destFilepath = sourceFilepath;
        destFilepath.replace_extension(".asm");
        cout << "No destination file specified, will output to: " << destFilepath << endl;
    }
    else {
        sourceFilepath = argv[1];
        destFilepath = argv[2];
    }

    debugOutput("The source file is: " + string(sourceFilepath));
    debugOutput("The destination file is: " + string(destFilepath));

    debugOutput("Commencing macroassembly...");
    ifstream sourceFile(sourceFilepath);
    string lineOfCode;
    unsigned int lineNumber = 1;
    while(getline(sourceFile, lineOfCode)) {
        processLine(lineOfCode, lineNumber);
        lineNumber++;
    }

    return 0;
}

void processLine (string line, unsigned int lineNumber) {
    // Make all characters upper case except for strings
    bool insideString = false;
    for (long unsigned int i = 0; i < line.length(); i++) {
        if (line[i] == '\'') insideString = !insideString;
        if (!insideString) line[i] = ::toupper(line[i]);
    }

    // Delete comments in the line
    size_t commentCharNum = line.find_first_of(";");
    if (commentCharNum != string::npos) {
        line.erase(commentCharNum, line.length());
    }

    size_t lineSeek = line.find_first_not_of("\t\n\v\f\r ");
    size_t seekEnd; // the end of the current part of line (label, opcode, params)
    if (lineSeek != string::npos) {
        string label = "";
        string opcode = "";
        string parameters = "";
        if (lineSeek == 0) {
            seekEnd = line.find_first_of("\t\n\v\f\r ", lineSeek);
            label = line.substr(lineSeek, seekEnd - lineSeek);
            lineSeek = line.find_first_not_of("\t\n\v\f\r ", seekEnd);
        }

        // the line can have only a label and nothing else, check if it's not the case
        if (lineSeek != string::npos) {
            seekEnd = line.find_first_of("\t\n\v\f\r ", lineSeek);
            opcode = line.substr(lineSeek, seekEnd - lineSeek);

            lineSeek = line.find_first_not_of("\t\n\v\f\r ", seekEnd);
            // the line can have no params, check if it's not the case
            if (lineSeek != string::npos) {
                parameters = line.substr(lineSeek, line.length() - lineSeek);
            }
        }


        debugOutput("Line " + to_string(lineNumber) + " has label: " + label + ", opcode: " + opcode + ", params: " + parameters);
        // Check if the line has a valid OPCODE
        // TODO: Proper OPCODE const array
        /*
        if (line.find("BYTE") != string::npos) {
            debugOutput("Valid SIC/XE OPCODE found at line " + to_string(lineNumber));
        }
        else if (line.find("MACRO") != string::npos) {
            cout << "Macro definition found at line " << lineNumber << endl;
        }
        */
    }
    else {
        debugOutput("Line " + to_string(lineNumber) + " is empty!");
    }

}
