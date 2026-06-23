// Semantic Analyzer Module

// ===== Token Types =====
const SEM_TK = {
    INT:1,FLOAT:2,VOID:3,IF:4,ELSE:5,WHILE:6,RETURN:7,INPUT:8,PRINT:9,
    ID:10,NUM:11,FLOAT_NUM:12,
    PLUS:13,MINUS:14,MUL:15,DIV:16,
    ASSIGN:17,LT:18,GT:19,LE:20,GE:21,EQ:22,NE:23,AND:24,OR:25,NOT:26,
    LPAREN:27,RPAREN:28,LBRACE:29,RBRACE:30,
    LBRACKET:31,RBRACKET:32,SEMI:33,COMMA:34,
    EOF:35,UNKNOWN:36
};

function semTokenType(t) {
    for (const k in SEM_TK) if (SEM_TK[k] === t) return k;
    return 'UNKNOWN';
}

// ===== Lexer =====
function semLex(src) {
    const tokens = [];
    const kws = {int:SEM_TK.INT,float:SEM_TK.FLOAT,void:SEM_TK.VOID,if:SEM_TK.IF,
        else:SEM_TK.ELSE,while:SEM_TK.WHILE,return:SEM_TK.RETURN,input:SEM_TK.INPUT,print:SEM_TK.PRINT};
    let p = 0, ln = 1;
    while (p < src.length) {
        while (p < src.length && /\s/.test(src[p])) { if (src[p]==='\n') ln++; p++; }
        if (p >= src.length) break;
        // comments
        if (p+1 < src.length && src[p]==='/' && src[p+1]==='/') {
            while (p < src.length && src[p]!=='\n') p++;
            continue;
        }
        let c = src[p];
        // identifier/keyword
        if (/[a-zA-Z_]/.test(c)) {
            let w = '';
            while (p < src.length && /[a-zA-Z0-9_]/.test(src[p])) w += src[p++];
            tokens.push({ type: kws[w] || SEM_TK.ID, value: w, line: ln });
            continue;
        }
        // number
        if (/\d/.test(c) || (c==='.' && p+1<src.length && /\d/.test(src[p+1]))) {
            let n = '', isFl = false;
            if (c==='.') { isFl = true; n += src[p++]; }
            while (p<src.length && /\d/.test(src[p])) n += src[p++];
            if (p<src.length && src[p]==='.') { isFl = true; n += src[p++]; while(p<src.length && /\d/.test(src[p])) n += src[p++]; }
            if (p<src.length && (src[p]==='e'||src[p]==='E')) { isFl=true; n+=src[p++]; if(p<src.length&&(src[p]==='+'||src[p]==='-')) n+=src[p++]; while(p<src.length&&/\d/.test(src[p])) n+=src[p++]; }
            tokens.push({ type: isFl ? SEM_TK.FLOAT_NUM : SEM_TK.NUM, value: n, line: ln });
            continue;
        }
        // two-char operators
        if (p+1 < src.length) {
            const tw = src.substring(p, p+2);
            const map2 = {'<=':SEM_TK.LE,'>=':SEM_TK.GE,'==':SEM_TK.EQ,'!=':SEM_TK.NE,'&&':SEM_TK.AND,'||':SEM_TK.OR};
            if (map2[tw] !== undefined) { tokens.push({type:map2[tw],value:tw,line:ln}); p+=2; continue; }
        }
        // single char
        const map1 = {'+':SEM_TK.PLUS,'-':SEM_TK.MINUS,'*':SEM_TK.MUL,'/':SEM_TK.DIV,
            '=':SEM_TK.ASSIGN,'<':SEM_TK.LT,'>':SEM_TK.GT,'!':SEM_TK.NOT,
            '(':SEM_TK.LPAREN,')':SEM_TK.RPAREN,'{':SEM_TK.LBRACE,'}':SEM_TK.RBRACE,
            '[':SEM_TK.LBRACKET,']':SEM_TK.RBRACKET,';':SEM_TK.SEMI,',':SEM_TK.COMMA};
        tokens.push({ type: map1[c] || SEM_TK.UNKNOWN, value: c, line: ln });
        p++;
    }
    tokens.push({ type: SEM_TK.EOF, value: '', line: ln });
    return tokens;
}

