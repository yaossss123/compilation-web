// SLR(1) Parsing Table Generator Module

function loadSLR1File(event) {
    const file = event.target.files[0];
    if (!file) return;
    readFileAsText(file, function(text) {
        document.getElementById('slr1-input').value = text;
    });
}

function parseSLR1Grammar(text) {
    const lines = text.trim().split('\n').map(l => l.trim()).filter(l => l.length > 0);
    const prods = []; // [{lhs, rhs:[sym,...]}]
    const nonTerminals = new Set();

    lines.forEach(line => {
        if (line.startsWith('#')) return;
        let parts = line.split('->');
        if (parts.length < 2) return;
        const lhs = parts[0].trim();
        nonTerminals.add(lhs);
        const rhsPart = parts[1].trim();
        rhsPart.split('|').forEach(alt => {
            const rhs = alt.trim().split(/\s+/).filter(s => s.length > 0);
            prods.push({ lhs, rhs });
        });
    });

    // Augment
    const startNt = prods[0].lhs;
    const augStart = startNt + "'";
    prods.unshift({ lhs: augStart, rhs: [startNt] });
    nonTerminals.add(augStart);

    // Collect all symbols
    const allSymbols = new Set();
    prods.forEach(p => { allSymbols.add(p.lhs); p.rhs.forEach(s => allSymbols.add(s)); });
    const terminals = new Set();
    allSymbols.forEach(s => { if (!nonTerminals.has(s)) terminals.add(s); });

    return { prods, nonTerminals: [...nonTerminals], terminals: [...terminals],
             ntSet: nonTerminals, termSet: terminals, allSymbols, augStart };
}

// FIRST sets for symbols
function slr1ComputeFirst(grammar) {
    const first = {};
    grammar.allSymbols.forEach(s => first[s] = new Set());

    // Terminals: FIRST(t) = {t}
    grammar.termSet.forEach(t => first[t].add(t));

    let changed = true;
    while (changed) {
        changed = false;
        grammar.prods.forEach(prod => {
            const before = new Set(first[prod.lhs]);
            if (prod.rhs.length === 0) {
                first[prod.lhs].add('ε');
            } else {
                let allEps = true;
                for (const sym of prod.rhs) {
                    first[sym].forEach(f => { if (f !== 'ε') first[prod.lhs].add(f); });
                    if (!first[sym].has('ε')) { allEps = false; break; }
                }
                if (allEps) first[prod.lhs].add('ε');
            }
            if (before.size !== first[prod.lhs].size) changed = true;
        });
    }
    return first;
}

// FOLLOW sets
function slr1ComputeFollow(grammar, first) {
    const follow = {};
    grammar.nonTerminals.forEach(nt => follow[nt] = new Set());
    follow[grammar.augStart].add('$');

    let changed = true;
    while (changed) {
        changed = false;
        grammar.prods.forEach(prod => {
            for (let i = 0; i < prod.rhs.length; i++) {
                const B = prod.rhs[i];
                if (!grammar.ntSet.has(B)) continue;
                const before = new Set(follow[B]);
                let betaHasEps = true;
                for (let j = i + 1; j < prod.rhs.length; j++) {
                    const next = prod.rhs[j];
                    first[next].forEach(f => { if (f !== 'ε') follow[B].add(f); });
                    if (!first[next].has('ε')) { betaHasEps = false; break; }
                }
                if (betaHasEps) {
                    follow[prod.lhs].forEach(f => follow[B].add(f));
                }
                if (before.size !== follow[B].size) changed = true;
            }
        });
    }
    return follow;
}

// LR(0) items: { prodIdx, dotPos }
function slr1ItemKey(p, d) { return p + ':' + d; }

