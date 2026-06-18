/**
 * 语义分析器 (Semantic Analyzer)
 * 编译原理第五次实验
 *
 * 功能：
 *   1. 词法分析：将源代码转换为 Token 流
 *   2. 语法分析：递归下降解析，构建抽象语法树 (AST)
 *   3. 符号表管理：支持变量声明、作用域嵌套、类型查询
 *   4. 语义检查：检测重复声明、未声明变量、类型不匹配等错误
 *
 * 支持的简化文法：
 *   P  -> { D } { S }    程序
 *   D  -> T d ;          声明
 *   T  -> int | float | void
 *   S  -> d = E ;        赋值
 *       | if ( B ) S     条件
 *       | if ( B ) S else S
 *       | while ( B ) S  循环
 *       | return E ;     返回
 *       | print E ;      输出
 *       | input d ;      输入
 *       | { S_list }     复合语句
 *   E  -> E + T | E - T | E * T | E / T | ( E ) | d | i | f | -E
 *   B  -> E rel E        布尔表达式 (rel: < > <= >= == !=)
 */

extern "C" {
    __declspec(dllimport) int __stdcall SetConsoleOutputCP(unsigned int);
    __declspec(dllimport) int __stdcall SetConsoleCP(unsigned int);
}
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <cstdio>

using namespace std;

// ===== Token 定义 =====
enum TokenType {
    // 关键字
    TK_INT, TK_FLOAT, TK_VOID, TK_IF, TK_ELSE, TK_WHILE,
    TK_RETURN, TK_INPUT, TK_PRINT,
    // 标识符和常量
    TK_ID, TK_NUM, TK_FLOAT_NUM,
    // 运算符
    TK_PLUS, TK_MINUS, TK_MUL, TK_DIV,
    TK_ASSIGN, TK_LT, TK_GT, TK_LE, TK_GE, TK_EQ, TK_NE,
    TK_AND, TK_OR, TK_NOT,
    // 界符
    TK_LPAREN, TK_RPAREN, TK_LBRACE, TK_RBRACE,
    TK_LBRACKET, TK_RBRACKET,
    TK_SEMI, TK_COMMA,
    // 特殊
    TK_EOF, TK_UNKNOWN
};

struct Token {
    TokenType type;
    string value;
    int line;
    string typeName() const {
        switch(type) {
            case TK_INT: return "INT"; case TK_FLOAT: return "FLOAT";
            case TK_VOID: return "VOID"; case TK_IF: return "IF";
            case TK_ELSE: return "ELSE"; case TK_WHILE: return "WHILE";
            case TK_RETURN: return "RETURN"; case TK_INPUT: return "INPUT";
            case TK_PRINT: return "PRINT"; case TK_ID: return "ID";
            case TK_NUM: return "NUM"; case TK_FLOAT_NUM: return "FLO";
            case TK_PLUS: return "ADD"; case TK_MINUS: return "SUB";
            case TK_MUL: return "MUL"; case TK_DIV: return "DIV";
            case TK_ASSIGN: return "ASG";
            case TK_LT: return "LSS"; case TK_GT: return "GTR";
            case TK_LE: return "LEQ"; case TK_GE: return "GEQ";
            case TK_EQ: return "EQU"; case TK_NE: return "NEQ";
            case TK_AND: return "AND"; case TK_OR: return "OR";
            case TK_NOT: return "NOT";
            case TK_LPAREN: return "LPA"; case TK_RPAREN: return "RPA";
            case TK_LBRACE: return "LBR"; case TK_RBRACE: return "RBR";
            case TK_LBRACKET: return "LBK"; case TK_RBRACKET: return "RBK";
            case TK_SEMI: return "SCO"; case TK_COMMA: return "CMA";
            case TK_EOF: return "EOF"; default: return "UNKNOWN";
        }
    }
};

