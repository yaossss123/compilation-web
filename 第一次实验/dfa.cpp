/**
 * DFA 模拟器 (Deterministic Finite Automaton Simulator)
 * 编译原理第一次实验
 *
 * 功能：
 *   1. 从文件读取DFA五元组（字符集、状态集、开始状态、接受状态集、状态转换表）
 *   2. 检查DFA合法性（开始状态唯一且在状态集中，接受状态集非空且在状态集中）
 *   3. 输出长度≤N的所有合法字符串（属于DFA语言集）
 *   4. 模拟DFA识别字符串的过程，判定字符串是否被接受
 *
 * 输入文件格式（.dfa）：
 *   第1行：字符集（空格分隔的字符）
 *   第2行：状态集（空格分隔的状态名）
 *   第3行：开始状态
 *   第4行：接受状态集（空格分隔）
 *   第5行起：状态转换表，格式为 "当前状态 输入字符 下一状态"
 */

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <sstream>
#include <algorithm>

using namespace std;

// DFA 结构体
struct DFA {
    vector<char> alphabet;          // 字符集（有序）
    vector<string> states;          // 状态集
    string startState;              // 开始状态
    set<string> acceptStates;       // 接受状态集
    map<pair<string, char>, string> transitions; // 状态转换表

    // 检查DFA合法性
    bool validate() {
        bool valid = true;

        // 检查开始状态是否在状态集中
        bool startFound = false;
        for (const auto& s : states) {
            if (s == startState) {
                startFound = true;
                break;
            }
        }
        if (!startFound) {
            cout << "[错误] 开始状态 '" << startState << "' 不在状态集中" << endl;
            valid = false;
        }

        // 检查开始状态唯一性（已在读取时保证只有一个）
        cout << "[检查] 开始状态: " << startState << " - "
             << (startFound ? "合法" : "非法") << endl;

        // 检查接受状态集是否为空
        if (acceptStates.empty()) {
            cout << "[错误] 接受状态集为空" << endl;
            valid = false;
        } else {
            cout << "[检查] 接受状态集非空 - 合法" << endl;
        }

        // 检查接受状态是否都在状态集中
        set<string> stateSet(states.begin(), states.end());
        for (const auto& as : acceptStates) {
            if (stateSet.find(as) == stateSet.end()) {
                cout << "[错误] 接受状态 '" << as << "' 不在状态集中" << endl;
                valid = false;
            }
        }

        return valid;
    }

    // 模拟DFA识别字符串，返回经过的状态序列
    bool simulate(const string& input, vector<string>& statePath) {
        statePath.clear();
        string currentState = startState;
        statePath.push_back(currentState);

        for (char c : input) {
            // 检查字符是否在字符集中
            bool charFound = false;
            for (char a : alphabet) {
                if (a == c) {
                    charFound = true;
                    break;
                }
            }
            if (!charFound) {
                return false; // 字符不在字符集中，拒绝
            }

            // 查找转换
            auto key = make_pair(currentState, c);
            if (transitions.find(key) == transitions.end()) {
                return false; // 无转换，拒绝
            }
            currentState = transitions[key];
            statePath.push_back(currentState);
        }

        // 检查最终状态是否为接受状态
        return acceptStates.count(currentState) > 0;
    }

    // 生成所有长度≤N的合法字符串
    vector<pair<string, vector<string>>> generateAll(int maxLen) {
        vector<pair<string, vector<string>>> results;

        // BFS：队列中存储 (当前字符串, 当前状态, 状态路径)
        queue<tuple<string, string, vector<string>>> q;

        vector<string> initPath;
        initPath.push_back(startState);
        q.push(make_tuple("", startState, initPath));

        while (!q.empty()) {
            auto front = q.front();
            q.pop();
            string currentStr = get<0>(front);
            string currentState = get<1>(front);
            vector<string> currentPath = get<2>(front);

            // 如果当前字符串非空且当前状态是接受状态，记录结果
            if (!currentStr.empty() && acceptStates.count(currentState) > 0) {
                results.push_back(make_pair(currentStr, currentPath));
            }

            // 如果还没达到最大长度，继续扩展
            if ((int)currentStr.length() < maxLen) {
                for (char c : alphabet) {
                    auto key = make_pair(currentState, c);
                    if (transitions.find(key) != transitions.end()) {
                        string nextState = transitions[key];
                        string newStr = currentStr + c;
                        vector<string> newPath = currentPath;
                        newPath.push_back(nextState);
                        q.push(make_tuple(newStr, nextState, newPath));
                    }
                }
            }
        }

        return results;
    }