function slr1Closure(items, grammar) {
    const result = items.map(i => ({...i}));
    const seen = new Set(result.map(i => slr1ItemKey(i.p, i.d)));
    const queue = [...result];
    while (queue.length > 0) {
        const item = queue.shift();
        const prod = grammar.prods[item.p];
        if (item.d >= prod.rhs.length) continue;
        const nextSym = prod.rhs[item.d];
        if (!grammar.ntSet.has(nextSym)) continue;
        grammar.prods.forEach((p, idx) => {
            if (p.lhs === nextSym) {
                const key = slr1ItemKey(idx, 0);
                if (!seen.has(key)) {
                    seen.add(key);
                    const ni = { p: idx, d: 0 };
                    result.push(ni);
                    queue.push(ni);
                }
            }
        });
    }
    return result;
}

function slr1Goto(items, symbol, grammar) {
    const moved = [];
    items.forEach(item => {
        const prod = grammar.prods[item.p];
        if (item.d < prod.rhs.length && prod.rhs[item.d] === symbol) {
            moved.push({ p: item.p, d: item.d + 1 });
        }
    });
    if (moved.length === 0) return null;
    return slr1Closure(moved, grammar);
}

function slr1ItemSetKey(items) {
    return items.map(i => i.p + ':' + i.d).sort((a,b) => {
        const [ap,ad] = a.split(':').map(Number);
        const [bp,bd] = b.split(':').map(Number);
        return ap !== bp ? ap - bp : ad - bd;
    }).join('|');
}

function slr1BuildCollection(grammar) {
    const I0 = slr1Closure([{ p: 0, d: 0 }], grammar);
    const states = [I0];
    const stateKeys = [slr1ItemSetKey(I0)];
    const transitions = [];
    const queue = [0];

    while (queue.length > 0) {
        const idx = queue.shift();
        const state = states[idx];
        grammar.allSymbols.forEach(sym => {
            if (sym === grammar.augStart) return;
            const newState = slr1Goto(state, sym, grammar);
            if (!newState) return;
            const key = slr1ItemSetKey(newState);
            let targetIdx = stateKeys.indexOf(key);
            if (targetIdx === -1) {
                targetIdx = states.length;
                states.push(newState);
                stateKeys.push(key);
                queue.push(targetIdx);
            }
            transitions.push({ from: idx, symbol: sym, to: targetIdx });
        });
    }
    return { states, transitions };
}

