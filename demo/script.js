document.addEventListener('DOMContentLoaded', () => {
    // --- STATE ---
    const MEMTABLE_MAX_SIZE = 5;
    let memtable = new Map();
    let wal = [];
    let sstables = [];
    let sstableCounter = 0;

    // --- DOM ELEMENTS ---
    const keyInput = document.getElementById('key-input');
    const valueInput = document.getElementById('value-input');
    const putBtn = document.getElementById('put-btn');
    const getKeyInput = document.getElementById('get-key-input');
    const getBtn = document.getElementById('get-btn');
    const compactBtn = document.getElementById('compact-btn');
    const resetBtn = document.getElementById('reset-btn');
    const walDiv = document.getElementById('wal');
    const memtableDiv = document.getElementById('memtable');
    const sstablesContainer = document.getElementById('sstables-container');
    const outputLog = document.getElementById('output-log');

    // --- CORE LOGIC ---
    const handlePut = () => {
        const key = keyInput.value.trim();
        const value = valueInput.value.trim();
        if (!key || !value) {
            logMessage('Error: Key and Value cannot be empty.', 'error');
            return;
        }

        // 1. Write to WAL
        wal.push(`PUT(${key}, ${value})`);
        // 2. Write to MemTable
        memtable.set(key, value);

        logMessage(`Wrote ('${key}', '${value}') to MemTable.`);
        render();

        // 3. Check if MemTable is full
        if (memtable.size >= MEMTABLE_MAX_SIZE) {
            logMessage('MemTable is full! Flushing to SSTable...', 'action');
            setTimeout(flushToSSTable, 500);
        }
    };

    const flushToSSTable = () => {
        // Create a new SSTable from a sorted version of the MemTable
        const sortedMemtable = new Map([...memtable.entries()].sort());
        const newSSTable = {
            id: sstableCounter++,
            data: sortedMemtable
        };
        sstables.push(newSSTable);

        // Clear in-memory structures
        memtable.clear();
        wal = [];

        logMessage(`Flushed MemTable to SSTable-${newSSTable.id}.`);
        render();
    };

    const handleGet = async () => {
        const key = getKeyInput.value.trim();
        if (!key) {
            logMessage('Error: Key to find cannot be empty.', 'error');
            return;
        }
        logMessage(`Searching for key: '${key}'...`);
        clearHighlights();

        // 1. Check MemTable
        logMessage('Step 1: Checking MemTable...');
        await highlightSearch(memtableDiv, key, 500);
        if (memtable.has(key)) {
            const value = memtable.get(key);
            logMessage(`Found key '${key}' in MemTable! Value: '${value}'`, 'success');
            await highlightFound(memtableDiv, key, 1000);
            render(); // to clear highlights
            return;
        }

        // 2. Check SSTables (newest to oldest)
        logMessage('Step 2: Checking SSTables (newest to oldest)...');
        for (let i = sstables.length - 1; i >= 0; i--) {
            const sstable = sstables[i];
            const sstableDiv = document.getElementById(`sstable-${sstable.id}`);
            await highlightSearch(sstableDiv, key, 500);

            if (sstable.data.has(key)) {
                const value = sstable.data.get(key);
                logMessage(`Found key '${key}' in SSTable-${sstable.id}! Value: '${value}'`, 'success');
                await highlightFound(sstableDiv, key, 1000);
                render(); // to clear highlights
                return;
            }
        }

        logMessage(`Key '${key}' not found in any structure.`, 'error');
        render(); // to clear highlights
    };
    
    const handleCompact = () => {
        if (sstables.length < 2) {
            logMessage('Need at least 2 SSTables to compact.', 'error');
            return;
        }

        logMessage('Starting compaction of all SSTables...', 'action');
        
        const mergedData = new Map();
        // Iterate oldest to newest to ensure newer values overwrite older ones
        for (const sstable of sstables) {
            for (const [key, value] of sstable.data.entries()) {
                mergedData.set(key, value);
            }
        }
        
        const newSSTable = {
            id: sstableCounter++,
            data: new Map([...mergedData.entries()].sort())
        };

        sstables = [newSSTable]; // Replace all old SSTables with the new compacted one

        logMessage(`Compaction complete. Created new SSTable-${newSSTable.id}.`);
        render();
    };


    // --- RENDERING & UI ---
    const render = () => {
        // Render WAL
        walDiv.innerHTML = wal.map(entry => `<div>${entry}</div>`).join('');
        // Render MemTable
        memtableDiv.innerHTML = '';
        for (const [key, value] of memtable.entries()) {
            memtableDiv.innerHTML += createKVPairHTML(key, value);
        }
        // Render SSTables
        sstablesContainer.innerHTML = '';
        sstables.forEach(sstable => {
            const sstableDiv = document.createElement('div');
            sstableDiv.className = 'sstable';
            sstableDiv.id = `sstable-${sstable.id}`;
            let kvHTML = '';
            for (const [key, value] of sstable.data.entries()) {
                kvHTML += createKVPairHTML(key, value);
            }
            sstableDiv.innerHTML = `<h4>SSTable-${sstable.id}</h4>${kvHTML}`;
            sstablesContainer.appendChild(sstableDiv);
        });
    };

    const createKVPairHTML = (key, value) => `<div class="kv-pair" data-key="${key}"><span class="key">${key}:</span> ${value}</div>`;
    
    const logMessage = (msg, type = 'info') => {
        outputLog.innerHTML = `<p><strong>Status:</strong> ${msg}</p>`;
        outputLog.style.color = type === 'error' ? '#e74c3c' : (type === 'success' ? '#2ecc71' : '#ecf0f1');
    };

    const clearHighlights = () => {
        document.querySelectorAll('.highlight-search, .highlight-found').forEach(el => {
            el.classList.remove('highlight-search', 'highlight-found');
        });
    };
    
    const highlightSearch = (container, key, delay) => {
        return new Promise(resolve => {
            const element = container.querySelector(`.kv-pair[data-key="${key}"]`);
            if (element) element.classList.add('highlight-search');
            setTimeout(resolve, delay);
        });
    };

    const highlightFound = (container, key, delay) => {
        return new Promise(resolve => {
            const element = container.querySelector(`.kv-pair[data-key="${key}"]`);
            if (element) {
                element.classList.remove('highlight-search');
                element.classList.add('highlight-found');
            }
            setTimeout(resolve, delay);
        });
    };
    
    const reset = () => {
        memtable.clear();
        wal = [];
        sstables = [];
        sstableCounter = 0;
        logMessage('Demo reset.');
        render();
    };

    // --- EVENT LISTENERS ---
    putBtn.addEventListener('click', handlePut);
    getBtn.addEventListener('click', handleGet);
    compactBtn.addEventListener('click', handleCompact);
    resetBtn.addEventListener('click', reset);
    
    // Initial Render
    render();
});