// ===== Symbol Table =====
function SemSymbolTable() {
    const scopes = [[]];
    const allScopes = [scopes[0]]; // 保留所有作用域（含已弹出的）
    return {
        pushScope() { const s = []; scopes.push(s); allScopes.push(s); },
        popScope() { if (scopes.length > 1) scopes.pop(); },
        insert(entry) {
            for (const e of scopes[scopes.length-1]) { if (e.name === entry.name) return false; }
            scopes[scopes.length-1].push({...entry, scope: allScopes.indexOf(scopes[scopes.length-1])});
            return true;
        },
        lookup(name) {
            for (let i = scopes.length-1; i >= 0; i--)
                for (const e of scopes[i]) if (e.name === name) return e;
            return null;
        },
        getScopes() { return allScopes; }
    };
}

// ===== AST Node =====
function SemNode(kind, value, dataType) {
    return { kind, value: value||'', dataType: dataType||'', children: [] };
}

// ===== Parser =====
function SemParser(tokens, symTable, errors) {
    let pos = 0;
    const peek = () => tokens[pos];
    const advance = () => tokens[pos++];
    const match = t => { if (peek().type === t) { advance(); return true; } return false; };
    const expect = t => {
        if (!match(t)) errors.push(`行${peek().line}: 期望${semTokenType(t)}, 得到'${peek().value}'`);
    };

    // Program
    function parseProgram() {
        const root = SemNode('Program');
        while (peek().type===SEM_TK.INT||peek().type===SEM_TK.FLOAT||peek().type===SEM_TK.VOID) {
            const saved = pos;
            const typeStr = peek().value; advance();
            if (peek().type === SEM_TK.ID) {
                const name = peek().value; advance();
                if (peek().type === SEM_TK.LPAREN) {
                    pos = saved;
                    root.children.push(parseFuncDecl());
                } else {
                    symTable.insert({name, type:typeStr, isArray:false, isFunc:false});
                    const decl = SemNode('VarDecl', name, typeStr);
                    match(SEM_TK.SEMI);
                    root.children.push(decl);
                }
            } else { pos = saved; break; }
        }
        while (peek().type !== SEM_TK.EOF && peek().type !== SEM_TK.RBRACE) {
            const stmt = parseStatement();
            if (stmt) root.children.push(stmt);
        }
        return root;
    }

    // Function
    function parseFuncDecl() {
        const retType = peek().value; advance();
        const funcName = peek().value; advance();
        if (!symTable.insert({name:funcName, type:retType, isArray:false, isFunc:true})) {
            errors.push(`[语义错误] 行${peek().line}: 函数'${funcName}'重复声明`);
        }
        const func = SemNode('FuncDecl', funcName, retType);
        expect(SEM_TK.LPAREN);
        symTable.pushScope();
        const params = SemNode('Params');
        while (peek().type===SEM_TK.INT||peek().type===SEM_TK.FLOAT||peek().type===SEM_TK.VOID) {
            const ptype = peek().value; advance();
            if (peek().type === SEM_TK.ID) {
                const pname = peek().value; advance();
                symTable.insert({name:pname, type:ptype, isArray:false, isFunc:false});
                params.children.push(SemNode('Param', pname, ptype));
            }
            match(SEM_TK.SEMI) || match(SEM_TK.COMMA);
        }
        func.children.push(params);
        expect(SEM_TK.RPAREN);
        if (match(SEM_TK.LBRACE)) {
            const body = SemNode('Block');
            while (!match(SEM_TK.RBRACE) && peek().type !== SEM_TK.EOF) {
                const stmt = parseStatement();
                if (stmt) body.children.push(stmt);
            }
            func.children.push(body);
        }
        symTable.popScope();
        match(SEM_TK.SEMI);
        return func;
    }

    // Statement
    function parseStatement() {
        const tok = peek();
        switch (tok.type) {
            case SEM_TK.INT: case SEM_TK.FLOAT: {
                const typeStr = peek().value; advance();
                if (peek().type === SEM_TK.ID) {
                    const name = peek().value; advance();
                    if (!symTable.insert({name, type:typeStr, isArray:false, isFunc:false})) {
                        errors.push(`[语义错误] 行${peek().line}: 变量'${name}'重复声明`);
                    }
                    const decl = SemNode('VarDecl', name, typeStr);
                    match(SEM_TK.SEMI);
                    return decl;
                }
                break;
            }
            case SEM_TK.ID: {
                const name = peek().value;
                if (!symTable.lookup(name)) errors.push(`[语义错误] 行${peek().line}: 变量'${name}'未声明`);
                advance();
                if (match(SEM_TK.ASSIGN)) {
                    const expr = parseExpr();
                    const assign = SemNode('Assign', name);
                    assign.children.push(SemNode('ID', name));
                    assign.children.push(expr);
                    match(SEM_TK.SEMI);
                    return assign;
                }
                break;
            }
            case SEM_TK.IF: {
                advance();
                const ifNode = SemNode('If');
                expect(SEM_TK.LPAREN);
                ifNode.children.push(parseBoolExpr());
                expect(SEM_TK.RPAREN);
                symTable.pushScope();
                ifNode.children.push(parseStatement());
                symTable.popScope();
                if (match(SEM_TK.ELSE)) {
                    symTable.pushScope();
                    ifNode.children.push(parseStatement());
                    symTable.popScope();
                }
                return ifNode;
            }
            case SEM_TK.WHILE: {
                advance();
                const whileNode = SemNode('While');
                expect(SEM_TK.LPAREN);
                whileNode.children.push(parseBoolExpr());
                expect(SEM_TK.RPAREN);
                symTable.pushScope();
                whileNode.children.push(parseStatement());
                symTable.popScope();
                return whileNode;
            }
            case SEM_TK.RETURN: {
                advance();
                const ret = SemNode('Return');
                if (peek().type !== SEM_TK.SEMI) ret.children.push(parseExpr());
                match(SEM_TK.SEMI);
                return ret;
            }
            case SEM_TK.PRINT: {
                advance();
                const pr = SemNode('Print');
                pr.children.push(parseExpr());
                match(SEM_TK.SEMI);
                return pr;
            }
            case SEM_TK.INPUT: {
                advance();
                const inp = SemNode('Input');
                if (peek().type === SEM_TK.ID) {
                    const name = peek().value;
                    if (!symTable.lookup(name)) errors.push(`[语义错误] 行${peek().line}: 变量'${name}'未声明`);
                    inp.children.push(SemNode('ID', name));
                    advance();
                }
                match(SEM_TK.SEMI);
                return inp;
            }
            case SEM_TK.LBRACE: {
                advance();
                const block = SemNode('Block');
                symTable.pushScope();
                while (!match(SEM_TK.RBRACE) && peek().type !== SEM_TK.EOF) {
                    const s = parseStatement();
                    if (s) block.children.push(s);
                }
                symTable.popScope();
                return block;
            }
            case SEM_TK.SEMI: advance(); return null;
            default: advance(); return null;
        }
        return null;
    }

    function parseBoolExpr() {
        let left = parseExpr();
        const op = peek().type;
        if (op===SEM_TK.LT||op===SEM_TK.GT||op===SEM_TK.LE||op===SEM_TK.GE||op===SEM_TK.EQ||op===SEM_TK.NE) {
            const opStr = peek().value; advance();
            const right = parseExpr();
            const cmp = SemNode('BinOp', opStr);
            cmp.children.push(left);
            cmp.children.push(right);
            return cmp;
        }
        return left;
    }

    function parseExpr() {
        let node = parseTerm();
        while (peek().type===SEM_TK.PLUS||peek().type===SEM_TK.MINUS) {
            const op = peek().value; advance();
            const right = parseTerm();
            const binop = SemNode('BinOp', op);
            binop.children.push(node);
            binop.children.push(right);
            node = binop;
        }
        if (node.children.length === 2) {
            if (node.children[0].dataType==='float'||node.children[1].dataType==='float') node.dataType='float';
            else node.dataType='int';
        }
        return node;
    }

    function parseTerm() {
        let node = parseFactor();
        while (peek().type===SEM_TK.MUL||peek().type===SEM_TK.DIV) {
            const op = peek().value; advance();
            const right = parseFactor();
            const binop = SemNode('BinOp', op);
            binop.children.push(node);
            binop.children.push(right);
            node = binop;
        }
        if (node.children.length === 2) {
            if (node.children[0].dataType==='float'||node.children[1].dataType==='float') node.dataType='float';
            else node.dataType='int';
        }
        return node;
    }

    function parseFactor() {
        if (match(SEM_TK.LPAREN)) {
            const e = parseExpr();
            expect(SEM_TK.RPAREN);
            return e;
        }
        if (peek().type === SEM_TK.MINUS) {
            advance();
            const f = parseFactor();
            const neg = SemNode('UnaryOp', '-');
            neg.children.push(f);
            neg.dataType = f.dataType;
            return neg;
        }
        if (peek().type === SEM_TK.NUM) {
            const v = peek().value; advance();
            return SemNode('IntConst', v, 'int');
        }
        if (peek().type === SEM_TK.FLOAT_NUM) {
            const v = peek().value; advance();
            return SemNode('FloatConst', v, 'float');
        }
        if (peek().type === SEM_TK.ID) {
            const name = peek().value; advance();
            const sym = symTable.lookup(name);
            return SemNode('ID', name, sym ? sym.type : '');
        }
        advance();
        return SemNode('Unknown', '?');
    }

    return { parseProgram };
}

