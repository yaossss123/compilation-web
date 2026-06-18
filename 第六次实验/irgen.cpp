/**
 * 中间代码生成器 (Intermediate Code Generator - Quadruples)
 * 编译原理第六次实验
 *
 * 功能：
 *   1. 词法分析：将源代码转换为 Token 流
 *   2. 语法分析：递归下降解析
 *   3. 中间代码生成：生成四元式 (op, arg1, arg2, result)
 *
 * 四元式格式: (运算符, 操作数1, 操作数2, 结果)
 * 示例: (+, x, y, t1) 表示 t1 = x + y
 */

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <cstdio>
#include <sstream>

using namespace std;

// ===== Token =====
enum TT {
    T_INT, T_FLOAT, T_VOID, T_IF, T_ELSE, T_WHILE,
    T_RETURN, T_INPUT, T_PRINT,
    T_ID, T_NUM, T_FLT,
    T_PLUS, T_MINUS, T_MUL, T_DIV,
    T_ASG, T_LT, T_GT, T_LE, T_GE, T_EQ, T_NE,
    T_AND, T_OR, T_NOT,
    T_LPA, T_RPA, T_LBR, T_RBR, T_LBK, T_RBK,
    T_SEM, T_COM, T_EOF, T_UNK
};

struct Tok {
    TT type; string val; int ln;
};

// ===== 四元式 =====
struct Quad {
    string op, arg1, arg2, result;
    int id;
};

// ===== 符号表条目 =====
struct Sym {
    string name, type;
    bool isFunc;
};

// ===== 词法分析 =====
vector<Tok> lex(const string& src) {
    vector<Tok> toks;
    size_t p = 0; int ln = 1;
    set<string> kws = {"int","float","void","if","else","while","return","input","print"};

    while (p < src.size()) {
        while (p < src.size() && isspace(src[p])) { if (src[p]=='\n') ln++; p++; }
        if (p >= src.size()) break;

        // 注释
        if (p+1 < src.size() && src[p]=='/' && src[p+1]=='/') {
            while (p < src.size() && src[p]!='\n') p++;
            continue;
        }

        char c = src[p];
        if (isalpha(c) || c=='_') {
            string w;
            while (p < src.size() && (isalnum(src[p])||src[p]=='_')) w += src[p++];
            TT t = T_ID;
            if (w=="int") t=T_INT; else if (w=="float") t=T_FLOAT;
            else if (w=="void") t=T_VOID; else if (w=="if") t=T_IF;
            else if (w=="else") t=T_ELSE; else if (w=="while") t=T_WHILE;
            else if (w=="return") t=T_RETURN; else if (w=="input") t=T_INPUT;
            else if (w=="print") t=T_PRINT;
            toks.push_back({t, w, ln});
        } else if (isdigit(c) || (c=='.' && p+1<src.size() && isdigit(src[p+1]))) {
            string n; bool fl=false;
            if (c=='.') { fl=true; n+=src[p++]; }
            while (p<src.size() && isdigit(src[p])) n+=src[p++];
            if (p<src.size() && src[p]=='.') { fl=true; n+=src[p++]; while(p<src.size()&&isdigit(src[p])) n+=src[p++]; }
            if (p<src.size() && (src[p]=='e'||src[p]=='E')) { fl=true; n+=src[p++]; if(p<src.size()&&(src[p]=='+'||src[p]=='-')) n+=src[p++]; while(p<src.size()&&isdigit(src[p])) n+=src[p++]; }
            toks.push_back({fl?T_FLT:T_NUM, n, ln});
        } else if (p+1 < src.size()) {
            string tw = src.substr(p,2);
            TT t = T_UNK;
            if(tw=="<=")t=T_LE; else if(tw==">=")t=T_GE; else if(tw=="==")t=T_EQ;
            else if(tw=="!=")t=T_NE; else if(tw=="&&")t=T_AND; else if(tw=="||")t=T_OR;
            if (t!=T_UNK) { toks.push_back({t,tw,ln}); p+=2; continue; }
            // single char
            goto single;
        } else {
            single:
            TT t = T_UNK;
            switch(c) {
                case '+':t=T_PLUS;break; case '-':t=T_MINUS;break;
                case '*':t=T_MUL;break; case '/':t=T_DIV;break;
                case '=':t=T_ASG;break; case '<':t=T_LT;break; case '>':t=T_GT;break;
                case '!':t=T_NOT;break;
                case '(':t=T_LPA;break; case ')':t=T_RPA;break;
                case '{':t=T_LBR;break; case '}':t=T_RBR;break;
                case '[':t=T_LBK;break; case ']':t=T_RBK;break;
                case ';':t=T_SEM;break; case ',':t=T_COM;break;
                default:break;
            }
            toks.push_back({t,string(1,c),ln}); p++;
        }
    }
    toks.push_back({T_EOF,"",ln});
    return toks;
}

