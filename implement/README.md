# TensorStore Implementation Overview

## Why this engine helps
- **Tensor burst buffering**: log spikes of sensor or inference tensors without blocking GPU pipelines.
- **Feature cache**: persist feature maps between CPU/GPU stages for reuse or audit.
- **Edge snapshotting**: secure rolling snapshots from IoT fleets with minimal code.

## Core workflow
1. `TensorStore::save` serializes metadata + floats, appends to WAL, and lands in the MemTable.
2. When `memtableMax` is reached (or `flush()` is called), the MemTable flushes to immutable SST files.
3. `TensorStore::load` rebuilds tensors from MemTable/SSTs; `loadPreview` feeds dashboards with partial data.
4. `summarize` produces readable snippets for logs/UI, and `dryRunScenario` demonstrates end-to-end persistence.

## Dry run (CLI)
```bash
cd e:\EmbedDb
g++ -std=c++17 -O2 src\*.cpp implement\tensor_store.cpp -o tensor_demo.exe
tensor_demo.exe --run-dryrun   # wire this flag to call TensorStore::dryRunScenario()
```
Expected flow:
1. WAL records tensor keys `101` and `202`.
2. MemTable flush prints `flushed 2 to sst_000000.sst`.
3. Dry run replays tensors and logs summaries such as `camera_frame (32x32) -> 0.25, 0.25, ...`.

## Web demo
- Location: `web/` (see `index.html`, `app.js`, `styles.css`).
- The form mirrors tensor metadata input; JavaScript simulates serialization, WAL append, flush, and preview steps.
- Hook `/api/tensors/store` and `/api/tensors/load` to backend endpoints that wrap `TensorStore::save` and `TensorStore::load`.
- **DL model feed**: regenerate tensors via `python implement/python_demo/dl_model_demo.py`; the emitted `weights_baseline.json` and `tensor_payload.txt` are read by the demo to compare baseline vs EmbedDb persistence, mirroring the CLI demos.

## Integration tips
- Batch ingestion: call `saveBatch` for micro-batches to minimize WAL fsyncs.
- Dashboards: use `loadPreview` and `summarize` to feed real-time UI cards without loading full tensors.
- Archival: periodically move SST files to colder tiers once compaction/retention policies run.

## CLI run modes

```bash
# original benchmark (numeric args still supported)
tensor_demo.exe 500000 32 1 20000

# tensor dry-run walkthrough
tensor_demo.exe --tensor-dryrun

# tiny DL training demo that checkpoints weights each epoch
tensor_demo.exe --tensor-train

# ingest tensor data from a file (see --tensor-ingest)
tensor_demo.exe --tensor-ingest implement/python_demo/tensor_payload.txt
```

- `--tensor-dryrun`: executes `TensorStore::dryRunScenario`, shows WAL+flush progress, and prints human summaries.
- `--tensor-train`: runs a miniature dense-layer “training” loop, logs weight updates, persists each epoch, and flushes to SST files.
- `--tensor-ingest <file>`: ingests tensor data from the specified file (default sample lives in `implement/python_demo/tensor_payload.txt`). If the file is missing, the binary now hints to run `python_demo/dl_model_demo.py`.

## Python DL demo
Directory: `implement/python_demo/`

```bash
cd implement/python_demo
python dl_model_demo.py
```

The script:
1. Trains a tiny NumPy-based dense layer.
2. Saves final weights to a baseline JSON file and measures time.
3. Emits `tensor_payload.txt` (CSV format) and calls `tensor_demo.exe --tensor-ingest` to store weights in EmbedDb.
4. Prints a side-by-side comparison of baseline vs EmbedDb persistence times along with summaries pulled from the ingest step.
5. The emitted payload doubles as the default input for `--tensor-ingest`, so you can immediately replay it via CLI or the browser demo.

## Browser demo (`implement/demo`)
1. Serve the folder (e.g., `cd implement/demo && npx serve`).
2. The page loads:
   - Baseline JSON stats from `python_demo/weights_baseline.json`.
   - EmbedDb payload stats from `python_demo/tensor_payload.txt`.
3. It visualizes WAL → MemTable → SST flow, highlights gaps when files are missing, and links back to the CLI/Python instructions so everything stays in sync with the TensorStore helpers.
