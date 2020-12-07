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
macroDefinition findMacro(vector<macroDefinition>* macroDefArray, string macroName);
void splitLine(string line, string* label, string* opcode, string* params);
void processLine(vector<macroDefinition>* macroDefArray, string line, ifstream* sourceFile, ofstream* destFile, unsigned int* lineNumber);
macroDefinition defineMacro(ifstream* sourceFile, vector<macroDefinition>* macroDefArray, unsigned int* lineNumber, string macroName, string macroParameters);
void expandInsideDefinition(macroDefinition* macroDef, macroDefinition macroToExpand, string label, string parameters);
void expandMacro(ofstream* destFile, macroDefinition macroToExpand, string label, string parameters);


// Temporary, will replace with Beck's label substitution later
unsigned int labelSubstitutions = 0;
unsigned int defineMacroLabelSubstitutions = 0;

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

    if (sourceFilepath == destFilepath) {
        cout << "ERROR: Destination file cannot be the same as the source file" << endl;
        return 1;
    }
    debugOutput("The source file is: " + sourceFilepath.string());
    debugOutput("The destination file is: " + destFilepath.string());
    debugOutput("Commencing macroassembly...");

    ifstream sourceFile(sourceFilepath);
    ofstream destFile(destFilepath);
    string lineOfCode;

    unsigned int lineNumber = 1;

    while(getline(sourceFile, lineOfCode)) {
        processLine(&macroDefArray, lineOfCode, &sourceFile, &destFile, &lineNumber);
        lineNumber++;
    }

    sourceFile.close();

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