// ===== 中间代码生成器 =====
class IRGen {
    vector<Tok>& toks;
    size_t pos;
    vector<Quad> quads;
    vector<Sym> syms;
    int tmpCnt;
    int labelCnt;

    Tok& peek() { return toks[pos]; }
    Tok& advance() { return toks[pos++]; }
    bool match(TT t) { if(peek().type==t){advance();return true;} return false; }

    string newTmp() { char b[32]; sprintf(b,"t%d",tmpCnt++); return b; }
    string newLabel() { char b[32]; sprintf(b,"L%d",labelCnt++); return b; }

    void emit(const string& op, const string& a1, const string& a2, const string& res) {
        Quad q; q.op=op; q.arg1=a1; q.arg2=a2; q.result=res; q.id=quads.size();
        quads.push_back(q);
    }

public:
    IRGen(vector<Tok>& t) : toks(t), pos(0), tmpCnt(1), labelCnt(0) {}

    void parse() {
        while (peek().type != T_EOF) {
            if (peek().type==T_INT||peek().type==T_FLOAT||peek().type==T_VOID) {
                parseDeclOrFunc();
            } else {
                parseStmt();
            }
        }
    }

    void parseDeclOrFunc() {
        string typeStr = peek().val; advance();
        if (peek().type == T_ID) {
            string name = peek().val; advance();
            if (peek().type == T_LPA) {
                // 函数声明
                syms.push_back({name, typeStr, true});
                emit("FUNC", name, "", "");
                advance(); // (
                // 参数
                while (peek().type==T_INT||peek().type==T_FLOAT||peek().type==T_VOID) {
                    string pt = peek().val; advance();
                    if (peek().type==T_ID) {
                        string pn = peek().val; advance();
                        syms.push_back({pn, pt, false});
                        emit("PARAM", pn, pt, "");
                    }
                    match(T_SEM) || match(T_COM);
                }
                match(T_RPA);
                if (match(T_LBR)) {
                    while (!match(T_RBR) && peek().type!=T_EOF) parseStmt();
                }
                emit("ENDFUNC", name, "", "");
                match(T_SEM);
            } else {
                // 变量声明
                syms.push_back({name, typeStr, false});
                emit("DECL", name, typeStr, "");
                match(T_SEM);
            }
        }
    }

    void parseStmt() {
        switch (peek().type) {
            case T_INT: case T_FLOAT: {
                string typeStr = peek().val; advance();
                if (peek().type == T_ID) {
                    string name = peek().val; advance();
                    syms.push_back({name, typeStr, false});
                    emit("DECL", name, typeStr, "");
                    if (match(T_ASG)) {
                        string val = parseExpr();
                        emit("=", val, "", name);
                    }
                    match(T_SEM);
                }
                break;
            }
            case T_ID: {
                string name = peek().val; advance();
                if (match(T_ASG)) {
                    string val = parseExpr();
                    emit("=", val, "", name);
                    match(T_SEM);
                } else if (match(T_LPA)) {
                    // 函数调用
                    string args;
                    while (peek().type != T_RPA && peek().type != T_EOF) {
                        string a = parseExpr();
                        if (!args.empty()) args += ",";
                        args += a;
                        match(T_COM);
                    }
                    match(T_RPA);
                    emit("CALL", name, args, "");
                    match(T_SEM);
                }
                break;
            }
            case T_IF: {
                advance();
                match(T_LPA);
                string cond = parseCond();
                match(T_RPA);
                string lElse = newLabel();
                string lEnd = newLabel();
                emit("JZ", cond, "", lElse);
                parseStmt();
                if (match(T_ELSE)) {
                    emit("JMP", "", "", lEnd);
                    emit("LABEL", lElse, "", "");
                    parseStmt();
                    emit("LABEL", lEnd, "", "");
                } else {
                    emit("LABEL", lElse, "", "");
                }
                break;
            }
            case T_WHILE: {
                advance();
                string lStart = newLabel();
                string lEnd = newLabel();
                emit("LABEL", lStart, "", "");
                match(T_LPA);
                string cond = parseCond();
                match(T_RPA);
                emit("JZ", cond, "", lEnd);
                parseStmt();
                emit("JMP", "", "", lStart);
                emit("LABEL", lEnd, "", "");
                break;
            }
            case T_RETURN: {
                advance();
                if (peek().type != T_SEM) {
                    string val = parseExpr();
                    emit("RETURN", val, "", "");
                } else {
                    emit("RETURN", "", "", "");
                }
                match(T_SEM);
                break;
            }
            case T_PRINT: {
                advance();
                string val = parseExpr();
                emit("PRINT", val, "", "");
                match(T_SEM);
                break;
            }
            case T_INPUT: {
                advance();
                if (peek().type == T_ID) {
                    string name = peek().val; advance();
                    emit("INPUT", "", "", name);
                }
                match(T_SEM);
                break;
            }
            case T_LBR: {
                advance();
                while (!match(T_RBR) && peek().type != T_EOF) parseStmt();
                break;
            }
            case T_SEM: advance(); break;
            default: advance(); break;
        }
    }

