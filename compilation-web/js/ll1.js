// LL(1) Parser Module

let ll1Data = null;

function loadLL1File(event) {
    const file = event.target.files[0];
    if (!file) return;
    readFileAsText(file, function(text) {
        document.getElementById('ll1-input').value = text;
    });
}

function parseLL1Grammar(text) {
    const lines = text.trim().split('\n').map(l => l.trim()).filter(l => l.length > 0);
    const grammar = {};
    const nonTerminals = [];
    lines.forEach(line => {
        const parts = line.split('->');
        if (parts.length < 2) return;
        const lhs = parts[0].trim();
        const rhs = parts[1].trim().split('|').map(s => s.trim());
        grammar[lhs] = rhs;
        if (!nonTerminals.includes(lhs)) nonTerminals.push(lhs);
    });
    return { grammar, nonTerminals };
}

function getSymbols(grammar) {
    const nonTerminals = new Set(Object.keys(grammar));
    const terminals = new Set();
    Object.values(grammar).forEach(prods => {
        prods.forEach(prod => {
            prod.split(/\s+/).forEach(sym => {
                if (sym !== '$' && !nonTerminals.has(sym)) terminals.add(sym);
            });
        });
    });
    return { nonTerminals: [...nonTerminals], terminals: [...terminals] };
}

function computeFirst(grammar, nonTerminals) {
    const first = {};
    nonTerminals.forEach(nt => first[nt] = new Set());
    let changed = true;
    while (changed) {
        changed = false;
        nonTerminals.forEach(nt => {
            grammar[nt].forEach(prod => {
                const syms = prod.split(/\s+/);
                for (const sym of syms) {
                    if (sym === '$') {
                        if (!first[nt].has('$')) { first[nt].add('$'); changed = true; }
                        break;
                    }
                    if (!nonTerminals.includes(sym)) {
                        if (!first[nt].has(sym)) { first[nt].add(sym); changed = true; }
                        break;
                    }
                    for (const f of first[sym]) {
                        if (f !== '$') { if (!first[nt].has(f)) { first[nt].add(f); changed = true; } }
                    }
                    if (!first[sym].has('$')) break;
                }
            });
        });
    }
    return first;
}

function computeFollow(grammar, nonTerminals, first, startSymbol) {
    const follow = {};
    nonTerminals.forEach(nt => follow[nt] = new Set());
    follow[startSymbol].add('#');
    let changed = true;
    while (changed) {
        changed = false;
        nonTerminals.forEach(nt => {
            grammar[nt].forEach(prod => {
                const syms = prod.split(/\s+/);
                for (let i = 0; i < syms.length; i++) {
                    const sym = syms[i];
                    if (!nonTerminals.includes(sym)) continue;
                    let restEpsilon = true;
                    for (let j = i + 1; j < syms.length; j++) {
                        const next = syms[j];
                        if (!nonTerminals.includes(next)) {
                            if (!follow[sym].has(next)) { follow[sym].add(next); changed = true; }
                            restEpsilon = false; break;
                        }
                        for (const f of first[next]) {
                            if (f !== '$') { if (!follow[sym].has(f)) { follow[sym].add(f); changed = true; } }
                        }
                        if (!first[next].has('$')) { restEpsilon = false; break; }
                    }
                    if (restEpsilon || i === syms.length - 1) {
                        for (const f of follow[nt]) {
                            if (!follow[sym].has(f)) { follow[sym].add(f); changed = true; }
                        }
                    }
                }
            });
        });
    }
    return follow;
}

function buildParseTable(grammar, nonTerminals, first, follow) {
    const { terminals } = getSymbols(grammar);
    const table = {};
    const conflicts = [];
    nonTerminals.forEach(nt => {
        table[nt] = {};
        grammar[nt].forEach(prod => {
            const syms = prod.split(/\s+/);
            const firstOfProd = new Set();
            let allEpsilon = true;
            for (const sym of syms) {
                if (sym === '$') { firstOfProd.add('$'); break; }
                if (!nonTerminals.includes(sym)) { firstOfProd.add(sym); allEpsilon = false; break; }
                for (const f of first[sym]) { if (f !== '$') firstOfProd.add(f); }
                if (!first[sym].has('$')) { allEpsilon = false; break; }
            }
            if (allEpsilon) firstOfProd.add('$');
            firstOfProd.forEach(a => {
                if (a === '$') {
                    follow[nt].forEach(b => {
                        if (table[nt][b]) conflicts.push(`[${nt}, ${b}]`);
                        table[nt][b] = prod;
                    });
                } else {
                    if (table[nt][a]) conflicts.push(`[${nt}, ${a}]`);
                    table[nt][a] = prod;
                }
            });
        });
    });
    return { table, terminals: [...terminals, '#'], conflicts };
}