function runSLR1() {
    const text = document.getElementById('slr1-input').value;
    const grammar = parseSLR1Grammar(text);
    if (grammar.prods.length === 0) { alert('文法格式错误'); return; }

    const first = slr1ComputeFirst(grammar);
    const follow = slr1ComputeFollow(grammar, first);
    const { states, transitions } = slr1BuildCollection(grammar);

    // Build transition lookup: [stateIdx, symbol] -> stateIdx
    const gotoMap = {};
    transitions.forEach(t => { gotoMap[t.from + ',' + t.symbol] = t.to; });

    // Build SLR(1) table
    const actionCols = [...grammar.termSet, '$'];
    const gotoCols = grammar.nonTerminals.filter(nt => nt !== grammar.augStart);

    const actionTable = {}; // [state,terminal] -> string
    const gotoTable = {};   // [state,nonterminal] -> state
    const conflicts = [];

    for (let i = 0; i < states.length; i++) {
        states[i].forEach(item => {
            const prod = grammar.prods[item.p];
            if (item.d < prod.rhs.length) {
                // Shift item
                const nextSym = prod.rhs[item.d];
                const key = i + ',' + nextSym;
                if (gotoMap[key] !== undefined) {
                    const nextState = gotoMap[key];
                    if (grammar.termSet.has(nextSym)) {
                        const cellKey = i + ',' + nextSym;
                        if (actionTable[cellKey]) {
                            // Conflict
                            const existing = actionTable[cellKey];
                            if (existing.startsWith('r')) {
                                // shift-reduce conflict: SLR(1) uses FOLLOW to resolve
                                const reduceProdIdx = parseInt(existing.substring(1));
                                const reduceLhs = grammar.prods[reduceProdIdx].lhs;
                                if (follow[reduceLhs] && follow[reduceLhs].has(nextSym)) {
                                    conflicts.push(`I${i}, ${nextSym}: 移进-归约冲突(FOLLOW包含该符号)`);
                                }
                                // shift takes priority
                                actionTable[cellKey] = 's' + nextState;
                            }
                        } else {
                            actionTable[cellKey] = 's' + nextState;
                        }
                    } else {
                        gotoTable[i + ',' + nextSym] = nextState;
                    }
                }
            } else {
                // Reduce item
                if (item.p === 0) {
                    // S' -> S . : accept
                    actionTable[i + ',$'] = 'acc';
                } else {
                    // A -> alpha . : reduce by prod i, for each terminal in FOLLOW(A)
                    const followA = follow[prod.lhs];
                    if (followA) {
                        followA.forEach(a => {
                            const cellKey = i + ',' + a;
                            if (actionTable[cellKey]) {
                                const existing = actionTable[cellKey];
                                if (existing.startsWith('s')) {
                                    // SLR(1) resolves: keep shift
                                    conflicts.push(`I${i}, ${a}: SLR(1)已解决(保留移进)`);
                                } else if (existing.startsWith('r')) {
                                    conflicts.push(`I${i}, ${a}: 归约-归约冲突`);
                                }
                            } else {
                                actionTable[cellKey] = 'r' + item.p;
                            }
                        });
                    }
                }
            }
        });
    }

    // Render FIRST/FOLLOW
    let ffHtml = '<div class="ff-section">';
    ffHtml += '<div class="ff-title">FIRST 集</div>';
    grammar.nonTerminals.forEach(nt => {
        const elems = [...first[nt]].join(', ');
        ffHtml += `<div class="ff-row"><span class="ff-label">FIRST(${nt})</span> = { ${elems} }</div>`;
    });
    ffHtml += '<div class="ff-title" style="margin-top:0.75rem">FOLLOW 集</div>';
    grammar.nonTerminals.forEach(nt => {
        const elems = [...follow[nt]].join(', ');
        ffHtml += `<div class="ff-row"><span class="ff-label">FOLLOW(${nt})</span> = { ${elems} }</div>`;
    });
    ffHtml += '</div>';
    document.getElementById('slr1-firstfollow').innerHTML = ffHtml;

    // Render SLR(1) table
    let html = '<div style="margin-bottom:0.75rem">';
    html += '<strong>增广文法:</strong><br>';
    grammar.prods.forEach((p, i) => {
        html += `<code>(${i}) ${p.lhs} → ${p.rhs.join(' ')}</code><br>`;
    });
    html += '</div>';

    html += '<table class="parse-table slr-table"><tr><th rowspan="2">状态</th>';
    html += `<th colspan="${actionCols.length}">ACTION</th>`;
    html += `<th colspan="${gotoCols.length}">GOTO</th></tr><tr>`;
    actionCols.forEach(c => html += `<th>${c}</th>`);
    gotoCols.forEach(c => html += `<th>${c}</th>`);
    html += '</tr>';

    for (let i = 0; i < states.length; i++) {
        html += `<tr><td><strong>I${i}</strong></td>`;
        actionCols.forEach(c => {
            const val = actionTable[i + ',' + c] || '';
            let cls = '';
            if (val === 'acc') cls = 'cell-acc';
            else if (val.startsWith('s')) cls = 'cell-shift';
            else if (val.startsWith('r')) cls = 'cell-reduce';
            html += `<td class="${cls}">${val}</td>`;
        });
        gotoCols.forEach(c => {
            const val = gotoTable[i + ',' + c];
            html += `<td class="${val !== undefined ? 'cell-goto' : ''}">${val !== undefined ? val : ''}</td>`;
        });
        html += '</tr>';
    }
    html += '</table>';

    if (conflicts.length > 0) {
        html += `<div class="mt-3 p-3 bg-yellow-50 rounded text-yellow-700 text-xs">
            <strong>冲突处理:</strong> ${conflicts.join('<br>')}</div>`;
    } else {
        html += `<div class="mt-3 p-3 bg-green-50 rounded text-green-600 text-xs">✓ 无冲突，该文法是SLR(1)文法</div>`;
    }

    document.getElementById('slr1-table').innerHTML = html;
}
