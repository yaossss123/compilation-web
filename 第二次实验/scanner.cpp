/**
 * Lexical Analyzer (Scanner) + DFA Integration
 * Supports: token classification, code line analysis,
 *           .src file scanning, DFA string recognition
 */

#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <set>
#include <map>

using namespace std;

set<string> keywords = {"int", "float", "void", "if", "else", "while", "return", "input", "print"};

string getKeywordType(const string& word) {
    if (word == "int") return "INT";
    if (word == "float") return "FLOAT";
    if (word == "void") return "VOID";
    if (word == "if") return "IF";
    if (word == "else") return "ELSE";
    if (word == "while") return "WHILE";
    if (word == "return") return "RETURN";
    if (word == "input") return "INPUT";
    if (word == "print") return "PRINT";
    return "";
}

struct Token {
    string type;
    string value;
    int line;
};

vector<Token> scanLine(const string& code, int lineNum) {
    vector<Token> tokens;
    size_t i = 0, len = code.length();

    // skip // comments
    size_t cpos = code.find("//");
    if (cpos != string::npos) len = cpos;

    while (i < len) {
        if (isspace((unsigned char)code[i])) { i++; continue; }

        if (isalpha((unsigned char)code[i])) {
            string word = "";
            while (i < len && isalnum((unsigned char)code[i])) { word += code[i]; i++; }
            if (keywords.count(word))
                tokens.push_back({getKeywordType(word), word, lineNum});
            else
                tokens.push_back({"ID", word, lineNum});
            continue;
        }

        if (isdigit((unsigned char)code[i]) || code[i] == '.') {
            string num = "";
            bool hasDot = (code[i] == '.');
            if (code[i] == '.') {
                num += code[i]; i++;
                while (i < len && isdigit((unsigned char)code[i])) { num += code[i]; i++; }
            } else {
                while (i < len && isdigit((unsigned char)code[i])) { num += code[i]; i++; }
                if (i < len && code[i] == '.') {
                    hasDot = true; num += code[i]; i++;
                    while (i < len && isdigit((unsigned char)code[i])) { num += code[i]; i++; }
                }
            }
            if (i < len && (code[i] == 'e' || code[i] == 'E')) {
                hasDot = true; num += code[i]; i++;
                if (i < len && (code[i] == '+' || code[i] == '-')) { num += code[i]; i++; }
                while (i < len && isdigit((unsigned char)code[i])) { num += code[i]; i++; }
            }
            if (hasDot) tokens.push_back({"FLOAT", num, lineNum});
            else tokens.push_back({"NUM", num, lineNum});
            continue;
        }

        // two-char operators (check before single-char)
        if (i + 1 < len) {
            string two = code.substr(i, 2);
            string tp = "";
            if (two == "<=") tp = "LEQ";
            else if (two == ">=") tp = "GEQ";
            else if (two == "==") tp = "EQU";
            else if (two == "!=") tp = "NEQ";
            else if (two == "&&") tp = "AND";
            else if (two == "||") tp = "OR";
            if (!tp.empty()) {
                tokens.push_back({tp, two, lineNum});
                i += 2; continue;
            }
        }

        // single-char operators and delimiters
        char c = code[i];
        string tp = "";
        switch (c) {
            case '+': tp = "ADD"; break;
            case '-': tp = "SUB"; break;
            case '*': tp = "MUL"; break;
            case '/': tp = "DIV"; break;
            case '=': tp = "ASG"; break;
            case '<': tp = "LSS"; break;
            case '>': tp = "GTR"; break;
            case '!': tp = "NOT"; break;
            case '(': tp = "LPAR"; break;
            case ')': tp = "RPAR"; break;
            case '[': tp = "LBK"; break;
            case ']': tp = "RBK"; break;
            case '{': tp = "LBR"; break;
            case '}': tp = "RBR"; break;
            case ',': tp = "CMA"; break;
            case ';': tp = "SEMI"; break;
            default: i++; continue;
        }
        tokens.push_back({tp, string(1, c), lineNum});
        i++;
    }
    return tokens;
}

string analyzeToken(const string& token) {
    if (keywords.count(token)) return getKeywordType(token);
    // two-char operators
    if (token == "<=") return "LEQ";
    if (token == ">=") return "GEQ";
    if (token == "==") return "EQU";
    if (token == "!=") return "NEQ";
    if (token == "&&") return "AND";
    if (token == "||") return "OR";
    // single-char operators
    if (token == "+") return "ADD";
    if (token == "-") return "SUB";
    if (token == "*") return "MUL";
    if (token == "/") return "DIV";
    if (token == "=") return "ASG";
    if (token == "<") return "LSS";
    if (token == ">") return "GTR";
    if (token == ";") return "SEMI";
    if (token == ",") return "CMA";
    if (token == "(") return "LPAR";
    if (token == ")") return "RPAR";
    if (token == "[") return "LBK";
    if (token == "]") return "RBK";
    if (token == "{") return "LBR";
    if (token == "}") return "RBR";
    // check float / integer
    if (!token.empty()) {
        size_t s = 0;
        if (token[0] == '+' || token[0] == '-') s = 1;
        if (s < token.length()) {
            bool hasDot = false, allNum = true;
            for (size_t j = s; j < token.length(); j++) {
                if (token[j] == '.') hasDot = true;
                else if (!isdigit(token[j]) && token[j] != 'e' && token[j] != 'E'
                         && token[j] != '+' && token[j] != '-') allNum = false;
            }
            if (hasDot && allNum) return "FLOAT";
            if (!hasDot && allNum) { bool ok = true; for (size_t j = s; j < token.length(); j++) if (!isdigit(token[j])) ok = false; if (ok) return "NUM"; }
        }
    }
    if (!token.empty() && isalpha(token[0])) {
        bool ok = true;
        for (size_t i = 1; i < token.length(); i++) if (!isalnum(token[i])) ok = false;
        if (ok) return "ID";
    }
    return "UNKNOWN";
}

