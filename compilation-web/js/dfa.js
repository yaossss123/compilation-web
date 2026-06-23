// DFA Simulator Module

let currentDfa = null;

function loadDfaFile(event) {
    const file = event.target.files[0];
    if (!file) return;
    readFileAsText(file, function(text) {
        const lines = text.trim().split('\n').map(l => l.trim()).filter(l => l.length > 0);
        if (lines.length >= 4) {
            document.getElementById('dfa-alphabet').value = lines[0];
            document.getElementById('dfa-states').value = lines[1];
            document.getElementById('dfa-start').value = lines[2];
            document.getElementById('dfa-accept').value = lines[3];
            if (lines.length > 4) {
                document.getElementById('dfa-transitions').value = lines.slice(4).join('\n');
            }
        }
    });
}

function parseDfaFromUI() {
    const alphabet = document.getElementById('dfa-alphabet').value.trim().split(/\s+/);
    const states = document.getElementById('dfa-states').value.trim().split(/\s+/);
    const startState = document.getElementById('dfa-start').value.trim();
    const acceptStates = new Set(document.getElementById('dfa-accept').value.trim().split(/\s+/));
    const transitions = {};

    const tLines = document.getElementById('dfa-transitions').value.trim().split('\n');
    tLines.forEach(line => {
        const parts = line.trim().split(/\s+/);
        if (parts.length >= 3) {
            transitions[parts[0] + ',' + parts[1]] = parts[2];
        }
    });

    return { alphabet, states, startState, acceptStates, transitions };
}

function renderDfaGraph(dfa, containerId) {
    const container = document.getElementById(containerId);
    container.innerHTML = '';

    const nodes = new vis.DataSet();
    const edges = new vis.DataSet();

    dfa.states.forEach(s => {
        const isStart = s === dfa.startState;
        const isAccept = dfa.acceptStates.has(s);
        let label = s;
        let color = { background: '#f3f4f6', border: '#9ca3af' };

        if (isStart && isAccept) {
            label = '▶ ' + s + ' ✓';
            color = { background: '#dcfce7', border: '#16a34a' };
        } else if (isStart) {
            label = '▶ ' + s;
            color = { background: '#dbeafe', border: '#2563eb' };
        } else if (isAccept) {
            label = s + ' ✓';
            color = { background: '#ede9fe', border: '#7c3aed' };
        }

        nodes.add({ id: s, label, shape: 'box', color, font: { size: 13 } });
    });

    const edgeMap = {};
    Object.keys(dfa.transitions).forEach(key => {
        const [from, ch] = key.split(',');
        const to = dfa.transitions[key];
        const ek = from + '->' + to;
        if (!edgeMap[ek]) edgeMap[ek] = [];
        edgeMap[ek].push(ch);
    });

    let eid = 0;
    Object.keys(edgeMap).forEach(ek => {
        const [from, to] = ek.split('->');
        edges.add({
            id: eid++, from, to,
            label: edgeMap[ek].join(', '),
            arrows: 'to',
            color: { color: '#6b7280', highlight: '#7c3aed' },
            font: { size: 12, align: 'middle' },
            smooth: { type: 'curvedCW', roundness: 0.2 }
        });
    });

    new vis.Network(container, { nodes, edges }, {
        physics: { solver: 'forceAtlas2Based', forceAtlas2Based: { gravitationalConstant: -80 } },
        interaction: { hover: true }
    });
}

function runDfa() {
    const dfa = parseDfaFromUI();
    currentDfa = dfa;
    renderDfaGraph(dfa, 'dfa-graph');
}

function testDfaString() {
    if (!currentDfa) { alert('请先点击"解析并生成图"按钮'); return; }
    const inputStr = document.getElementById('dfa-string').value;

    const dfa = currentDfa;
    console.log('=== DFA Test ===');
    console.log('Input:', JSON.stringify(inputStr));
    console.log('Start:', dfa.startState);
    console.log('Accept states:', [...dfa.acceptStates]);
    console.log('Transitions:', JSON.stringify(dfa.transitions));

    let cur = dfa.startState;
    let path = [cur];
    let ok = true;

    for (const ch of inputStr) {
        const key = cur + ',' + ch;
        console.log('  char:', JSON.stringify(ch), 'key:', key, '->', dfa.transitions[key]);
        if (dfa.transitions[key] === undefined) { ok = false; console.log('  BREAK: no transition for', key); break; }
        cur = dfa.transitions[key];
        path.push(cur);
    }

    const accepted = ok && dfa.acceptStates.has(cur);
    console.log('Final state:', cur, 'Accepted:', accepted);
    const div = document.getElementById('dfa-result');
    div.classList.remove('hidden');
    div.innerHTML = `
        <p><strong>输入:</strong> <code>${inputStr || '(空字符串 ε)'}</code></p>
        <p><strong>路径:</strong> ${path.join(' → ')}</p>
        <p><strong>结果:</strong> <span class="${accepted ? 'result-accept' : 'result-reject'}">${accepted ? '✓ ACCEPT' : '✗ REJECT'}</span></p>
        ${!ok ? '<p class="text-red-500 text-xs">转移中断：无对应出边</p>' : ''}
    `;
}

function enumerateDfa() {
    if (!currentDfa) { alert('请先解析DFA'); return; }
    const maxLen = parseInt(document.getElementById('dfa-maxlen').value) || 5;
    const dfa = currentDfa;

    const accepted = [];
    const queue = [{ state: dfa.startState, str: '' }];

    while (queue.length > 0 && accepted.length < 200) {
        const { state, str } = queue.shift();
        if (dfa.acceptStates.has(state)) {
            accepted.push(str === '' ? '(ε)' : str);
        }
        if (str.length < maxLen) {
            dfa.alphabet.forEach(ch => {
                const next = dfa.transitions[state + ',' + ch];
                if (next !== undefined) queue.push({ state: next, str: str + ch });
            });
        }
    }

    const div = document.getElementById('dfa-enum');
    div.classList.remove('hidden');

    if (accepted.length === 0) {
        div.innerHTML = `<p class="text-gray-500">长度 ≤ ${maxLen} 范围内无接受字符串</p>`;
    } else {
        const tags = accepted.map(s => `<span class="accept-tag">${s}</span>`).join('');
        div.innerHTML = `
            <p class="mb-2">长度 ≤ ${maxLen}，共 <strong>${accepted.length}</strong> 个：</p>
            <div>${tags}</div>
        `;
    }
}