    string parseExpr() {
        string left = parseTerm();
        while (peek().type==T_PLUS||peek().type==T_MINUS) {
            string op = peek().val; advance();
            string right = parseTerm();
            string tmp = newTmp();
            emit(op, left, right, tmp);
            left = tmp;
        }
        return left;
    }

    string parseTerm() {
        string left = parseFactor();
        while (peek().type==T_MUL||peek().type==T_DIV) {
            string op = peek().val; advance();
            string right = parseFactor();
            string tmp = newTmp();
            emit(op, left, right, tmp);
            left = tmp;
        }
        return left;
    }

    string parseFactor() {
        if (match(T_LPA)) {
            string e = parseExpr();
            match(T_RPA);
            return e;
        }
        if (peek().type == T_MINUS) {
            advance();
            string f = parseFactor();
            string tmp = newTmp();
            emit("NEG", f, "", tmp);
            return tmp;
        }
        if (peek().type==T_NUM||peek().type==T_FLT) {
            string v = peek().val; advance();
            return v;
        }
        if (peek().type == T_ID) {
            string name = peek().val; advance();
            if (match(T_LPA)) {
                // 函数调用表达式
                string args;
                while (peek().type != T_RPA && peek().type != T_EOF) {
                    string a = parseExpr();
                    if (!args.empty()) args += ",";
                    args += a;
                    match(T_COM);
                }
                match(T_RPA);
                string tmp = newTmp();
                emit("CALL", name, args, tmp);
                return tmp;
            }
            return name;
        }
        advance();
        return "err";
    }

    string parseCond() {
        string left = parseExpr();
        if (peek().type==T_LT||peek().type==T_GT||peek().type==T_LE||
            peek().type==T_GE||peek().type==T_EQ||peek().type==T_NE) {
            string op = peek().val; advance();
            string right = parseExpr();
            string tmp = newTmp();
            emit(op, left, right, tmp);
            return tmp;
        }
        return left;
    }

    void printQuads() {
        cout << "========== 四元式中间代码 ==========" << endl;
        cout << "编号\t( 运算符, 操作数1, 操作数2, 结果 )" << endl;
        cout << "----\t---------------------------------" << endl;
        for (const auto& q : quads) {
            printf("%03d\t( %-8s, %-10s, %-10s, %-10s )\n",
                   q.id, q.op.c_str(),
                   q.arg1.c_str(), q.arg2.c_str(), q.result.c_str());
        }
        cout << endl;
        cout << "共 " << quads.size() << " 条四元式" << endl;
    }

    void printSymbolTable() {
        cout << "========== 符号表 ==========" << endl;
        for (const auto& s : syms) {
            cout << "  " << s.name << " : " << s.type;
            if (s.isFunc) cout << " (函数)";
            cout << endl;
        }
        cout << endl;
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
    vector<Tok> tokens = lex(source);
    cout << "========== Token 流 ==========" << endl;
    for (const auto& t : tokens) {
        if (t.type == T_EOF) break;
        cout << "<" << t.val << "> ";
    }
    cout << endl << endl;

    // 2. 语法分析 + 中间代码生成
    IRGen irgen(tokens);
    irgen.parse();

    // 3. 输出
    irgen.printSymbolTable();
    irgen.printQuads();

    return 0;
}
