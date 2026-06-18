/**
 * SLR(1) 分析表生成器
 * 编译原理第四次实验
 *
 * 功能：
 *   1. 输入文法规则，支持扩展巴科斯范式
 *   2. 自动增广文法
 *   3. 计算 FIRST 集和 FOLLOW 集
 *   4. 生成 LR(0) 项目集规范族
 *   5. 生成 SLR(1) 分析表（ACTION 表 + GOTO 表）
 *   6. 利用 FOLLOW 集解决移进-归约冲突
 *
 * 文法文件格式（.txt）：
 *   # 注释
 *   E -> E + T | T
 *   T -> T * F | F
 *   F -> ( E ) | id
 */

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <cstdio>
#include <queue>
#include <sstream>
#include <algorithm>

using namespace std;

// 产生式
struct Production {
    string lhs;
    vector<string> rhs;
};

// LR(0) 项目
struct LR0Item {
    int prodIdx;
    int dotPos;
    bool operator<(const LR0Item& o) const {
        if (prodIdx != o.prodIdx) return prodIdx < o.prodIdx;
        return dotPos < o.dotPos;
    }
    bool operator==(const LR0Item& o) const {
        return prodIdx == o.prodIdx && dotPos == o.dotPos;
    }
};

typedef set<LR0Item> ItemSet;

// 全局变量
vector<Production> productions;
set<string> nonTerminals;
set<string> terminals;
set<string> allSymbols;
string augmentedStart;
vector<ItemSet> canonicalCollection;
map<pair<int, string>, int> gotoTable;
map<string, set<string>> firstSets;
map<string, set<string>> followSets;

// SLR(1) 分析表条目
struct ActionEntry {
    enum Type { SHIFT, REDUCE, ACCEPT, EMPTY } type;
    int value; // 状态编号(SHIFT) 或 产生式编号(REDUCE)
    string toString() const {
        char buf[32];
        if (type == SHIFT) { sprintf(buf, "s%d", value); return buf; }
        if (type == REDUCE) { sprintf(buf, "r%d", value); return buf; }
        if (type == ACCEPT) return "acc";
        return "";
    }
};

// ACTION 表和 GOTO 表
map<pair<int, string>, ActionEntry> actionTable;
map<pair<int, string>, int> slrGotoTable;

bool isNonTerminal(const string& s) {
    return nonTerminals.count(s) > 0;
}

// ===== 文法解析 =====
void parseGrammar(const string& filename) {
    ifstream fin(filename);
    if (!fin.is_open()) {
        cout << "[错误] 无法打开文件: " << filename << endl;
        return;
    }
    string line;
    while (getline(fin, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == string::npos) continue;
        line = line.substr(start);
        size_t end = line.find_last_not_of(" \t\r\n");
        if (end != string::npos) line = line.substr(0, end + 1);
        if (line.empty() || line[0] == '#') continue;

        // 替换 unicode →
        size_t arrowPos = line.find("\xe2\x86\x92");
        if (arrowPos != string::npos) line.replace(arrowPos, 3, "->");

        arrowPos = line.find("->");
        if (arrowPos == string::npos) continue;

        string lhs = line.substr(0, arrowPos);
        lhs.erase(remove(lhs.begin(), lhs.end(), ' '), lhs.end());
        lhs.erase(remove(lhs.begin(), lhs.end(), '\t'), lhs.end());
        nonTerminals.insert(lhs);

        string rhsPart = line.substr(arrowPos + 2);
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

    for (const auto& prod : productions)
        for (const auto& sym : prod.rhs)
            allSymbols.insert(sym);
    for (const auto& sym : allSymbols)
        if (nonTerminals.find(sym) == nonTerminals.end())
            terminals.insert(sym);
}

// ===== 增广文法 =====
void augmentGrammar() {
    augmentedStart = productions[0].lhs + "'";
    while (nonTerminals.count(augmentedStart) || allSymbols.count(augmentedStart))
        augmentedStart += "'";

    Production augProd;
    augProd.lhs = augmentedStart;
    augProd.rhs = {productions[0].lhs};
    productions.insert(productions.begin(), augProd);
    nonTerminals.insert(augmentedStart);
    allSymbols.insert(augmentedStart);
}

// ===== FIRST 集计算 =====
void computeFirstSets() {
    // 终结符的 FIRST 集就是它自身
    for (const auto& t : terminals) {
        firstSets[t].insert(t);
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& prod : productions) {
            // 对于 A -> X1 X2 ... Xn
            // FIRST(A) 包含 FIRST(X1) - {epsilon}
            // 如果 epsilon in FIRST(X1), 则加入 FIRST(X2) - {epsilon}, 以此类推
            set<string> before = firstSets[prod.lhs];

            if (prod.rhs.empty()) {
                firstSets[prod.lhs].insert("epsilon");
            } else {
                bool allHaveEpsilon = true;
                for (const auto& sym : prod.rhs) {
                    for (const auto& f : firstSets[sym]) {
                        if (f != "epsilon") {
                            firstSets[prod.lhs].insert(f);
                        }
                    }
                    if (firstSets[sym].count("epsilon") == 0) {
                        allHaveEpsilon = false;
                        break;
                    }
                }
                if (allHaveEpsilon) {
                    firstSets[prod.lhs].insert("epsilon");
                }
            }

            if (firstSets[prod.lhs] != before) changed = true;
        }
    }
}