int main() {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    cout << "========================================" << endl;
    cout << "  Lexical Analyzer + DFA Scanner" << endl;
    cout << "  Mandatory: Mode 1-3  |  Optional: Mode 4" << endl;
    cout << "========================================" << endl;

    int choice;
    while (true) {
        cout << endl;
        cout << "1. Token classification (input n tokens)" << endl;
        cout << "2. Code line analysis (input one line)" << endl;
        cout << "3. Scan source file (.src)" << endl;
        cout << "4. DFA string recognition (load .dfa file)" << endl;
        cout << "0. Exit" << endl;
        cout << "> ";
        if (!(cin >> choice)) { cin.clear(); cin.ignore(10000, '\n'); continue; }

        if (choice == 0) {
            cout << "Done." << endl;
            break;
        }
        else if (choice == 1) {
            int n;
            cout << "Number of tokens: ";
            cin >> n;
            for (int i = 0; i < n; i++) {
                string token;
                cin >> token;
                cout << token << " -> " << analyzeToken(token) << endl;
            }
        }
        else if (choice == 2) {
            string line;
            getline(cin, line);
            cout << "Input code: ";
            getline(cin, line);
            vector<Token> tokens = scanLine(line, 1);
            cout << "Tokens:" << endl;
            for (size_t i = 0; i < tokens.size(); i++) {
                cout << "  (" << tokens[i].type << ", " << tokens[i].value << ")";
                if (i + 1 < tokens.size()) cout << " ->";
            }
            cout << endl;
        }
        else if (choice == 3) {
            string filePath;
            cout << "Source file path: ";
            cin >> filePath;

            ifstream fin(filePath);
            if (!fin.is_open()) {
                cout << "[Error] Cannot open: " << filePath << endl;
                continue;
            }

            string line;
            int lineNum = 0;
            int totalTokens = 0;
            map<string, int> typeCount;

            cout << endl;
            cout << "=== Lexical Analysis: " << filePath << " ===" << endl;
            while (getline(fin, line)) {
                lineNum++;
                vector<Token> tokens = scanLine(line, lineNum);
                for (size_t j = 0; j < tokens.size(); j++) {
                    cout << "  L" << tokens[j].line << ": ("
                         << tokens[j].type << ", " << tokens[j].value << ")" << endl;
                    typeCount[tokens[j].type]++;
                    totalTokens++;
                }
            }
            fin.close();

            cout << endl;
            cout << "--- Summary ---" << endl;
            cout << "Lines: " << lineNum << "  |  Tokens: " << totalTokens << endl;
            for (auto it = typeCount.begin(); it != typeCount.end(); ++it)
                cout << "  " << it->first << ": " << it->second << endl;
        }
        else if (choice == 4) {
            string dfaFile;
            cout << "DFA file path: ";
            cin >> dfaFile;

            ifstream fin(dfaFile);
            if (!fin.is_open()) {
                cout << "[Error] Cannot open: " << dfaFile << endl;
                continue;
            }
            vector<char> alphabet;
            vector<string> states;
            string startState;
            set<string> acceptStates;
            map<pair<string,char>, string> transitions;

            string line;
            getline(fin, line);
            { istringstream iss(line); char c; while (iss >> c) alphabet.push_back(c); }
            getline(fin, line);
            { istringstream iss(line); string s; while (iss >> s) states.push_back(s); }
            getline(fin, line);
            { istringstream iss(line); iss >> startState; }
            getline(fin, line);
            { istringstream iss(line); string a; while (iss >> a) acceptStates.insert(a); }
            while (getline(fin, line)) {
                if (line.empty()) continue;
                istringstream iss(line);
                string from, to; char ch;
                if (iss >> from >> ch >> to)
                    transitions[make_pair(from, ch)] = to;
            }
            fin.close();

            cout << "DFA loaded! Alphabet: {";
            for (size_t i = 0; i < alphabet.size(); i++) {
                if (i > 0) cout << ", ";
                cout << alphabet[i];
            }
            cout << "}, Accept: {";
            bool first = true;
            for (const auto& a : acceptStates) {
                if (!first) cout << ", ";
                cout << a; first = false;
            }
            cout << "}" << endl;

            string input;
            cout << "Input string: ";
            cin >> input;

            string cur = startState;
            vector<string> path;
            path.push_back(cur);
            bool ok = true;
            for (char ch : input) {
                auto key = make_pair(cur, ch);
                if (transitions.find(key) == transitions.end()) {
                    ok = false; break;
                }
                cur = transitions[key];
                path.push_back(cur);
            }
            bool accepted = ok && acceptStates.count(cur) > 0;
            cout << "Path: ";
            for (size_t i = 0; i < path.size(); i++) {
                if (i > 0) cout << " -> ";
                cout << path[i];
            }
            cout << endl;
            cout << "Result: " << (accepted ? "ACCEPT" : "REJECT") << endl;
        }
        else {
            cout << "Invalid choice" << endl;
        }
    }
    return 0;
}