// ===== 符号表 =====
struct SymbolEntry {
    string name;
    string type;     // "int", "float", "void"
    int scope;       // 作用域层级
    bool isArray;
    bool isFunc;
};

struct SymbolTable {
    vector<vector<SymbolEntry>> scopes;

    SymbolTable() { scopes.push_back(vector<SymbolEntry>()); }

    void pushScope() { scopes.push_back(vector<SymbolEntry>()); }
    void popScope() { if (scopes.size() > 1) scopes.pop_back(); }

    bool insert(const SymbolEntry& entry) {
        // 检查当前作用域是否已存在
        for (const auto& e : scopes.back()) {
            if (e.name == entry.name) return false; // 重复声明
        }
        SymbolEntry newEntry = entry;
        newEntry.scope = scopes.size() - 1;
        scopes.back().push_back(newEntry);
        return true;
    }

    SymbolEntry* lookup(const string& name) {
        for (int i = scopes.size() - 1; i >= 0; i--) {
            for (auto& e : scopes[i]) {
                if (e.name == name) return &e;
            }
        }
        return NULL;
    }

    void print() const {
        cout << "========== 符号表 ==========" << endl;
        for (size_t i = 0; i < scopes.size(); i++) {
            cout << "  作用域 " << i << ":" << endl;
            for (const auto& e : scopes[i]) {
                cout << "    " << e.name << " : " << e.type;
                if (e.isArray) cout << "[]";
                if (e.isFunc) cout << "()";
                cout << endl;
            }
        }
        cout << endl;
    }
};

// ===== AST 节点 =====
struct ASTNode {
    string kind;        // 节点类型
    string value;       // 节点值
    string dataType;    // 数据类型
    vector<ASTNode*> children;

    ASTNode(const string& k, const string& v = "") : kind(k), value(v), dataType("") {}
    ~ASTNode() { for (auto c : children) delete c; }

    void addChild(ASTNode* child) { children.push_back(child); }

    void printTree(int indent = 0) const {
        for (int i = 0; i < indent; i++) cout << "  ";
        cout << kind;
        if (!value.empty()) cout << "(" << value << ")";
        if (!dataType.empty()) cout << " :" << dataType;
        cout << endl;
        for (auto c : children) c->printTree(indent + 1);
    }
};

// ===== 词法分析器 =====
class Lexer {
    string src;
    size_t pos;
    int line;
    set<string> keywords;
public:
    vector<Token> tokens;

    Lexer(const string& source) : src(source), pos(0), line(1) {
        const char* kws[] = {"int","float","void","if","else","while","return","input","print"};
        for (int i = 0; i < 9; i++) keywords.insert(kws[i]);
        tokenize();
    }

