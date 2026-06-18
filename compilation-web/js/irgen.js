// Intermediate Code Generator Module (Quadruples)

// Token types (shared with semantic module)
const IR_TK = {
    INT:1,FLOAT:2,VOID:3,IF:4,ELSE:5,WHILE:6,RETURN:7,INPUT:8,PRINT:9,
    ID:10,NUM:11,FLT:12,
    PLUS:13,MINUS:14,MUL:15,DIV:16,
    ASG:17,LT:18,GT:19,LE:20,GE:21,EQ:22,NE:23,AND:24,OR:25,NOT:26,
    LPA:27,RPA:28,LBR:29,RBR:30,LBK:31,RBK:32,SEM:33,COM:34,
    EOF:35,UNK:36
};

// ===== Lexer =====
function irLex(src) {
    const tokens = [];
    const kws = {int:IR_TK.INT,float:IR_TK.FLOAT,void:IR_TK.VOID,if:IR_TK.IF,
        else:IR_TK.ELSE,while:IR_TK.WHILE,return:IR_TK.RETURN,input:IR_TK.INPUT,print:IR_TK.PRINT};
    let p = 0, ln = 1;
    while (p < src.length) {
        while (p < src.length && /\s/.test(src[p])) { if (src[p]==='\n') ln++; p++; }
        if (p >= src.length) break;
        if (p+1 < src.length && src[p]==='/' && src[p+1]==='/') {
            while (p < src.length && src[p]!=='\n') p++;
            continue;
        }
        let c = src[p];
        if (/[a-zA-Z_]/.test(c)) {
            let w = '';
            while (p < src.length && /[a-zA-Z0-9_]/.test(src[p])) w += src[p++];
            tokens.push({ type: kws[w] || IR_TK.ID, val: w, ln });
            continue;
        }
        if (/\d/.test(c) || (c==='.' && p+1<src.length && /\d/.test(src[p+1]))) {
            let n = '', fl = false;
            if (c==='.') { fl = true; n += src[p++]; }
            while (p<src.length && /\d/.test(src[p])) n += src[p++];
            if (p<src.length && src[p]==='.') { fl=true; n+=src[p++]; while(p<src.length&&/\d/.test(src[p])) n+=src[p++]; }
            if (p<src.length && (src[p]==='e'||src[p]==='E')) { fl=true; n+=src[p++]; if(p<src.length&&(src[p]==='+'||src[p]==='-')) n+=src[p++]; while(p<src.length&&/\d/.test(src[p])) n+=src[p++]; }
            tokens.push({ type: fl ? IR_TK.FLT : IR_TK.NUM, val: n, ln });
            continue;
        }
        if (p+1 < src.length) {
            const tw = src.substring(p, p+2);
            const m = {'<=':IR_TK.LE,'>=':IR_TK.GE,'==':IR_TK.EQ,'!=':IR_TK.NE,'&&':IR_TK.AND,'||':IR_TK.OR};
            if (m[tw] !== undefined) { tokens.push({type:m[tw],val:tw,ln}); p+=2; continue; }
        }
        const m1 = {'+':IR_TK.PLUS,'-':IR_TK.MINUS,'*':IR_TK.MUL,'/':IR_TK.DIV,
            '=':IR_TK.ASG,'<':IR_TK.LT,'>':IR_TK.GT,'!':IR_TK.NOT,
            '(':IR_TK.LPA,')':IR_TK.RPA,'{':IR_TK.LBR,'}':IR_TK.RBR,
            '[':IR_TK.LBK,']':IR_TK.RBK,';':IR_TK.SEM,',':IR_TK.COM};
        tokens.push({ type: m1[c] || IR_TK.UNK, val: c, ln });
        p++;
    }
    tokens.push({ type: IR_TK.EOF, val: '', ln });
    return tokens;
}

