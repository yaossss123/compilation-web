// LR(0) Canonical Collection Module

function loadLR0File(event) {
    const file = event.target.files[0];
    if (!file) return;
    readFileAsText(file, function(text) {
        document.getElementById('lr0-input').value = text;
    });
}

function parseLR0Grammar(text) {
    const lines = text.trim().split('\n').map(l => l.trim()).filter(l => l.length > 0);
    const grammar = [];
    const nonTerminals = new Set();

    lines.forEach(line => {
        const parts = line.split('->');
        if (parts.length < 2) return;
        const lhs = parts[0].trim();
        const rhsList = parts[1].trim().split('|').map(s => s.trim());
        nonTerminals.add(lhs);
        rhsList.forEach(rhs => {
            grammar.push({ lhs, rhs: rhs.split(/\s+/) });
        });
    });

    // Augment
    const startNt = grammar[0].lhs;
    const augmentedStart = startNt + "'";
    grammar.unshift({ lhs: augmentedStart, rhs: [startNt] });
    nonTerminals.add(augmentedStart);

    return { grammar, nonTerminals: [...nonTerminals] };
}

function itemKey(item) {
    return item.lhs + '->' + item.rhs.slice(0, item.dot).join(' ') + ' . ' + item.rhs.slice(item.dot).join(' ');
}

function itemSetKey(items) {
    return items.map(itemKey).sort().join(' | ');
}

function closure(items, grammar, nonTerminals) {
    const result = items.map(i => ({ ...i }));
    const seen = new Set(result.map(itemKey));
    let queue = [...result];

    while (queue.length > 0) {
        const item = queue.shift();
        if (item.dot >= item.rhs.length) continue;
        const nextSym = item.rhs[item.dot];
        if (!nonTerminals.includes(nextSym)) continue;

        grammar.forEach(prod => {
            if (prod.lhs === nextSym) {
                const newItem = { lhs: prod.lhs, rhs: [...prod.rhs], dot: 0 };
                const key = itemKey(newItem);
                if (!seen.has(key)) {
                    seen.add(key);
                    result.push(newItem);
                    queue.push(newItem);
                }
            }
        });
    }
    return result;
}

function goto(items, symbol, grammar, nonTerminals) {
    const moved = [];
    items.forEach(item => {
        if (item.dot < item.rhs.length && item.rhs[item.dot] === symbol) {
            moved.push({ lhs: item.lhs, rhs: [...item.rhs], dot: item.dot + 1 });
        }
    });
    if (moved.length === 0) return null;
    return closure(moved, grammar, nonTerminals);
}

function buildCanonicalCollection(grammar, nonTerminals) {
    const startItem = { lhs: grammar[0].lhs, rhs: [...grammar[0].rhs], dot: 0 };
    const I0 = closure([startItem], grammar, nonTerminals);

    const states = [I0];
    const stateKeys = [itemSetKey(I0)];
    const transitions = [];
    const queue = [0];

    // Get all symbols
    const allSymbols = new Set();
    grammar.forEach(p => {
        p.rhs.forEach(s => allSymbols.add(s));
    });
    allSymbols.delete('$');

    while (queue.length > 0) {
        const idx = queue.shift();
        const state = states[idx];

        allSymbols.forEach(sym => {
            const newState = goto(state, sym, grammar, nonTerminals);
            if (!newState) return;
            const key = itemSetKey(newState);
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

    // Detect conflicts
    const conflicts = [];
    states.forEach((state, idx) => {
        const hasShift = state.some(i => i.dot < i.rhs.length);
        const hasReduce = state.some(i => i.dot >= i.rhs.length && i.lhs !== grammar[0].lhs);
        const reduceCount = state.filter(i => i.dot >= i.rhs.length && i.lhs !== grammar[0].lhs).length;
        if (hasShift && hasReduce) conflicts.push(`I${idx}: shift-reduce`);
        if (reduceCount > 1) conflicts.push(`I${idx}: reduce-reduce`);
    });

    return { states, transitions, conflicts };
}

function renderLR0Graph(collection, containerId) {
    const container = document.getElementById(containerId);
    container.innerHTML = '';

    const nodes = new vis.DataSet();
    const edges = new vis.DataSet();

    collection.states.forEach((state, idx) => {
        const label = 'I' + idx + '\n(' + state.length + ' items)';
        const hasConflict = collection.conflicts.some(c => c.startsWith('I' + idx));
        nodes.add({
            id: idx,
            label: label,
            shape: 'box',
            color: {
                background: hasConflict ? '#fef2f2' : '#eff6ff',
                border: hasConflict ? '#dc2626' : '#3b82f6'
            },
            font: { size: 12 }
        });
    });

    // Group edges
    const edgeMap = {};
    collection.transitions.forEach(t => {
        const key = t.from + '->' + t.to;
        if (!edgeMap[key]) edgeMap[key] = [];
        edgeMap[key].push(t.symbol);
    });

    let edgeId = 0;
    Object.keys(edgeMap).forEach(key => {
        const [from, to] = key.split('->').map(Number);
        const labels = edgeMap[key].join(', ');
        edges.add({
            id: edgeId++,
            from, to,
            label: labels,
            arrows: 'to',
            color: { color: '#6b7280' },
            font: { size: 11, align: 'middle' },
            smooth: { type: 'curvedCW', roundness: 0.15 }
        });
    });

    new vis.Network(container, { nodes, edges }, {
        physics: { solver: 'forceAtlas2Based', forceAtlas2Based: { gravitationalConstant: -120 } },
        interaction: { hover: true }
    });
}

function runLR0() {
    const text = document.getElementById('lr0-input').value;
    const { grammar, nonTerminals } = parseLR0Grammar(text);

    if (grammar.length === 0) { alert('文法格式错误'); return; }

    const collection = buildCanonicalCollection(grammar, nonTerminals);

    // Render items
    const itemsDiv = document.getElementById('lr0-items');

    let html = '';
    collection.states.forEach((state, idx) => {
        html += `<div class="item-set"><div class="item-set-title">I${idx}</div>`;
        state.forEach(item => {
            const before = item.rhs.slice(0, item.dot).join(' ');
            const after = item.rhs.slice(item.dot).join(' ');
            const itemStr = `${item.lhs} → ${before} • ${after}`.trim();
            html += `<div class="item">${itemStr}</div>`;
        });
        html += '</div>';
    });

    if (collection.conflicts.length > 0) {
        html += `<div class="mt-3 p-3 bg-red-50 rounded text-red-600 text-xs"><strong>冲突:</strong> ${collection.conflicts.join('; ')}<br>该文法不是LR(0)文法</div>`;
    } else {
        html += `<div class="mt-3 p-3 bg-green-50 rounded text-green-600 text-xs">✓ 无冲突，该文法是LR(0)文法</div>`;
    }

    itemsDiv.innerHTML = html;

    // Render graph
    renderLR0Graph(collection, 'lr0-graph');
}