    void tokenize() {
        while (pos < src.size()) {
            skipWhitespace();
            if (pos >= src.size()) break;

            char c = src[pos];

            // 标识符/关键字
            if (isalpha(c) || c == '_') {
                string word;
                while (pos < src.size() && (isalnum(src[pos]) || src[pos] == '_')) {
                    word += src[pos++];
                }
                if (keywords.count(word)) {
                    TokenType t = TK_ID;
                    if (word == "int") t = TK_INT;
                    else if (word == "float") t = TK_FLOAT;
                    else if (word == "void") t = TK_VOID;
                    else if (word == "if") t = TK_IF;
                    else if (word == "else") t = TK_ELSE;
                    else if (word == "while") t = TK_WHILE;
                    else if (word == "return") t = TK_RETURN;
                    else if (word == "input") t = TK_INPUT;
                    else if (word == "print") t = TK_PRINT;
                    tokens.push_back({t, word, line});
                } else {
                    tokens.push_back({TK_ID, word, line});
                }
            }
            // 数字
            else if (isdigit(c) || (c == '.' && pos+1 < src.size() && isdigit(src[pos+1]))) {
                string num;
                bool isFloat = false;
                if (c == '.') { isFloat = true; num += src[pos++]; }
                while (pos < src.size() && isdigit(src[pos])) num += src[pos++];
                if (pos < src.size() && src[pos] == '.') {
                    isFloat = true; num += src[pos++];
                    while (pos < src.size() && isdigit(src[pos])) num += src[pos++];
                }
                if (pos < src.size() && (src[pos] == 'e' || src[pos] == 'E')) {
                    isFloat = true; num += src[pos++];
                    if (pos < src.size() && (src[pos] == '+' || src[pos] == '-')) num += src[pos++];
                    while (pos < src.size() && isdigit(src[pos])) num += src[pos++];
                }
                tokens.push_back({isFloat ? TK_FLOAT_NUM : TK_NUM, num, line});
            }
            // 双字符运算符
            else if (pos + 1 < src.size()) {
                string two = src.substr(pos, 2);
                TokenType t = TK_UNKNOWN;
                if (two == "<=") t = TK_LE;
                else if (two == ">=") t = TK_GE;
                else if (two == "==") t = TK_EQ;
                else if (two == "!=") t = TK_NE;
                else if (two == "&&") t = TK_AND;
                else if (two == "||") t = TK_OR;
                if (t != TK_UNKNOWN) {
                    tokens.push_back({t, two, line}); pos += 2; continue;
                }
                // 单字符
                addSingleChar(c);
            }
            else { addSingleChar(c); }
        }
        tokens.push_back({TK_EOF, "", line});
    }

private:
    void skipWhitespace() {
        while (pos < src.size() && isspace(src[pos])) {
            if (src[pos] == '\n') line++;
            pos++;
        }
        // 跳过注释 //
        if (pos + 1 < src.size() && src[pos] == '/' && src[pos+1] == '/') {
            while (pos < src.size() && src[pos] != '\n') pos++;
            skipWhitespace();
        }
    }

    void addSingleChar(char c) {
        TokenType t = TK_UNKNOWN;
        switch(c) {
            case '+': t = TK_PLUS; break; case '-': t = TK_MINUS; break;
            case '*': t = TK_MUL; break; case '/': t = TK_DIV; break;
            case '=': t = TK_ASSIGN; break;
            case '<': t = TK_LT; break; case '>': t = TK_GT; break;
            case '!': t = TK_NOT; break;
            case '(': t = TK_LPAREN; break; case ')': t = TK_RPAREN; break;
            case '{': t = TK_LBRACE; break; case '}': t = TK_RBRACE; break;
            case '[': t = TK_LBRACKET; break; case ']': t = TK_RBRACKET; break;
            case ';': t = TK_SEMI; break; case ',': t = TK_COMMA; break;
            default: break;
        }
        tokens.push_back({t, string(1, c), line});
        pos++;
    }
};

// ===== 语法分析器 + 语义分析 =====
class Parser {
    vector<Token>& tokens;
    size_t pos;
    SymbolTable& symTable;
    vector<string>& errors;
public:
    Parser(vector<Token>& t, SymbolTable& st, vector<string>& err)
        : tokens(t), pos(0), symTable(st), errors(err) {}

    Token& peek() { return tokens[pos]; }
    Token& advance() { return tokens[pos++]; }
    bool match(TokenType t) {
        if (peek().type == t) { advance(); return true; }
        return false;
    }
    void expect(TokenType t) {
        if (!match(t)) {
            char buf[128];
            sprintf(buf, "[错误] 行%d: 期望 %s, 得到 '%s'", peek().line,
                    Token({t,"",0}).typeName().c_str(), peek().value.c_str());
            errors.push_back(buf);
        }
    }