// ===== IR Generator =====
function IRGenerator(tokens) {
    let pos = 0;
    const quads = [];
    const syms = [];
    let tmpCnt = 1, labelCnt = 0;

    const peek = () => tokens[pos];
    const advance = () => tokens[pos++];
    const match = t => { if (peek().type===t) { advance(); return true; } return false; };
    const newTmp = () => 't' + (tmpCnt++);
    const newLabel = () => 'L' + (labelCnt++);
    const emit = (op, a1, a2, res) => quads.push({id:quads.length, op, arg1:a1||'', arg2:a2||'', result:res||''});

    function parseDeclOrFunc() {
        const typeStr = peek().val; advance();
        if (peek().type === IR_TK.ID) {
            const name = peek().val; advance();
            if (peek().type === IR_TK.LPA) {
                // Function
                syms.push({name, type:typeStr, isFunc:true});
                emit('FUNC', name, '', '');
                advance(); // (
                while (peek().type===IR_TK.INT||peek().type===IR_TK.FLOAT||peek().type===IR_TK.VOID) {
                    const pt = peek().val; advance();
                    if (peek().type === IR_TK.ID) {
                        const pn = peek().val; advance();
                        syms.push({name:pn, type:pt, isFunc:false});
                        emit('PARAM', pn, pt, '');
                    }
                    match(IR_TK.SEM) || match(IR_TK.COM);
                }
                match(IR_TK.RPA);
                if (match(IR_TK.LBR)) {
                    while (!match(IR_TK.RBR) && peek().type !== IR_TK.EOF) parseStmt();
                }
                emit('ENDFUNC', name, '', '');
                match(IR_TK.SEM);
            } else {
                // Variable
                syms.push({name, type:typeStr, isFunc:false});
                emit('DECL', name, typeStr, '');
                match(IR_TK.SEM);
            }
        }
    }

    function parseStmt() {
        switch (peek().type) {
            case IR_TK.INT: case IR_TK.FLOAT: {
                const typeStr = peek().val; advance();
                if (peek().type === IR_TK.ID) {
                    const name = peek().val; advance();
                    syms.push({name, type:typeStr, isFunc:false});
                    emit('DECL', name, typeStr, '');
                    if (match(IR_TK.ASG)) {
                        const val = parseExpr();
                        emit('=', val, '', name);
                    }
                    match(IR_TK.SEM);
                }
                break;
            }
            case IR_TK.ID: {
                const name = peek().val; advance();
                if (match(IR_TK.ASG)) {
                    const val = parseExpr();
                    emit('=', val, '', name);
                    match(IR_TK.SEM);
                } else if (match(IR_TK.LPA)) {
                    let args = '';
                    while (peek().type !== IR_TK.RPA && peek().type !== IR_TK.EOF) {
                        const a = parseExpr();
                        if (args) args += ',';
                        args += a;
                        match(IR_TK.COM);
                    }
                    match(IR_TK.RPA);
                    emit('CALL', name, args, '');
                    match(IR_TK.SEM);
                }
                break;
            }
            case IR_TK.IF: {
                advance();
                match(IR_TK.LPA);
                const cond = parseCond();
                match(IR_TK.RPA);
                const lElse = newLabel();
                const lEnd = newLabel();
                emit('JZ', cond, '', lElse);
                parseStmt();
                if (match(IR_TK.ELSE)) {
                    emit('JMP', '', '', lEnd);
                    emit('LABEL', lElse, '', '');
                    parseStmt();
                    emit('LABEL', lEnd, '', '');
                } else {
                    emit('LABEL', lElse, '', '');
                }
                break;
            }
            case IR_TK.WHILE: {
                advance();
                const lStart = newLabel();
                const lEnd = newLabel();
                emit('LABEL', lStart, '', '');
                match(IR_TK.LPA);
                const cond = parseCond();
                match(IR_TK.RPA);
                emit('JZ', cond, '', lEnd);
                parseStmt();
                emit('JMP', '', '', lStart);
                emit('LABEL', lEnd, '', '');
                break;
            }
            case IR_TK.RETURN: {
                advance();
                if (peek().type !== IR_TK.SEM) {
                    const val = parseExpr();
                    emit('RETURN', val, '', '');
                } else {
                    emit('RETURN', '', '', '');
                }
                match(IR_TK.SEM);
                break;
            }
            case IR_TK.PRINT: {
                advance();
                const val = parseExpr();
                emit('PRINT', val, '', '');
                match(IR_TK.SEM);
                break;
            }
            case IR_TK.INPUT: {
                advance();
                if (peek().type === IR_TK.ID) {
                    const name = peek().val; advance();
                    emit('INPUT', '', '', name);
                }
                match(IR_TK.SEM);
                break;
            }
            case IR_TK.LBR: {
                advance();
                while (!match(IR_TK.RBR) && peek().type !== IR_TK.EOF) parseStmt();
                break;
            }
            case IR_TK.SEM: advance(); break;
            default: advance(); break;
        }
    }

    function parseExpr() {
        let left = parseTerm();
        while (peek().type===IR_TK.PLUS||peek().type===IR_TK.MINUS) {
            const op = peek().val; advance();
            const right = parseTerm();
            const tmp = newTmp();
            emit(op, left, right, tmp);
            left = tmp;
        }
        return left;
    }

    function parseTerm() {
        let left = parseFactor();
        while (peek().type===IR_TK.MUL||peek().type===IR_TK.DIV) {
            const op = peek().val; advance();
            const right = parseFactor();
            const tmp = newTmp();
            emit(op, left, right, tmp);
            left = tmp;
        }
        return left;
    }

    function parseFactor() {
        if (match(IR_TK.LPA)) {
            const e = parseExpr();
            match(IR_TK.RPA);
            return e;
        }
        if (peek().type === IR_TK.MINUS) {
            advance();
            const f = parseFactor();
            const tmp = newTmp();
            emit('NEG', f, '', tmp);
            return tmp;
        }
        if (peek().type===IR_TK.NUM||peek().type===IR_TK.FLT) {
            const v = peek().val; advance();
            return v;
        }
        if (peek().type === IR_TK.ID) {
            const name = peek().val; advance();
            if (match(IR_TK.LPA)) {
                let args = '';
                while (peek().type !== IR_TK.RPA && peek().type !== IR_TK.EOF) {
                    const a = parseExpr();
                    if (args) args += ',';
                    args += a;
                    match(IR_TK.COM);
                }
                match(IR_TK.RPA);
                const tmp = newTmp();
                emit('CALL', name, args, tmp);
                return tmp;
            }
            return name;
        }
        advance();
        return 'err';
    }

    function parseCond() {
        const left = parseExpr();
        const t = peek().type;
        if (t===IR_TK.LT||t===IR_TK.GT||t===IR_TK.LE||t===IR_TK.GE||t===IR_TK.EQ||t===IR_TK.NE) {
            const op = peek().val; advance();
            const right = parseExpr();
            const tmp = newTmp();
            emit(op, left, right, tmp);
            return tmp;
        }
        return left;
    }

    function parse() {
        while (peek().type !== IR_TK.EOF) {
            if (peek().type===IR_TK.INT||peek().type===IR_TK.FLOAT||peek().type===IR_TK.VOID) {
                parseDeclOrFunc();
            } else {
                parseStmt();
            }
        }
    }

    return { parse, quads: () => quads, syms: () => syms };
}