macroDefinition findMacro(vector<macroDefinition>* macroDefArray, string macroName) {
    for (unsigned int i = 0; i < macroDefArray->size(); i++) {
        if (macroDefArray->at(i).name == macroName)
            return macroDefArray->at(i);
    }
    macroDefinition nullMacro; nullMacro.name = ""; nullMacro.code = ""; nullMacro.params = "";
    return nullMacro;
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

void processLine(vector<macroDefinition>* macroDefArray, string line, ifstream* sourceFile, ofstream* destFile, unsigned int* lineNumber) {
    string label = "";
    string opcode = "";
    string parameters = "";

    splitLine(line, &label, &opcode, &parameters);

    if (label == "" && opcode == "" && parameters == "") {
        //debugOutput("Line " + to_string(*lineNumber) + " is empty!");
        return;
    }
    else {
        //debugOutput("Line " + to_string(*lineNumber) + " has label: " + label + ", opcode: " + opcode + ", params: " + parameters);
    }

    // Searches for a macro definition of the same name in the vector
    // If not found, will return a null macro with all empty fields
    macroDefinition foundMacro = findMacro(macroDefArray, opcode);
    if (opcode == "MACRO") {
        macroDefinition newDefinition = defineMacro(sourceFile, macroDefArray, lineNumber, label, parameters);
        macroDefArray->push_back(newDefinition);
        if (newDefinition.params != "")
            debugOutput("Line " + to_string(*lineNumber) + ": Macro " + newDefinition.name + " defined with parameters " + newDefinition.params + ", with the following code:\n" + newDefinition.code);
        else
            debugOutput("Line " + to_string(*lineNumber) + ": Macro " + newDefinition.name + " defined without parameters, with the following code:\n" + newDefinition.code);
    }
    else if (foundMacro.name == opcode) {
        debugOutput("Line " + to_string(*lineNumber) + ": Found a " + opcode + " macro call, expanding...");
        expandMacro(destFile, foundMacro, label, parameters);
    }
    else {
        *destFile << label << "\t" << opcode << "\t" << parameters << endl;
    }
}

macroDefinition defineMacro(ifstream* sourceFile, vector<macroDefinition>* macroDefArray, unsigned int* lineNumber, string macroName, string macroParameters) {
    macroDefinition newDefinition;
    newDefinition.name = macroName;
    newDefinition.params = macroParameters;

    string line;

    string label = "";
    string opcode = "";
    string params = "";

    // get first line
    bool isEOF = false;
    if (!getline(*sourceFile, line))
        isEOF = true;
    *lineNumber = *lineNumber + 1;

    splitLine(line, &label, &opcode, &params);

    while (opcode != "MEND" && !isEOF) {
        line = sanitizeString(line);
        bool lineIsNotEmpty = (label != "" || opcode != "" || params != "");
        bool macroExpanded = false;
        //debugOutput(label + " " + opcode + " " + params);
        if (lineIsNotEmpty) {
            macroDefinition foundMacro = findMacro(macroDefArray, opcode);
            if (foundMacro.name == opcode) {
                expandInsideDefinition(&newDefinition, foundMacro, label, params);
                macroExpanded = true;
            }
            else {
                newDefinition.code += line;
            }
        }
            

        if (!getline(*sourceFile, line)) isEOF = true;
        *lineNumber = *lineNumber + 1;

        splitLine(line, &label, &opcode, &params);
        if (opcode != "MEND" && lineIsNotEmpty && !macroExpanded) newDefinition.code += "\n";
    }

    return newDefinition;
}


void expandInsideDefinition(macroDefinition* macroDef, macroDefinition macroToExpand, string label, string parameters) {
    struct substitution {
        string match;
        string replacement;
    };

    struct lineOfCode {
        string label;
        string opcode;
        vector<string> parameters;
    };

    // Step 1, turn the code of the macro into a vector of lines
    vector<lineOfCode> codeToExpand;
    size_t lineSeek = 0;
    size_t seekEnd;

    // A paranoid check if the macro has any code
    if (macroToExpand.code != "") {
        // Process each line, putting it into a vector of code lines (makes it easier to substitute labels later)
        while (seekEnd != string::npos) {
            seekEnd = macroToExpand.code.find_first_of("\n", lineSeek);
            string currentLine = macroToExpand.code.substr(lineSeek, seekEnd - lineSeek);

            string currentLineLabel, currentLineOpcode, currentLineParameters;
            splitLine(currentLine, &currentLineLabel, &currentLineOpcode, &currentLineParameters);

            lineOfCode currentLineOfCode;
            currentLineOfCode.label = currentLineLabel;
            currentLineOfCode.opcode = currentLineOpcode;

            vector<string> tempParameterVector;
            if (currentLineParameters != "") {
                // Remove all spaces in the parameters strings
                currentLineParameters.erase(remove_if(currentLineParameters.begin(), currentLineParameters.end(), ::isspace), currentLineParameters.end());
                string delimiter = ",";
                size_t paramLineSeek;
                size_t paramSeekEnd = -1;
                do {
                    paramLineSeek = paramSeekEnd + 1;
                    paramSeekEnd = currentLineParameters.find(delimiter, paramLineSeek);
                    string tempParameter = currentLineParameters.substr(paramLineSeek, paramSeekEnd - paramLineSeek);
                    tempParameterVector.push_back(tempParameter);
                } while (paramSeekEnd != string::npos);
            }
            lineSeek = seekEnd + 1;
            currentLineOfCode.parameters = tempParameterVector;
            codeToExpand.push_back(currentLineOfCode);
        }
    }


    // Step 2, take macro parameters and substitute them
    //
    // Remove all spaces in the parameters strings
    macroToExpand.params.erase(remove_if(macroToExpand.params.begin(), macroToExpand.params.end(), ::isspace), macroToExpand.params.end());
    parameters.erase(remove_if(parameters.begin(), parameters.end(), ::isspace), parameters.end());

    // Create a vector that stores substitutions
    // Parameters to replace (match) and what to replace them with (replacement)
    vector<substitution> substitutionVector;

    // If the macro has any parameters, add them to the substitutionVector
    if (macroToExpand.params != "") {
        // Parameters are separated by commas, spaces were trimmed
        string delimiter = ",";
        size_t lineSeek;
        size_t seekEnd = -1;
        // Parse the parameters of the macro itself
        do {
            lineSeek = seekEnd + 1;
            seekEnd = macroToExpand.params.find(delimiter, lineSeek);
            string macroParameter = macroToExpand.params.substr(lineSeek, seekEnd - lineSeek);
            substitution tempSubstitution;
            tempSubstitution.match = macroParameter; tempSubstitution.replacement = "";
            substitutionVector.push_back(tempSubstitution);
        } while (seekEnd != string::npos);

        // Parse the passed parameters
        // TODO: Error if less parameters were passed than intended by the macro definition.
        seekEnd = -1;
        unsigned int i = -1;
        do {
            i++;
            lineSeek = seekEnd + 1;
            seekEnd = parameters.find(delimiter, lineSeek);
            string parameter = parameters.substr(lineSeek, seekEnd - lineSeek);
            substitutionVector[i].replacement = parameter;
        } while (seekEnd != string::npos && i < substitutionVector.size());

        debugOutput("Substitutions:");
        for (unsigned int i = 0; i < substitutionVector.size(); i++) {
            debugOutput(substitutionVector.at(i).match + " will be replaced by " + substitutionVector.at(i).replacement);
        }

        // Go through the parameters in the code and substitute them
        for (unsigned int i = 0; i < codeToExpand.size(); i++) {
            for (unsigned int j = 0; j < codeToExpand.at(i).parameters.size(); j++) {
                // Prefixes would make it difficult to compare strings so we'll store the prefix in a string
                // and remove it from the parameter string, adding it back later
                string prefix = "";
                if (codeToExpand.at(i).parameters.at(j)[0] == '#' || codeToExpand.at(i).parameters.at(j)[0] == '@') {
                        prefix = codeToExpand.at(i).parameters.at(j)[0];
                        codeToExpand.at(i).parameters.at(j).erase(0, 1);
                }
                // Search the substitution vector for matches and replace with the corresponding replacement parameter, adding the prefix back in
                for (unsigned int k = 0; k < substitutionVector.size(); k++) {
                    if (codeToExpand.at(i).parameters.at(j) == substitutionVector.at(k).match) {
                        codeToExpand.at(i).parameters.at(j) = (prefix + substitutionVector.at(k).replacement);
                    }
                }
            }
        }
    }

    // Substitute labels to avoid label conflicts when expanding macro two times or more
    for (unsigned int i = 0; i < codeToExpand.size(); i++) {
        // If label has '$' prefix, it is a local label inside the macro and should be replaced
        if (codeToExpand.at(i).label[0] == '$') {
            string oldLabel = codeToExpand.at(i).label;
            string newLabel = "$tm" + to_string(defineMacroLabelSubstitutions++);
            for (unsigned int j = 0; j < codeToExpand.size(); j++) {
                for (unsigned int k = 0; k < codeToExpand.at(j).parameters.size(); k++) {
                    // Check if there's a prefix
                    if (codeToExpand.at(j).parameters.at(k)[0] == '#' || codeToExpand.at(j).parameters.at(k)[0] == '@') {
                        string prefix;
                        prefix = codeToExpand.at(j).parameters.at(k)[0];
                        if (codeToExpand.at(j).parameters.at(k).substr(1,string::npos) == oldLabel)
                            codeToExpand.at(j).parameters.at(k) = (prefix + newLabel);
                    }
                    else {
                        if (codeToExpand.at(j).parameters.at(k) == oldLabel)
                            codeToExpand.at(j).parameters.at(k) = newLabel;
                    }
                }
            }
            codeToExpand.at(i).label = newLabel;
        }
    }

    // Output the code to destination file
    //
    // If the macro call string has a label, it should be preserved
    if (label != "")
        macroDef->code += (label + "\n");

    // Output the code line vector
    for (unsigned int i = 0; i < codeToExpand.size(); i++) {
        macroDef->code += (codeToExpand.at(i).label + "\t" + codeToExpand.at(i).opcode);
        if (codeToExpand.at(i).parameters.size() > 0) {
            macroDef->code += ("\t" + codeToExpand.at(i).parameters.at(0));
            for (unsigned int j = 1; j < codeToExpand.at(i).parameters.size(); j++) {
                macroDef->code += ("," + codeToExpand.at(i).parameters.at(j));
            }
        }
        macroDef->code += "\n";
    }
}



void expandMacro(ofstream* destFile, macroDefinition macroToExpand, string label, string parameters) {
    struct substitution {
        string match;
        string replacement;
    };

    struct lineOfCode {
        string label;
        string opcode;
        vector<string> parameters;
    };

    // Step 1, turn the code of the macro into a vector of lines
    vector<lineOfCode> codeToExpand;
    size_t lineSeek = 0;
    size_t seekEnd;

    // A paranoid check if the macro has any code
    if (macroToExpand.code != "") {
        // Process each line, putting it into a vector of code lines (makes it easier to substitute labels later)
        while (seekEnd != string::npos) {
            seekEnd = macroToExpand.code.find_first_of("\n", lineSeek);
            string currentLine = macroToExpand.code.substr(lineSeek, seekEnd - lineSeek);

            string currentLineLabel, currentLineOpcode, currentLineParameters;
            splitLine(currentLine, &currentLineLabel, &currentLineOpcode, &currentLineParameters);

            lineOfCode currentLineOfCode;
            currentLineOfCode.label = currentLineLabel;
            currentLineOfCode.opcode = currentLineOpcode;

            vector<string> tempParameterVector;
            if (currentLineParameters != "") {
                // Remove all spaces in the parameters strings
                currentLineParameters.erase(remove_if(currentLineParameters.begin(), currentLineParameters.end(), ::isspace), currentLineParameters.end());
                string delimiter = ",";
                size_t paramLineSeek;
                size_t paramSeekEnd = -1;
                do {
                    paramLineSeek = paramSeekEnd + 1;
                    paramSeekEnd = currentLineParameters.find(delimiter, paramLineSeek);
                    string tempParameter = currentLineParameters.substr(paramLineSeek, paramSeekEnd - paramLineSeek);
                    tempParameterVector.push_back(tempParameter);
                } while (paramSeekEnd != string::npos);
            }
            lineSeek = seekEnd + 1;
            currentLineOfCode.parameters = tempParameterVector;
            codeToExpand.push_back(currentLineOfCode);
        }
    }


    // Step 2, take macro parameters and substitute them
    //
    // Remove all spaces in the parameters strings
    macroToExpand.params.erase(remove_if(macroToExpand.params.begin(), macroToExpand.params.end(), ::isspace), macroToExpand.params.end());
    parameters.erase(remove_if(parameters.begin(), parameters.end(), ::isspace), parameters.end());

    // Create a vector that stores substitutions
    // Parameters to replace (match) and what to replace them with (replacement)
    vector<substitution> substitutionVector;

    // If the macro has any parameters, add them to the substitutionVector
    if (macroToExpand.params != "") {
        // Parameters are separated by commas, spaces were trimmed
        string delimiter = ",";
        size_t lineSeek;
        size_t seekEnd = -1;
        // Parse the parameters of the macro itself
        do {
            lineSeek = seekEnd + 1;
            seekEnd = macroToExpand.params.find(delimiter, lineSeek);
            string macroParameter = macroToExpand.params.substr(lineSeek, seekEnd - lineSeek);
            substitution tempSubstitution;
            tempSubstitution.match = macroParameter; tempSubstitution.replacement = "";
            substitutionVector.push_back(tempSubstitution);
        } while (seekEnd != string::npos);

        // Parse the passed parameters
        // TODO: Error if less parameters were passed than intended by the macro definition.
        seekEnd = -1;
        unsigned int i = -1;
        do {
            i++;
            lineSeek = seekEnd + 1;
            seekEnd = parameters.find(delimiter, lineSeek);
            string parameter = parameters.substr(lineSeek, seekEnd - lineSeek);
            substitutionVector[i].replacement = parameter;
        } while (seekEnd != string::npos && i < substitutionVector.size());

        debugOutput("Substitutions:");
        for (unsigned int i = 0; i < substitutionVector.size(); i++) {
            debugOutput(substitutionVector.at(i).match + " will be replaced by " + substitutionVector.at(i).replacement);
        }

        // Go through the parameters in the code and substitute them
        for (unsigned int i = 0; i < codeToExpand.size(); i++) {
            for (unsigned int j = 0; j < codeToExpand.at(i).parameters.size(); j++) {
                // Prefixes would make it difficult to compare strings so we'll store the prefix in a string
                // and remove it from the parameter string, adding it back later
                string prefix = "";
                if (codeToExpand.at(i).parameters.at(j)[0] == '#' || codeToExpand.at(i).parameters.at(j)[0] == '@') {
                        prefix = codeToExpand.at(i).parameters.at(j)[0];
                        codeToExpand.at(i).parameters.at(j).erase(0, 1);
                }
                // Search the substitution vector for matches and replace with the corresponding replacement parameter, adding the prefix back in
                for (unsigned int k = 0; k < substitutionVector.size(); k++) {
                    if (codeToExpand.at(i).parameters.at(j) == substitutionVector.at(k).match) {
                        //codeToExpand.at(i).parameters.at(j) = (prefix + substitutionVector.at(k).replacement);
                        codeToExpand.at(i).parameters.at(j) = substitutionVector.at(k).replacement;
                    }
                }
                // bugfix?
                codeToExpand.at(i).parameters.at(j) = prefix + codeToExpand.at(i).parameters.at(j);
            }
        }
    }

    // Substitute labels to avoid label conflicts when expanding macro two times or more
    for (unsigned int i = 0; i < codeToExpand.size(); i++) {
        // If label has '$' prefix, it is a local label inside the macro and should be replaced
        if (codeToExpand.at(i).label[0] == '$') {
            string oldLabel = codeToExpand.at(i).label;
            string newLabel = "lb" + to_string(labelSubstitutions++);
            for (unsigned int j = 0; j < codeToExpand.size(); j++) {
                for (unsigned int k = 0; k < codeToExpand.at(j).parameters.size(); k++) {
                    // Check if there's a prefix
                    if (codeToExpand.at(j).parameters.at(k)[0] == '#' || codeToExpand.at(j).parameters.at(k)[0] == '@') {
                        string prefix;
                        prefix = codeToExpand.at(j).parameters.at(k)[0];
                        if (codeToExpand.at(j).parameters.at(k).substr(1,string::npos) == oldLabel)
                            codeToExpand.at(j).parameters.at(k) = (prefix + newLabel);
                    }
                    else {
                        if (codeToExpand.at(j).parameters.at(k) == oldLabel)
                            codeToExpand.at(j).parameters.at(k) = newLabel;
                    }
                }
            }
            codeToExpand.at(i).label = newLabel;
        }
    }

    // Output the code to destination file
    //
    // If the macro call string has a label, it should be preserved
    if (label != "")
        *destFile << label << endl;

    // Add a comment marking the beginning of macro expansion to the assembler program code
    *destFile << "; " << macroToExpand.name << " " << parameters << endl;

    // Output the code line vector
    for (unsigned int i = 0; i < codeToExpand.size(); i++) {
        *destFile << codeToExpand.at(i).label << "\t" << codeToExpand.at(i).opcode;
        if (codeToExpand.at(i).parameters.size() > 0) {
            *destFile << "\t" << codeToExpand.at(i).parameters.at(0);
            for (unsigned int j = 1; j < codeToExpand.at(i).parameters.size(); j++) {
                *destFile << "," << codeToExpand.at(i).parameters.at(j);
            }
        }
        *destFile << endl;
    }

    // Add a comment marking the end of macro expansion to the assembler program code
    *destFile << "; MEND" << endl;
}
