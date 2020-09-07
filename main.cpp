#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#include <vector>

using namespace std;

#ifdef _DEBUG // Enable debug features
void debugOutput(string stringToOutput) {
    cout << "DEBUG: " << stringToOutput << endl;
}
#else
void debugOutput(string stringToOutput) {
}
#endif

struct macroDefinition {
    string name;
    string params;
    string code;
};

void sanitizeString(string* line);
void splitLine(string line, string* label, string* opcode, string* params);
void processLine(vector<macroDefinition>* macroDefArray, string line, ifstream* sourceFile, unsigned int* lineNumber);
macroDefinition defineMacro(ifstream* sourceFile, unsigned int* lineNumber, string macroName, string macroParameters);
void expandMacro();

int main(int argc, char *argv[])
{
    vector<macroDefinition> macroDefArray;

    std::filesystem::path sourceFilepath;
    std::filesystem::path destFilepath;

    cout << "SIC/XE Macroassmbler" << endl;

    #ifdef _DEBUG
    cout << "Debug build" << endl;
    #endif

    // TODO: sourceFilepath should never equal destFilepath!!!
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

    debugOutput("The source file is: " + sourceFilepath.string());
    debugOutput("The destination file is: " + destFilepath.string());
    debugOutput("Commencing macroassembly...");

    ifstream sourceFile(sourceFilepath);
    string lineOfCode;

    unsigned int lineNumber = 1;

    while(getline(sourceFile, lineOfCode)) {
        processLine(&macroDefArray, lineOfCode, &sourceFile, &lineNumber);
        lineNumber++;
    }

    return 0;
}

// sanitizeString - Make all characters upper case except for strings and delete comments
string sanitizeString(string line) {
    bool insideString = false;
    for (long unsigned int i = 0; i < line.length(); i++) {
        if (line[i] == '\'') insideString = !insideString;
        // since there can be a ";" symbol inside a SIC/XE string, also check for the comment symbol but only OUTSIDE a SIC/XE string
        // if there is a ";" (comment symbol) outside of a SIC/XE string, delete it and all that comes after it
        if (!insideString && line[i] == ';') {
            line.erase(i, line.length());
            break;
        }
        // else just make the character upper case so we can easily parse the file later on
        else if (!insideString) line[i] = ::toupper(line[i]);
    }
    return line;
}

// splitLine - delete comments and split the string into separate strings - label, opcode, parameters. Returns the values through pointers to strings passed into it.
void splitLine(string line, string* label, string* opcode, string* params) {

    line = sanitizeString(line);

    size_t lineSeek = line.find_first_not_of("\t\n\v\f\r ");
    size_t seekEnd; // the end of the current part of line (label, opcode, params)

    *label = "";
    *opcode = "";
    *params = "";

    if (lineSeek != string::npos) {
        if (lineSeek == 0) {
            seekEnd = line.find_first_of("\t\n\v\f\r ", lineSeek);
            *label = line.substr(lineSeek, seekEnd - lineSeek);
            lineSeek = line.find_first_not_of("\t\n\v\f\r ", seekEnd);
        }

        // the line can have only a label and nothing else, check if it's not the case
        if (lineSeek != string::npos) {
            seekEnd = line.find_first_of("\t\n\v\f\r ", lineSeek);
            *opcode = line.substr(lineSeek, seekEnd - lineSeek);

            lineSeek = line.find_first_not_of("\t\n\v\f\r ", seekEnd);
            // the line can have no params, check if it's not the case
            if (lineSeek != string::npos) {
                *params = line.substr(lineSeek, line.length() - lineSeek);
            }
        }
    }
}

void processLine(vector<macroDefinition>* macroDefArray, string line, ifstream* sourceFile, unsigned int* lineNumber) {
    string label = "";
    string opcode = "";
    string parameters = "";

    splitLine(line, &label, &opcode, &parameters);

    if (label == "" && opcode == "" && parameters == "") {
        debugOutput("Line " + to_string(*lineNumber) + " is empty!");
    }
    else {
        debugOutput("Line " + to_string(*lineNumber) + " has label: " + label + ", opcode: " + opcode + ", params: " + parameters);
    }

    if (opcode == "MACRO") {
        macroDefinition newDefinition = defineMacro(sourceFile, lineNumber, label, parameters);
        macroDefArray->push_back(newDefinition);
        debugOutput("Macro " + newDefinition.name + " defined with parameters " + newDefinition.params + " with the following code:\n" + newDefinition.code);
    }
}

macroDefinition defineMacro(ifstream* sourceFile, unsigned int* lineNumber, string macroName, string macroParameters) {
    macroDefinition newDefinition;
    newDefinition.name = macroName;
    newDefinition.params = macroParameters;

    string line;

    string label = "";
    string opcode = "";
    string params = "";

    // get first line
    getline(*sourceFile, line);
    *lineNumber = *lineNumber + 1;

    splitLine(line, &label, &opcode, &params);

    while (opcode != "MEND") {
        line = sanitizeString(line);
        newDefinition.code += line;

        getline(*sourceFile, line);
        *lineNumber = *lineNumber + 1;

        splitLine(line, &label, &opcode, &params);
        if (opcode != "MEND") newDefinition.code += "\n";
    }

    return newDefinition;
}