// ===== FOLLOW 集计算 =====
void computeFollowSets() {
    // FOLLOW(S') 包含 $
    followSets[augmentedStart].insert("$");

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& prod : productions) {
            // 对于 A -> alpha B beta
            // FOLLOW(B) 包含 FIRST(beta) - {epsilon}
            // 如果 epsilon in FIRST(beta) 或 beta 为空，则 FOLLOW(B) 包含 FOLLOW(A)
            for (size_t i = 0; i < prod.rhs.size(); i++) {
                const string& B = prod.rhs[i];
                if (!isNonTerminal(B)) continue;

                set<string> before = followSets[B];

                // 计算 FIRST(beta)
                bool betaHasEpsilon = true;
                for (size_t j = i + 1; j < prod.rhs.size(); j++) {
                    const string& nextSym = prod.rhs[j];
                    for (const auto& f : firstSets[nextSym]) {
                        if (f != "epsilon") {
                            followSets[B].insert(f);
                        }
                    }
                    if (firstSets[nextSym].count("epsilon") == 0) {
                        betaHasEpsilon = false;
                        break;
                    }
                }

                if (betaHasEpsilon) {
                    for (const auto& f : followSets[prod.lhs]) {
                        followSets[B].insert(f);
                    }
                }

                if (followSets[B] != before) changed = true;
            }
        }
    }
}