    // P -> { D } { S }
    ASTNode* parseProgram() {
        ASTNode* root = new ASTNode("Program");
        // 解析声明
        while (peek().type == TK_INT || peek().type == TK_FLOAT || peek().type == TK_VOID) {
            // 检查是否是函数声明或变量声明
            size_t saved = pos;
            string typeStr = peek().value; advance(); // T

            if (peek().type == TK_ID) {
                string name = peek().value; advance(); // d

                if (peek().type == TK_LPAREN) {
                    // 函数声明: T d ( params ) { S_list }
                    pos = saved;
                    ASTNode* funcDecl = parseFuncDecl();
                    if (funcDecl) root->addChild(funcDecl);
                } else {
                    // 变量声明: T d ;
                    SymbolEntry entry;
                    entry.name = name; entry.type = typeStr;
                    entry.isArray = false; entry.isFunc = false;
                    if (!symTable.insert(entry)) {
                        char buf[128];
                        sprintf(buf, "[语义错误] 行%d: 变量 '%s' 重复声明", peek().line, name.c_str());
                        errors.push_back(buf);
                    }
                    ASTNode* decl = new ASTNode("VarDecl", name);
                    decl->dataType = typeStr;
                    root->addChild(decl);
                    match(TK_SEMI);
                }
            } else {
                pos = saved;
                break;
            }
        }
        // 解析语句
        while (peek().type != TK_EOF && peek().type != TK_RBRACE) {
            ASTNode* stmt = parseStatement();
            if (stmt) root->addChild(stmt);
        }
        return root;
    }

    // 函数声明
    ASTNode* parseFuncDecl() {
        string retType = peek().value; advance(); // T
        string funcName = peek().value; advance(); // d

        SymbolEntry entry;
        entry.name = funcName; entry.type = retType;
        entry.isArray = false; entry.isFunc = true;
        if (!symTable.insert(entry)) {
            char buf[128];
            sprintf(buf, "[语义错误] 行%d: 函数 '%s' 重复声明", peek().line, funcName.c_str());
            errors.push_back(buf);
        }

        ASTNode* func = new ASTNode("FuncDecl", funcName);
        func->dataType = retType;

        expect(TK_LPAREN);
        symTable.pushScope();

        // 参数列表
        ASTNode* params = new ASTNode("Params");
        while (peek().type == TK_INT || peek().type == TK_FLOAT || peek().type == TK_VOID) {
            string ptype = peek().value; advance();
            if (peek().type == TK_ID) {
                string pname = peek().value; advance();
                SymbolEntry pe;
                pe.name = pname; pe.type = ptype;
                pe.isArray = false; pe.isFunc = false;
                symTable.insert(pe);
                ASTNode* p = new ASTNode("Param", pname);
                p->dataType = ptype;
                params->addChild(p);
            }
            match(TK_SEMI) || match(TK_COMMA);
        }
        func->addChild(params);

        expect(TK_RPAREN);

        // 函数体
        if (match(TK_LBRACE)) {
            ASTNode* body = new ASTNode("Block");
            while (!match(TK_RBRACE) && peek().type != TK_EOF) {
                ASTNode* stmt = parseStatement();
                if (stmt) body->addChild(stmt);
            }
            func->addChild(body);
        }
        symTable.popScope();

        match(TK_SEMI);
        return func;
    }