function ll1Parse(grammar, nonTerminals, table, inputStr, startSymbol) {
    const tokens = inputStr.trim().split(/\s+/).concat(['#']);
    const stack = ['#', startSymbol];
    const steps = [];
    let stepNum = 0;
    while (stack.length > 0) {
        stepNum++;
        if (stepNum > 100) { steps.push({ num: stepNum, stack: '...', input: '...', action: '超出限制' }); break; }
        const top = stack[stack.length - 1];
        const curToken = tokens[0];
        const stackStr = stack.slice().reverse().join(' ');
        const inputStr2 = tokens.join(' ');
        if (top === '#') {
            if (curToken === '#') { steps.push({ num: stepNum, stack: stackStr, input: inputStr2, action: '接受 ✓' }); break; }
            else { steps.push({ num: stepNum, stack: stackStr, input: inputStr2, action: '错误' }); break; }
        }
        if (!nonTerminals.includes(top)) {
            if (top === curToken) { stack.pop(); tokens.shift(); steps.push({ num: stepNum, stack: stackStr, input: inputStr2, action: `匹配 ${top}` }); }
            else { steps.push({ num: stepNum, stack: stackStr, input: inputStr2, action: `错误：期望${top}，得到${curToken}` }); break; }
        } else {
            const entry = table[top] ? table[top][curToken] : undefined;
            if (!entry) { steps.push({ num: stepNum, stack: stackStr, input: inputStr2, action: `M[${top},${curToken}]为空` }); break; }
            stack.pop();
            if (entry !== '$') { entry.split(/\s+/).reverse().forEach(s => stack.push(s)); }
            steps.push({ num: stepNum, stack: stackStr, input: inputStr2, action: `${top} → ${entry}` });
        }
    }
    return steps;
}

function runLL1() {
    const text = document.getElementById('ll1-input').value;
    const { grammar, nonTerminals } = parseLL1Grammar(text);
    if (nonTerminals.length === 0) { alert('文法格式错误'); return; }

    const first = computeFirst(grammar, nonTerminals);
    const follow = computeFollow(grammar, nonTerminals, first, nonTerminals[0]);
    const { table, terminals, conflicts } = buildParseTable(grammar, nonTerminals, first, follow);
    ll1Data = { grammar, nonTerminals, table, terminals };

    let html = '<table class="parse-table"><tr><th></th>';
    terminals.forEach(t => html += `<th>${t}</th>`);
    html += '</tr>';
    nonTerminals.forEach(nt => {
        html += `<tr><th>${nt}</th>`;
        terminals.forEach(t => {
            const val = table[nt] && table[nt][t] ? `${nt} → ${table[nt][t]}` : '';
            html += `<td>${val}</td>`;
        });
        html += '</tr>';
    });
    html += '</table>';
    if (conflicts.length > 0) {
        html += `<p class="text-red-500 text-xs mt-2">冲突: ${conflicts.join(', ')} — 不是LL(1)文法</p>`;
    }
    document.getElementById('ll1-table').innerHTML = html;
}

function runLL1Parse() {
    if (!ll1Data) { alert('请先构造分析表'); return; }
    const inputStr = document.getElementById('ll1-string').value;
    if (!inputStr) return;
    const steps = ll1Parse(ll1Data.grammar, ll1Data.nonTerminals, ll1Data.table, inputStr, ll1Data.nonTerminals[0]);
    const div = document.getElementById('ll1-derivation');
    div.innerHTML = steps.map(s =>
        `<div class="step-row"><span class="step-num">${s.num}</span><span class="stack">[${s.stack}]</span> <span class="input-s">${s.input}</span> <span class="action">${s.action}</span></div>`
    ).join('');
}