// ===== Main Entry =====
function runIRGen() {
    const src = document.getElementById('irgen-input').value;
    const tokens = irLex(src);
    const gen = IRGenerator(tokens);
    gen.parse();

    const quads = gen.quads();
    const syms = gen.syms();

    // Token stream
    let tokHtml = '<div class="token-stream">';
    tokens.forEach(t => {
        if (t.type === IR_TK.EOF) return;
        tokHtml += `<span class="token-tag">&lt;${t.val}&gt;</span> `;
    });
    tokHtml += '</div>';
    document.getElementById('irgen-tokens').innerHTML = tokHtml;

    // Symbol table
    let stHtml = '<table class="parse-table sym-table"><tr><th>名称</th><th>类型</th><th>类别</th></tr>';
    syms.forEach(s => {
        stHtml += `<tr><td>${s.name}</td><td>${s.type}</td><td>${s.isFunc ? '函数' : '变量'}</td></tr>`;
    });
    stHtml += '</table>';
    document.getElementById('irgen-symtable').innerHTML = stHtml;

    // Quadruples
    let qHtml = '<div class="quad-header"><code>编号 &nbsp; ( 运算符, 操作数1, 操作数2, 结果 )</code></div>';
    qHtml += '<table class="parse-table quad-table"><tr><th>#</th><th>运算符</th><th>操作数1</th><th>操作数2</th><th>结果</th></tr>';
    quads.forEach(q => {
        let cls = '';
        if (q.op==='LABEL') cls = 'quad-label';
        else if (q.op==='JZ'||q.op==='JMP') cls = 'quad-jump';
        else if (q.op==='FUNC'||q.op==='ENDFUNC') cls = 'quad-func';
        else if (q.op==='RETURN'||q.op==='PRINT'||q.op==='INPUT'||q.op==='DECL'||q.op==='PARAM'||q.op==='CALL') cls = 'quad-special';
        qHtml += `<tr class="${cls}"><td>${String(q.id).padStart(3,'0')}</td><td><strong>${q.op}</strong></td><td>${q.arg1}</td><td>${q.arg2}</td><td>${q.result}</td></tr>`;
    });
    qHtml += '</table>';
    qHtml += `<div class="mt-2 text-gray-500 text-xs">共 ${quads.length} 条四元式</div>`;
    document.getElementById('irgen-quads').innerHTML = qHtml;
}
