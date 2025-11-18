const baselineBox = document.getElementById('baseline-json');
const embedBox = document.getElementById('embeddb-payload');
const refreshBtn = document.getElementById('refresh-btn');

async function loadText(path) {
    try {
        const res = await fetch(path);
        if (!res.ok) throw new Error(`${res.status} ${res.statusText}`);
        return await res.text();
    } catch (err) {
        return `Missing (${path}).\nRun python_demo/dl_model_demo.py first.\n\n${err}`;
    }
}

async function refresh() {
    const baseline = await loadText('../python_demo/weights_baseline.json');
    const payload = await loadText('../python_demo/tensor_payload.txt');
    baselineBox.textContent = baseline;
    embedBox.textContent = payload;
}

refreshBtn.addEventListener('click', refresh);
refresh();