    // 打印DFA信息
    void printInfo() {
        cout << "========== DFA 信息 ==========" << endl;
        cout << "字符集: {";
        for (size_t i = 0; i < alphabet.size(); i++) {
            if (i > 0) cout << ", ";
            cout << alphabet[i];
        }
        cout << "}" << endl;

        cout << "状态集: {";
        for (size_t i = 0; i < states.size(); i++) {
            if (i > 0) cout << ", ";
            cout << states[i];
        }
        cout << "}" << endl;

        cout << "开始状态: " << startState << endl;

        cout << "接受状态集: {";
        bool first = true;
        for (const auto& s : acceptStates) {
            if (!first) cout << ", ";
            cout << s;
            first = false;
        }
        cout << "}" << endl;

        cout << "状态转换表:" << endl;
        for (const auto& t : transitions) {
            cout << "  " << t.first.first << " --" << t.first.second
                 << "--> " << t.second << endl;
        }
        cout << "==============================" << endl;
    }
};

// 从文件读取DFA
bool loadDFA(const string& filename, DFA& dfa) {
    ifstream fin(filename);
    if (!fin.is_open()) {
        cout << "[错误] 无法打开文件: " << filename << endl;
        return false;
    }

    string line;

    // 第1行：字符集
    if (!getline(fin, line)) return false;
    istringstream iss1(line);
    char c;
    while (iss1 >> c) {
        dfa.alphabet.push_back(c);
    }

    // 第2行：状态集
    if (!getline(fin, line)) return false;
    istringstream iss2(line);
    string state;
    while (iss2 >> state) {
        dfa.states.push_back(state);
    }

    // 第3行：开始状态
    if (!getline(fin, line)) return false;
    istringstream iss3(line);
    iss3 >> dfa.startState;

    // 第4行：接受状态集
    if (!getline(fin, line)) return false;
    istringstream iss4(line);
    string accState;
    while (iss4 >> accState) {
        dfa.acceptStates.insert(accState);
    }

    // 第5行起：状态转换表
    while (getline(fin, line)) {
        if (line.empty()) continue;
        istringstream iss(line);
        string from, to;
        char ch;
        if (iss >> from >> ch >> to) {
            dfa.transitions[make_pair(from, ch)] = to;
        }
    }

    fin.close();
    return true;
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    string filename;

    if (argc > 1) {
        filename = argv[1];
    } else {
        cout << "请输入DFA文件路径: ";
        cin >> filename;
    }

    DFA dfa;
    if (!loadDFA(filename, dfa)) {
        return 1;
    }

    // 打印DFA信息
    dfa.printInfo();
    cout << endl;

    // 检查DFA合法性
    cout << "========== DFA 合法性检查 ==========" << endl;
    if (!dfa.validate()) {
        cout << "DFA 不合法，程序终止。" << endl;
        return 1;
    }
    cout << "DFA 合法性检查通过！" << endl;
    cout << endl;

    int choice;
    while (true) {
        cout << "========== 功能菜单 ==========" << endl;
        cout << "1. 输出长度≤N的所有合法字符串" << endl;
        cout << "2. 判定输入字符串是否被DFA接受" << endl;
        cout << "3. 退出" << endl;
        cout << "请选择功能: ";
        cin >> choice;

        if (choice == 1) {
            int maxLen;
            cout << "请输入最大长度 N: ";
            cin >> maxLen;

            cout << endl << "========== 长度≤" << maxLen << "的合法字符串 ==========" << endl;
            auto results = dfa.generateAll(maxLen);

            if (results.empty()) {
                cout << "未找到合法字符串" << endl;
            } else {
                for (const auto& r : results) {
                    cout << r.first << " -> ";
                    for (size_t i = 0; i < r.second.size(); i++) {
                        if (i > 0) cout << ",";
                        cout << r.second[i];
                    }
                    cout << endl;
                }
                cout << "共 " << results.size() << " 个合法字符串" << endl;
            }
            cout << endl;

        } else if (choice == 2) {
            string input;
            cout << "请输入待判定的字符串: ";
            cin >> input;

            vector<string> statePath;
            bool accepted = dfa.simulate(input, statePath);

            cout << endl << "========== DFA 识别结果 ==========" << endl;
            cout << "输入字符串: " << input << endl;
            cout << "状态序列: ";
            for (size_t i = 0; i < statePath.size(); i++) {
                if (i > 0) cout << ",";
                cout << statePath[i];
            }
            cout << endl;
            cout << "判定结果: " << (accepted ? "接受（合法字符串）" : "拒绝（非法字符串）") << endl;
            cout << endl;

        } else if (choice == 3) {
            break;
        } else {
            cout << "无效选择，请重新输入" << endl;
        }
    }

    cout << "程序结束。" << endl;
    return 0;
}
