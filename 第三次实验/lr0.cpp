/**
 * LR(0) 项目集规范族 (Canonical Collection of LR(0) Items)
 * 编译原理第三次实验
 *
 * 功能：
 *   1. 输入文法规则，支持扩展巴科斯范式 (E → E + T | T)
 *   2. 自动增广文法：添加 S' → S
 *   3. 生成LR(0)项目集规范族（Closure + Goto）
 *   4. 输出所有项目集和状态转移关系
 *   5. 检测是否存在移进-归约或归约-归约冲突
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

// 产生式：lhs -> rhs
struct Production {
    string lhs;
    vector<string> rhs;
};

// LR(0) 项目：产生式编号 + 点位置
struct LR0Item {
    int prodIdx;   // 产生式编号
    int dotPos;    // 点的位置（0表示点在开头）

    bool operator<(const LR0Item& o) const {
        if (prodIdx != o.prodIdx) return prodIdx < o.prodIdx;
        return dotPos < o.dotPos;
    }
    bool operator==(const LR0Item& o) const {
        return prodIdx == o.prodIdx && dotPos == o.dotPos;
    }
};

// 项目集
typedef set<LR0Item> ItemSet;

// 全局变量
vector<Production> productions;       // 所有产生式
set<string> nonTerminals;             // 非终结符集合
set<string> terminals;                // 终结符集合
set<string> allSymbols;               // 所有符号（终结符+非终结符）
string augmentedStart;                // 增广后的起始符号

// 项目集规范族
vector<ItemSet> canonicalCollection;
// 转移关系：(状态编号, 符号) -> 新状态编号
map<pair<int, string>, int> gotoTable;

// 判断是否为非终结符
bool isNonTerminal(const string& s) {
    return nonTerminals.count(s) > 0;
}

// 解析文法文件
void parseGrammar(const string& filename) {
    ifstream fin(filename);
    if (!fin.is_open()) {
        cout << "[错误] 无法打开文件: " << filename << endl;
        return;
    }

    string line;
    string firstLhs = "";

    while (getline(fin, line)) {
        // 去除行首行尾空白
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == string::npos) continue;
        line = line.substr(start);
        size_t end = line.find_last_not_of(" \t\r\n");
        if (end != string::npos) line = line.substr(0, end + 1);
        if (line.empty() || line[0] == '#') continue; // 跳过注释

        // 替换 → 为 ->
        size_t arrowPos = line.find("\xe2\x86\x92");
        if (arrowPos != string::npos) {
            line.replace(arrowPos, 3, "->");
        }

        // 查找 ->
        arrowPos = line.find("->");
        if (arrowPos == string::npos) continue;

        string lhs = line.substr(0, arrowPos);
        string rhsPart = line.substr(arrowPos + 2);

        // 去除lhs空白
        lhs.erase(remove(lhs.begin(), lhs.end(), ' '), lhs.end());
        lhs.erase(remove(lhs.begin(), lhs.end(), '\t'), lhs.end());

        if (firstLhs.empty()) firstLhs = lhs;

        nonTerminals.insert(lhs);

        // 按 | 分割 rhs
        istringstream iss(rhsPart);
        string token;
        vector<string> currentRhs;

        while (iss >> token) {
            if (token == "|") {
                if (!currentRhs.empty()) {
                    productions.push_back({lhs, currentRhs});
                    currentRhs.clear();
                }
            } else {
                currentRhs.push_back(token);
            }
        }
        if (!currentRhs.empty()) {
            productions.push_back({lhs, currentRhs});
        }
    }
    fin.close();

    // 收集所有符号
    for (const auto& prod : productions) {
        for (const auto& sym : prod.rhs) {
            allSymbols.insert(sym);
        }
    }

    // 终结符 = 所有符号 - 非终结符
    for (const auto& sym : allSymbols) {
        if (nonTerminals.find(sym) == nonTerminals.end()) {
            terminals.insert(sym);
        }
    }
}

// 增广文法
void augmentGrammar() {
    augmentedStart = productions[0].lhs + "'";

    // 确保增广起始符号不与已有符号冲突
    while (nonTerminals.count(augmentedStart) || allSymbols.count(augmentedStart)) {
        augmentedStart += "'";
    }

    Production augProd;
    augProd.lhs = augmentedStart;
    augProd.rhs = {productions[0].lhs};

    // 插入到产生式列表开头
    productions.insert(productions.begin(), augProd);
    nonTerminals.insert(augmentedStart);
    allSymbols.insert(augmentedStart);
}

// 计算闭包
ItemSet closure(const ItemSet& items) {
    ItemSet result = items;
    queue<LR0Item> worklist;

    for (const auto& item : items) {
        worklist.push(item);
    }

    while (!worklist.empty()) {
        LR0Item item = worklist.front();
        worklist.pop();

        const Production& prod = productions[item.prodIdx];
        if (item.dotPos < (int)prod.rhs.size()) {
            string nextSym = prod.rhs[item.dotPos];
            if (isNonTerminal(nextSym)) {
                // 添加所有 nextSym 的产生式，点在最前
                for (int i = 0; i < (int)productions.size(); i++) {
                    if (productions[i].lhs == nextSym) {
                        LR0Item newItem = {i, 0};
                        if (result.find(newItem) == result.end()) {
                            result.insert(newItem);
                            worklist.push(newItem);
                        }
                    }
                }
            }
        }
    }

    return result;
}

// 计算 Goto
ItemSet computeGoto(const ItemSet& items, const string& symbol) {
    ItemSet newItems;

    for (const auto& item : items) {
        const Production& prod = productions[item.prodIdx];
        if (item.dotPos < (int)prod.rhs.size() && prod.rhs[item.dotPos] == symbol) {
            LR0Item newItem = {item.prodIdx, item.dotPos + 1};
            newItems.insert(newItem);
        }
    }

    if (newItems.empty()) return newItems;
    return closure(newItems);
}

// 构建LR(0)项目集规范族
void buildCanonicalCollection() {
    // 初始项目集 I0：S' → ·S
    ItemSet I0;
    I0.insert({0, 0}); // 产生式0，点在位置0
    I0 = closure(I0);
    canonicalCollection.push_back(I0);

    // BFS 扩展
    queue<int> worklist;
    worklist.push(0);

    while (!worklist.empty()) {
        int stateIdx = worklist.front();
        worklist.pop();

        // 拷贝当前项目集，避免push_back导致引用失效
        ItemSet currentSet = canonicalCollection[stateIdx];

        // 对每个符号尝试 Goto
        for (const auto& sym : allSymbols) {
            ItemSet nextSet = computeGoto(currentSet, sym);
            if (nextSet.empty()) continue;

            // 查找是否已存在相同的项目集
            int foundIdx = -1;
            for (int i = 0; i < (int)canonicalCollection.size(); i++) {
                if (canonicalCollection[i] == nextSet) {
                    foundIdx = i;
                    break;
                }
            }

            if (foundIdx == -1) {
                foundIdx = canonicalCollection.size();
                canonicalCollection.push_back(nextSet);
                worklist.push(foundIdx);
            }

            gotoTable[make_pair(stateIdx, sym)] = foundIdx;
        }
    }
}

// 打印项目
void printItem(const LR0Item& item) {
    const Production& prod = productions[item.prodIdx];
    cout << "  " << prod.lhs << " -> ";
    for (int i = 0; i < (int)prod.rhs.size(); i++) {
        if (i == item.dotPos) cout << "· ";
        cout << prod.rhs[i] << " ";
    }
    if (item.dotPos == (int)prod.rhs.size()) cout << "·";
    cout << endl;
}

// 打印项目集
void printItemSet(const ItemSet& items) {
    for (const auto& item : items) {
        printItem(item);
    }
}

// 打印增广文法
void printGrammar() {
    cout << "========== 增广文法 ==========" << endl;
    for (int i = 0; i < (int)productions.size(); i++) {
        cout << "  (" << i << ") " << productions[i].lhs << " -> ";
        for (int j = 0; j < (int)productions[i].rhs.size(); j++) {
            if (j > 0) cout << " ";
            cout << productions[i].rhs[j];
        }
        cout << endl;
    }
    cout << endl;

    cout << "非终结符: {";
    bool first = true;
    for (const auto& nt : nonTerminals) {
        if (!first) cout << ", ";
        cout << nt;
        first = false;
    }
    cout << "}" << endl;

    cout << "终结符:   {";
    first = true;
    for (const auto& t : terminals) {
        if (!first) cout << ", ";
        cout << t;
        first = false;
    }
    cout << "}" << endl;
    cout << endl;
}

// 检测LR(0)冲突
bool checkLR0Conflicts() {
    bool hasConflict = false;

    for (int i = 0; i < (int)canonicalCollection.size(); i++) {
        const ItemSet& items = canonicalCollection[i];

        vector<int> shiftItems;   // 移进项目（点不在末尾）
        vector<int> reduceItems;  // 归约项目（点在末尾）

        for (const auto& item : items) {
            const Production& prod = productions[item.prodIdx];
            if (item.dotPos == (int)prod.rhs.size()) {
                reduceItems.push_back(item.prodIdx);
            } else {
                shiftItems.push_back(item.prodIdx);
            }
        }

        // 移进-归约冲突
        if (!shiftItems.empty() && !reduceItems.empty()) {
            cout << "[冲突] 状态 I" << i << " 存在移进-归约冲突:" << endl;
            cout << "  移进项目:" << endl;
            for (int idx : shiftItems) {
                for (const auto& item : items) {
                    if (item.prodIdx == idx && item.dotPos < (int)productions[idx].rhs.size()) {
                        cout << "    ";
                        printItem(item);
                    }
                }
            }
            cout << "  归约项目:" << endl;
            for (int idx : reduceItems) {
                for (const auto& item : items) {
                    if (item.prodIdx == idx) {
                        cout << "    ";
                        printItem(item);
                    }
                }
            }
            hasConflict = true;
        }

        // 归约-归约冲突
        if (reduceItems.size() > 1) {
            // 排除增广起始产生式 S' -> S 的归约（接受）
            int realReduce = 0;
            for (int idx : reduceItems) {
                if (idx != 0) realReduce++; // 0号产生式是增广的
            }
            if (realReduce > 1) {
                cout << "[冲突] 状态 I" << i << " 存在归约-归约冲突:" << endl;
                for (int idx : reduceItems) {
                    if (idx != 0) {
                        for (const auto& item : items) {
                            if (item.prodIdx == idx) {
                                cout << "    ";
                                printItem(item);
                            }
                        }
                    }
                }
                hasConflict = true;
            }
        }
    }

    return hasConflict;
}

// 打印规范族和转移关系
void printCanonicalCollection() {
    cout << "========== LR(0) 项目集规范族 ==========" << endl;
    cout << "共 " << canonicalCollection.size() << " 个状态" << endl << endl;

    for (int i = 0; i < (int)canonicalCollection.size(); i++) {
        cout << "--- I" << i << " ---" << endl;
        printItemSet(canonicalCollection[i]);
        cout << endl;
    }

    cout << "========== 状态转移关系 ==========" << endl;
    for (const auto& entry : gotoTable) {
        cout << "  I" << entry.first.first << " --" << entry.first.second
             << "--> I" << entry.second << endl;
    }
    cout << "共 " << gotoTable.size() << " 个转移函数" << endl;
    cout << endl;

    // 打印转移表（矩阵形式）
    cout << "========== GOTO 表 ==========" << endl;
    // 收集所有出现的符号
    set<string> tableSymbols;
    for (const auto& entry : gotoTable) {
        tableSymbols.insert(entry.first.second);
    }

    // 表头
    cout << "\t";
    for (const auto& sym : tableSymbols) {
        cout << sym << "\t";
    }
    cout << endl;

    // 表内容
    for (int i = 0; i < (int)canonicalCollection.size(); i++) {
        cout << "I" << i << "\t";
        for (const auto& sym : tableSymbols) {
            auto key = make_pair(i, sym);
            if (gotoTable.count(key)) {
                cout << "I" << gotoTable[key] << "\t";
            } else {
                cout << "\t";
            }
        }
        cout << endl;
    }
    cout << endl;
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    string filename;

    if (argc > 1) {
        filename = argv[1];
    } else {
        cout << "请输入文法文件路径: ";
        cin >> filename;
    }

    // 1. 解析文法
    cout << "========== 原始文法 ==========" << endl;
    parseGrammar(filename);

    if (productions.empty()) {
        cout << "[错误] 未解析到有效的产生式" << endl;
        return 1;
    }

    for (const auto& p : productions) {
        cout << "  " << p.lhs << " -> ";
        for (int j = 0; j < (int)p.rhs.size(); j++) {
            if (j > 0) cout << " ";
            cout << p.rhs[j];
        }
        cout << endl;
    }
    cout << endl;

    // 2. 增广文法
    augmentGrammar();
    printGrammar();

    // 3. 构建LR(0)项目集规范族
    buildCanonicalCollection();

    // 4. 输出规范族
    printCanonicalCollection();

    // 5. 检查LR(0)冲突
    cout << "========== LR(0) 冲突检测 ==========" << endl;
    bool hasConflict = checkLR0Conflicts();
    if (hasConflict) {
        cout << "该文法不是 LR(0) 文法（存在冲突）" << endl;
    } else {
        cout << "该文法是 LR(0) 文法（无冲突）" << endl;
    }

    return 0;
}