// ===== Render AST =====
function renderAST(node, depth) {
    if (!node) return '';
    let html = '';
    for (let i = 0; i < depth; i++) html += '&nbsp;&nbsp;';
    html += '<span class="ast-node">' + node.kind;
    if (node.value) html += '<span class="ast-val">(' + node.value + ')</span>';
    if (node.dataType) html += '<span class="ast-type">:' + node.dataType + '</span>';
    html += '</span>\n';
    node.children.forEach(c => { html += renderAST(c, depth + 1); });
    return html;
}

// ===== Main Entry =====
function runSemantic() {
    const src = document.getElementById('sem-input').value;
    const tokens = semLex(src);
    const symTable = SemSymbolTable();
    const errors = [];
    const parser = SemParser(tokens, symTable, errors);
    const ast = parser.parseProgram();

    // AST
    document.getElementById('sem-ast').innerHTML = '<pre class="ast-tree">' + renderAST(ast, 0) + '</pre>';

    // Symbol table
    const scopes = symTable.getScopes();
    let stHtml = '';
    scopes.forEach((scope, i) => {
        stHtml += `<div class="scope-group"><div class="scope-title">作用域 ${i}</div>`;
        if (scope.length === 0) {
            stHtml += '<div class="scope-empty">(空)</div>';
        } else {
            stHtml += '<table class="parse-table sym-table"><tr><th>名称</th><th>类型</th><th>类别</th></tr>';
            scope.forEach(e => {
                const cat = e.isFunc ? '函数' : (e.isArray ? '数组' : '变量');
                stHtml += `<tr><td>${e.name}</td><td>${e.type}</td><td>${cat}</td></tr>`;
            });
            stHtml += '</table>';
        }
        stHtml += '</div>';
    });
    document.getElementById('sem-symtable').innerHTML = stHtml;

    // Errors
    const errDiv = document.getElementById('sem-errors');
    if (errors.length === 0) {
        errDiv.innerHTML = '<div class="p-3 bg-green-50 rounded text-green-600 text-sm">✓ 未检测到语义错误</div>';
    } else {
        errDiv.innerHTML = '<div class="p-3 bg-red-50 rounded text-red-600 text-sm">' +
            errors.map(e => `<div>${e}</div>`).join('') + '</div>';
    }
}