    // 语句
    ASTNode* parseStatement() {
        switch (peek().type) {
            case TK_INT: case TK_FLOAT: {
                // 变量声明
                string typeStr = peek().value; advance();
                if (peek().type == TK_ID) {
                    string name = peek().value; advance();
                    SymbolEntry entry;
                    entry.name = name; entry.type = typeStr;
                    entry.isArray = false; entry.isFunc = false;
                    if (!symTable.insert(entry)) {
                        char buf[128];
                        sprintf(buf, "[语义错误] 行%d: 变量 '%s' 重复声明", peek().line, name.c_str());
                        errors.push_back(buf);
                    }
                    ASTNode* decl = new ASTNode("VarDecl", name);
                    decl->dataType = typeStr;
                    match(TK_SEMI);
                    return decl;
                }
                break;
            }
            case TK_ID: {
                string name = peek().value;
                SymbolEntry* sym = symTable.lookup(name);
                if (!sym) {
                    char buf[128];
                    sprintf(buf, "[语义错误] 行%d: 变量 '%s' 未声明", peek().line, name.c_str());
                    errors.push_back(buf);
                }
                advance();
                if (match(TK_ASSIGN)) {
                    // 赋值语句: d = E ;
                    ASTNode* assign = new ASTNode("Assign", name);
                    ASTNode* expr = parseExpr();
                    if (sym && expr && !expr->dataType.empty()) {
                        assign->dataType = sym->type;
                        // 简单类型检查
                        if (sym->type == "int" && expr->dataType == "float") {
                            char buf[128];
                            sprintf(buf, "[警告] 行%d: 变量 '%s'(int) 被赋予 float 值", tokens[pos-1].line, name.c_str());
                            errors.push_back(buf);
                        }
                    }
                    assign->addChild(new ASTNode("ID", name));
                    assign->addChild(expr);
                    match(TK_SEMI);
                    return assign;
                }
                break;
            }
            case TK_IF: {
                advance();
                ASTNode* ifNode = new ASTNode("If");
                expect(TK_LPAREN);
                ASTNode* cond = parseBoolExpr();
                ifNode->addChild(cond);
                expect(TK_RPAREN);
                symTable.pushScope();
                ifNode->addChild(parseStatement());
                symTable.popScope();
                if (match(TK_ELSE)) {
                    symTable.pushScope();
                    ifNode->addChild(parseStatement());
                    symTable.popScope();
                }
                return ifNode;
            }
            case TK_WHILE: {
                advance();
                ASTNode* whileNode = new ASTNode("While");
                expect(TK_LPAREN);
                whileNode->addChild(parseBoolExpr());
                expect(TK_RPAREN);
                symTable.pushScope();
                whileNode->addChild(parseStatement());
                symTable.popScope();
                return whileNode;
            }
            case TK_RETURN: {
                advance();
                ASTNode* ret = new ASTNode("Return");
                if (peek().type != TK_SEMI) ret->addChild(parseExpr());
                match(TK_SEMI);
                return ret;
            }
            case TK_PRINT: {
                advance();
                ASTNode* pr = new ASTNode("Print");
                pr->addChild(parseExpr());
                match(TK_SEMI);
                return pr;
            }
            case TK_INPUT: {
                advance();
                ASTNode* inp = new ASTNode("Input");
                if (peek().type == TK_ID) {
                    string name = peek().value;
                    SymbolEntry* sym = symTable.lookup(name);
                    if (!sym) {
                        char buf[128];
                        sprintf(buf, "[语义错误] 行%d: 变量 '%s' 未声明", peek().line, name.c_str());
                        errors.push_back(buf);
                    }
                    inp->addChild(new ASTNode("ID", name));
                    advance();
                }
                match(TK_SEMI);
                return inp;
            }
            case TK_LBRACE: {
                advance();
                ASTNode* block = new ASTNode("Block");
                symTable.pushScope();
                while (!match(TK_RBRACE) && peek().type != TK_EOF) {
                    ASTNode* s = parseStatement();
                    if (s) block->addChild(s);
                }
                symTable.popScope();
                return block;
            }
            case TK_SEMI: advance(); return NULL;
            default:
                advance(); // 跳过无法识别的token
                return NULL;
        }
        return NULL;
    }

    // 布尔表达式
    ASTNode* parseBoolExpr() {
        ASTNode* left = parseExpr();
        if (peek().type == TK_LT || peek().type == TK_GT ||
            peek().type == TK_LE || peek().type == TK_GE ||
            peek().type == TK_EQ || peek().type == TK_NE) {
            string op = peek().value;
            advance();
            ASTNode* right = parseExpr();
            ASTNode* cmp = new ASTNode("BinOp", op);
            cmp->addChild(left);
            cmp->addChild(right);
            return cmp;
        }
        return left;
    }

