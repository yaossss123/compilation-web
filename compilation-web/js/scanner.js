/**
 * scanner.js - 词法分析扫描器（实验二）
 * 与 scanner.cpp 逻辑一致：逐字符扫描，识别 Token
 */

const SCANNER_KEYWORDS = new Set([
    'int', 'float', 'void', 'if', 'else', 'while', 'return', 'input', 'print'
]);

function getKeywordType(word) {
    const map = {
        'int': 'INT', 'float': 'FLOAT', 'void': 'VOID',
        'if': 'IF', 'else': 'ELSE', 'while': 'WHILE',
        'return': 'RETURN', 'input': 'INPUT', 'print': 'PRINT'
    };
    return map[word] || '';
}

/**
 * 扫描一行代码，返回 Token 数组
 * 与 scanner.cpp scanLine() 逻辑一致
 */
function scannerScanLine(code, lineNum) {
    const tokens = [];
    let i = 0;
    let len = code.length;

    // 跳过 // 注释
    const cpos = code.indexOf('//');
    if (cpos !== -1) len = cpos;

    while (i < len) {
        const c = code[i];
        // 跳过空白
        if (/\s/.test(c)) { i++; continue; }

        // 标识符 / 关键字
        if (/[a-zA-Z]/.test(c)) {
            let word = '';
            while (i < len && /[a-zA-Z0-9]/.test(code[i])) { word += code[i]; i++; }
            if (SCANNER_KEYWORDS.has(word)) {
                tokens.push({ type: getKeywordType(word), value: word, line: lineNum });
            } else {
                tokens.push({ type: 'ID', value: word, line: lineNum });
            }
            continue;
        }

        // 数字 / 浮点数（含科学计数法）
        if (/[0-9]/.test(c) || c === '.') {
            let num = '';
            let hasDot = (c === '.');
            if (c === '.') {
                num += code[i]; i++;
                while (i < len && /[0-9]/.test(code[i])) { num += code[i]; i++; }
            } else {
                while (i < len && /[0-9]/.test(code[i])) { num += code[i]; i++; }
                if (i < len && code[i] === '.') {
                    hasDot = true; num += code[i]; i++;
                    while (i < len && /[0-9]/.test(code[i])) { num += code[i]; i++; }
                }
            }
            // 科学计数法
            if (i < len && (code[i] === 'e' || code[i] === 'E')) {
                hasDot = true; num += code[i]; i++;
                if (i < len && (code[i] === '+' || code[i] === '-')) { num += code[i]; i++; }
                while (i < len && /[0-9]/.test(code[i])) { num += code[i]; i++; }
            }
            tokens.push({ type: hasDot ? 'FLOAT' : 'NUM', value: num, line: lineNum });
            continue;
        }

        // 双字符运算符（先判断双字符，再判断单字符）
        if (i + 1 < len) {
            const two = code.substring(i, i + 2);
            const twoMap = { '<=': 'LEQ', '>=': 'GEQ', '==': 'EQU', '!=': 'NEQ', '&&': 'AND', '||': 'OR' };
            if (twoMap[two]) {
                tokens.push({ type: twoMap[two], value: two, line: lineNum });
                i += 2; continue;
            }
        }

        // 单字符运算符和界符
        const singleMap = {
            '+': 'ADD', '-': 'SUB', '*': 'MUL', '/': 'DIV',
            '=': 'ASG', '<': 'LSS', '>': 'GTR', '!': 'NOT',
            '(': 'LPAR', ')': 'RPAR', '[': 'LBK', ']': 'RBK',
            '{': 'LBR', '}': 'RBR', ',': 'CMA', ';': 'SEMI'
        };
        if (singleMap[c]) {
            tokens.push({ type: singleMap[c], value: c, line: lineNum });
            i++;
        } else {
            i++; // 跳过无法识别字符
        }
    }
    return tokens;
}

/**
 * 扫描完整源代码（多行）
 */
function scannerScanAll(code) {
    const lines = code.split('\n');
    const allTokens = [];
    const lineTokens = [];
    for (let i = 0; i < lines.length; i++) {
        const tokens = scannerScanLine(lines[i], i + 1);
        lineTokens.push({ lineNum: i + 1, tokens: tokens });
        allTokens.push(...tokens);
    }
    return { allTokens, lineTokens };
}

// ========== UI 渲染 ==========

const TOKEN_COLORS = {
    // 关键字
    INT: '#3b82f6', FLOAT: '#3b82f6', VOID: '#3b82f6',
    IF: '#3b82f6', ELSE: '#3b82f6', WHILE: '#3b82f6',
    RETURN: '#3b82f6', INPUT: '#3b82f6', PRINT: '#3b82f6',
    // 标识符
    ID: '#10b981',
    // 数字
    NUM: '#f59e0b',
    // 运算符
    ADD: '#ef4444', SUB: '#ef4444', MUL: '#ef4444', DIV: '#ef4444',
    ASG: '#ef4444', LSS: '#ef4444', GTR: '#ef4444',
    LEQ: '#ef4444', GEQ: '#ef4444', EQU: '#ef4444', NEQ: '#ef4444',
    AND: '#ef4444', OR: '#ef4444', NOT: '#ef4444',
    // 界符
    LPAR: '#8b5cf6', RPAR: '#8b5cf6',
    LBK: '#8b5cf6', RBK: '#8b5cf6',
    LBR: '#8b5cf6', RBR: '#8b5cf6',
    CMA: '#8b5cf6', SEMI: '#8b5cf6'
};