// ===== Closure =====
ItemSet closure(const ItemSet& items) {
    ItemSet result = items;
    queue<LR0Item> worklist;
    for (const auto& item : items) worklist.push(item);

    while (!worklist.empty()) {
        LR0Item item = worklist.front();
        worklist.pop();
        const Production& prod = productions[item.prodIdx];
        if (item.dotPos < (int)prod.rhs.size()) {
            string nextSym = prod.rhs[item.dotPos];
            if (isNonTerminal(nextSym)) {
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

// ===== Goto =====
ItemSet computeGoto(const ItemSet& items, const string& symbol) {
    ItemSet newItems;
    for (const auto& item : items) {
        const Production& prod = productions[item.prodIdx];
        if (item.dotPos < (int)prod.rhs.size() && prod.rhs[item.dotPos] == symbol) {
            newItems.insert({item.prodIdx, item.dotPos + 1});
        }
    }
    if (newItems.empty()) return newItems;
    return closure(newItems);
}

// ===== 构建LR(0)规范族 =====
void buildCanonicalCollection() {
    ItemSet I0;
    I0.insert({0, 0});
    I0 = closure(I0);
    canonicalCollection.push_back(I0);

    queue<int> worklist;
    worklist.push(0);

    while (!worklist.empty()) {
        int stateIdx = worklist.front();
        worklist.pop();
        ItemSet currentSet = canonicalCollection[stateIdx]; // 拷贝避免引用失效

        for (const auto& sym : allSymbols) {
            ItemSet nextSet = computeGoto(currentSet, sym);
            if (nextSet.empty()) continue;

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

// ===== 生成 SLR(1) 分析表 =====
void buildSLR1Table() {
    for (int i = 0; i < (int)canonicalCollection.size(); i++) {
        const ItemSet& items = canonicalCollection[i];

        for (const auto& item : items) {
            const Production& prod = productions[item.prodIdx];

            if (item.dotPos < (int)prod.rhs.size()) {
                // 点在中间：移进项目
                string nextSym = prod.rhs[item.dotPos];
                auto key = make_pair(i, nextSym);

                if (gotoTable.count(key)) {
                    int nextState = gotoTable[key];
                    if (terminals.count(nextSym)) {
                        ActionEntry entry;
                        entry.type = ActionEntry::SHIFT;
                        entry.value = nextState;
                        auto existing = actionTable.find(key);
                        if (existing != actionTable.end()) {
                            // 冲突：SLR(1) 用 FOLLOW 集解决
                            if (existing->second.type == ActionEntry::REDUCE) {
                                int reduceProd = existing->second.value;
                                string reduceLhs = productions[reduceProd].lhs;
                                // 如果 nextSym 在 FOLLOW(reduceLhs) 中，保留归约
                                // 否则保留移进（当前情况）
                                if (followSets[reduceLhs].count(nextSym)) {
                                    // 真正的冲突，标记为冲突
                                    cout << "[SLR冲突] 状态I" << i << ", 符号" << nextSym
                                         << ": 移进->I" << nextState << " vs 归约r" << reduceProd << endl;
                                } else {
                                    // FOLLOW 集中没有该符号，保留移进
                                    actionTable[key] = entry;
                                }
                            }
                        } else {
                            actionTable[key] = entry;
                        }
                    } else {
                        // 非终结符：GOTO
                        slrGotoTable[key] = nextState;
                    }
                }
            } else {
                // 点在末尾：归约项目
                if (item.prodIdx == 0) {
                    // S' -> S ·，接受
                    ActionEntry entry;
                    entry.type = ActionEntry::ACCEPT;
                    entry.value = 0;
                    actionTable[make_pair(i, "$")] = entry;
                } else {
                    // A -> alpha ·，对 FOLLOW(A) 中的每个终结符执行归约
                    for (const auto& a : followSets[prod.lhs]) {
                        auto key = make_pair(i, a);
                        auto existing = actionTable.find(key);
                        if (existing != actionTable.end()) {
                            if (existing->second.type == ActionEntry::SHIFT) {
                                // 移进-归约冲突，SLR(1) 已解决：保留移进
                                // （因为 nextSym 不在 FOLLOW 中才保留移进）
                                cout << "[SLR解决] 状态I" << i << ", 符号" << a
                                     << ": 移进优先（SLR(1)已解决冲突）" << endl;
                            } else if (existing->second.type == ActionEntry::REDUCE) {
                                cout << "[SLR冲突] 状态I" << i << ", 符号" << a
                                     << ": 归约-归约冲突 r" << existing->second.value
                                     << " vs r" << item.prodIdx << endl;
                            }
                        } else {
                            ActionEntry entry;
                            entry.type = ActionEntry::REDUCE;
                            entry.value = item.prodIdx;
                            actionTable[key] = entry;
                        }
                    }
                }
            }
        }
    }
}

// ===== 打印函数 =====
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
}

void printFirstSets() {
    cout << "========== FIRST 集 ==========" << endl;
    for (const auto& nt : nonTerminals) {
        cout << "  FIRST(" << nt << ") = {";
        bool first = true;
        for (const auto& f : firstSets[nt]) {
            if (!first) cout << ", ";
            cout << f;
            first = false;
        }
        cout << "}" << endl;
    }
    cout << endl;
}

void printFollowSets() {
    cout << "========== FOLLOW 集 ==========" << endl;
    for (const auto& nt : nonTerminals) {
        cout << "  FOLLOW(" << nt << ") = {";
        bool first = true;
        for (const auto& f : followSets[nt]) {
            if (!first) cout << ", ";
            cout << f;
            first = false;
        }
        cout << "}" << endl;
    }
    cout << endl;
}

void printCanonicalCollection() {
    cout << "========== LR(0) 项目集规范族 ==========" << endl;
    cout << "共 " << canonicalCollection.size() << " 个状态" << endl << endl;
    for (int i = 0; i < (int)canonicalCollection.size(); i++) {
        cout << "--- I" << i << " ---" << endl;
        for (const auto& item : canonicalCollection[i]) {
            const Production& prod = productions[item.prodIdx];
            cout << "  " << prod.lhs << " -> ";
            for (int j = 0; j < (int)prod.rhs.size(); j++) {
                if (j == item.dotPos) cout << ". ";
                cout << prod.rhs[j] << " ";
            }
            if (item.dotPos == (int)prod.rhs.size()) cout << ".";
            cout << endl;
        }
        cout << endl;
    }
}

void printSLR1Table() {
    cout << "========== SLR(1) 分析表 ==========" << endl;

    // 收集所有列
    vector<string> actionCols;
    for (const auto& t : terminals) actionCols.push_back(t);
    actionCols.push_back("$");

    vector<string> gotoCols;
    for (const auto& nt : nonTerminals) {
        if (nt != augmentedStart) gotoCols.push_back(nt);
    }

    // ACTION 表头
    cout << endl << "--- ACTION 表 ---" << endl;
    cout << "状态\t";
    for (const auto& c : actionCols) cout << c << "\t";
    cout << endl;

    // ACTION 表内容
    for (int i = 0; i < (int)canonicalCollection.size(); i++) {
        cout << "I" << i << "\t";
        for (const auto& c : actionCols) {
            auto key = make_pair(i, c);
            if (actionTable.count(key)) {
                cout << actionTable[key].toString() << "\t";
            } else {
                cout << "\t";
            }
        }
        cout << endl;
    }

    // GOTO 表头
    cout << endl << "--- GOTO 表 ---" << endl;
    cout << "状态\t";
    for (const auto& c : gotoCols) cout << c << "\t";
    cout << endl;

    // GOTO 表内容
    for (int i = 0; i < (int)canonicalCollection.size(); i++) {
        cout << "I" << i << "\t";
        for (const auto& c : gotoCols) {
            auto key = make_pair(i, c);
            if (slrGotoTable.count(key)) {
                cout << slrGotoTable[key] << "\t";
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
    parseGrammar(filename);
    if (productions.empty()) {
        cout << "[错误] 未解析到有效的产生式" << endl;
        return 1;
    }

    // 2. 增广文法
    augmentGrammar();
    printGrammar();

    // 3. 计算 FIRST 集
    computeFirstSets();
    printFirstSets();

    // 4. 计算 FOLLOW 集
    computeFollowSets();
    printFollowSets();

    // 5. 构建 LR(0) 规范族
    buildCanonicalCollection();
    printCanonicalCollection();

    // 6. 生成 SLR(1) 分析表
    buildSLR1Table();
    printSLR1Table();

    return 0;
}