    // 表达式 E -> T { (+|-) T }
    ASTNode* parseExpr() {
        ASTNode* node = parseTerm();
        while (peek().type == TK_PLUS || peek().type == TK_MINUS) {
            string op = peek().value; advance();
            ASTNode* right = parseTerm();
            ASTNode* binop = new ASTNode("BinOp", op);
            binop->addChild(node);
            binop->addChild(right);
            node = binop;
        }
        // 推断类型
        if (node->children.size() == 2) {
            if (node->children[0]->dataType == "float" || node->children[1]->dataType == "float")
                node->dataType = "float";
            else node->dataType = "int";
        }
        return node;
    }

    // 项 T -> F { (*|/) F }
    ASTNode* parseTerm() {
        ASTNode* node = parseFactor();
        while (peek().type == TK_MUL || peek().type == TK_DIV) {
            string op = peek().value; advance();
            ASTNode* right = parseFactor();
            ASTNode* binop = new ASTNode("BinOp", op);
            binop->addChild(node);
            binop->addChild(right);
            node = binop;
        }
        if (node->children.size() == 2) {
            if (node->children[0]->dataType == "float" || node->children[1]->dataType == "float")
                node->dataType = "float";
            else node->dataType = "int";
        }
        return node;
    }

    // 因子 F -> (E) | d | i | f | -F
    ASTNode* parseFactor() {
        if (peek().type == TK_LPAREN) {
            advance();
            ASTNode* e = parseExpr();
            expect(TK_RPAREN);
            return e;
        }
        if (peek().type == TK_MINUS) {
            advance();
            ASTNode* f = parseFactor();
            ASTNode* neg = new ASTNode("UnaryOp", "-");
            neg->addChild(f);
            neg->dataType = f->dataType;
            return neg;
        }
        if (peek().type == TK_NUM) {
            string v = peek().value; advance();
            ASTNode* n = new ASTNode("IntConst", v);
            n->dataType = "int";
            return n;
        }
        if (peek().type == TK_FLOAT_NUM) {
            string v = peek().value; advance();
            ASTNode* n = new ASTNode("FloatConst", v);
            n->dataType = "float";
            return n;
        }
        if (peek().type == TK_ID) {
            string name = peek().value; advance();
            SymbolEntry* sym = symTable.lookup(name);
            ASTNode* id = new ASTNode("ID", name);
            if (sym) id->dataType = sym->type;
            return id;
        }
        // 未知，返回空节点
        advance();
        return new ASTNode("Unknown", "?");
    }
};

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    string filename;
    if (argc > 1) filename = argv[1];
    else { cout << "请输入源文件路径: "; cin >> filename; }

    ifstream fin(filename);
    if (!fin.is_open()) {
        cout << "[错误] 无法打开文件: " << filename << endl;
        return 1;
    }
    string source((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
    fin.close();

    cout << "========== 源代码 ==========" << endl;
    cout << source << endl << endl;

    // 1. 词法分析
    Lexer lexer(source);
    cout << "========== Token 流 ==========" << endl;
    for (const auto& t : lexer.tokens) {
        if (t.type == TK_EOF) break;
        cout << "<" << t.typeName() << ", " << t.value << "> ";
    }
    cout << endl << endl;

    // 2. 语法分析 + 语义分析
    SymbolTable symTable;
    vector<string> errors;
    Parser parser(lexer.tokens, symTable, errors);
    ASTNode* ast = parser.parseProgram();

    // 3. 输出AST
    cout << "========== 抽象语法树 (AST) ==========" << endl;
    if (ast) ast->printTree();
    cout << endl;

    // 4. 输出符号表
    symTable.print();

    // 5. 输出语义错误
    cout << "========== 语义检查结果 ==========" << endl;
    if (errors.empty()) {
        cout << "未检测到语义错误" << endl;
    } else {
        for (const auto& e : errors) cout << "  " << e << endl;
    }
    cout << endl;

    delete ast;
    return 0;
}