function renderScanner() {
    const code = document.getElementById('scan-input').value;
    const outputEl = document.getElementById('scan-output');
    const statsEl = document.getElementById('scan-stats');

    if (!code.trim()) {
        outputEl.innerHTML = '<p class="text-gray-400 text-sm">请输入源代码</p>';
        statsEl.innerHTML = '<p class="text-gray-400 text-sm">请输入源代码</p>';
        return;
    }

    const { allTokens, lineTokens } = scannerScanAll(code);

    // 渲染 Token 流（按行分组）
    let html = '<table style="width:100%;border-collapse:collapse;font-size:0.85rem">';
    html += '<tr style="border-bottom:2px solid #334155">';
    html += '<th style="text-align:left;padding:4px 8px;color:#94a3b8;width:60px">行</th>';
    html += '<th style="text-align:left;padding:4px 8px;color:#94a3b8">Token 流</th>';
    html += '</tr>';

    for (const lt of lineTokens) {
        html += '<tr style="border-bottom:1px solid #1e293b">';
        html += '<td style="padding:4px 8px;color:#64748b;vertical-align:top">' + lt.lineNum + '</td>';
        html += '<td style="padding:4px 8px">';
        if (lt.tokens.length === 0) {
            html += '<span style="color:#475569;font-style:italic">(空行或注释)</span>';
        } else {
            for (const t of lt.tokens) {
                const color = TOKEN_COLORS[t.type] || '#94a3b8';
                html += '<span class="token-tag" style="background:' + color + '22;border:1px solid ' + color + ';color:' + color + '" title="' + t.type + ': ' + escHtml(t.value) + '">';
                html += '<strong>' + escHtml(t.value) + '</strong>';
                html += '<small>' + t.type + '</small>';
                html += '</span> ';
            }
        }
        html += '</td></tr>';
    }
    html += '</table>';
    outputEl.innerHTML = html;

    // 统计信息
    const stats = { keywords: 0, identifiers: 0, numbers: 0, operators: 0, delimiters: 0 };
    for (const t of allTokens) {
        if (['INT','FLOAT','VOID','IF','ELSE','WHILE','RETURN','INPUT','PRINT'].includes(t.type)) stats.keywords++;
        else if (t.type === 'ID') stats.identifiers++;
        else if (['NUM','FLOAT'].includes(t.type)) stats.numbers++;
        else if (['ADD','SUB','MUL','DIV','ASG','LSS','GTR','LEQ','GEQ','EQU','NEQ','AND','OR','NOT'].includes(t.type)) stats.operators++;
        else if (['LPAR','RPAR','LBK','RBK','LBR','RBR','CMA','SEMI'].includes(t.type)) stats.delimiters++;
    }

    let sHtml = '<div style="display:flex;gap:16px;flex-wrap:wrap">';
    sHtml += statBox('总 Token 数', allTokens.length, '#e2e8f0');
    sHtml += statBox('关键字', stats.keywords, '#3b82f6');
    sHtml += statBox('标识符', stats.identifiers, '#10b981');
    sHtml += statBox('数字', stats.numbers, '#f59e0b');
    sHtml += statBox('运算符', stats.operators, '#ef4444');
    sHtml += statBox('界符', stats.delimiters, '#8b5cf6');
    sHtml += '</div>';

    // 各类别详细列表
    sHtml += '<div style="margin-top:12px;font-size:0.8rem;color:#94a3b8">';
    const idSet = new Set(allTokens.filter(t => t.type === 'ID').map(t => t.value));
    const numSet = new Set(allTokens.filter(t => t.type === 'NUM' || t.type === 'FLOAT').map(t => t.value));
    if (idSet.size > 0) sHtml += '<div><strong>标识符:</strong> ' + [...idSet].join(', ') + '</div>';
    if (numSet.size > 0) sHtml += '<div><strong>数字:</strong> ' + [...numSet].join(', ') + '</div>';
    sHtml += '</div>';

    statsEl.innerHTML = sHtml;
}

function statBox(label, count, color) {
    return '<div style="background:#1e293b;border-radius:8px;padding:8px 16px;text-align:center;min-width:80px">' +
        '<div style="font-size:1.4rem;font-weight:bold;color:' + color + '">' + count + '</div>' +
        '<div style="font-size:0.75rem;color:#94a3b8">' + label + '</div></div>';
}

function escHtml(s) {
    return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

// 文件导入
function loadScannerFile(event) {
    var file = event.target.files[0];
    if (!file) return;
    var reader = new FileReader();
    reader.onload = function(e) {
        document.getElementById('scan-input').value = e.target.result;
        document.getElementById('scan-file-name').textContent = file.name;
        document.getElementById('scan-file-name').style.color = '#10b981';
    };
    reader.readAsText(file);
}

// 入口函数（供 index.html onclick 调用）
function runScanner() {
    renderScanner();
}

