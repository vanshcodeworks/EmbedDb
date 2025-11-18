import json
import subprocess
import time
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
EXECUTABLE = ROOT / "tensor_demo.exe"
BASE_DIR = Path(__file__).resolve().parent
BASELINE_FILE = BASE_DIR / "weights_baseline.json"
PAYLOAD_FILE = BASE_DIR / "tensor_payload.txt"

def train_dense_layer(dim=4, epochs=5, lr=0.1):
    np.random.seed(42)
    weights = np.random.uniform(-0.5, 0.5, size=dim)
    grad = np.linspace(0.02, -0.03, dim)
    for _ in range(epochs):
        weights -= lr * grad
    return weights.tolist()

def logic_dataset():
    inputs = np.array(
        [
            [0.0, 0.0],
            [0.0, 1.0],
            [1.0, 0.0],
            [1.0, 1.0],
        ],
        dtype=np.float32,
    )
    targets = np.array(
        [
            [0.0, 0.0, 0.0],  # XOR, AND, OR for (0,0)
            [1.0, 0.0, 1.0],
            [1.0, 0.0, 1.0],
            [0.0, 1.0, 1.0],
        ],
        dtype=np.float32,
    )
    return inputs, targets

def train_logic_model(hidden=4, epochs=8000, lr=0.8):
    X, Y = logic_dataset()
    rng = np.random.default_rng(42)
    W1 = rng.uniform(-1.0, 1.0, size=(2, hidden))
    b1 = np.zeros((1, hidden))
    W2 = rng.uniform(-1.0, 1.0, size=(hidden, 3))
    b2 = np.zeros((1, 3))

    for _ in range(epochs):
        z1 = X @ W1 + b1
        a1 = np.tanh(z1)
        z2 = a1 @ W2 + b2
        y_hat = 1 / (1 + np.exp(-z2))

        diff = y_hat - Y
        batch_scale = 2.0 / len(X)
        dz2 = diff * y_hat * (1 - y_hat) * batch_scale
        dW2 = a1.T @ dz2
        db2 = dz2.sum(axis=0, keepdims=True)

        da1 = dz2 @ W2.T
        dz1 = da1 * (1 - np.square(a1))
        dW1 = X.T @ dz1
        db1 = dz1.sum(axis=0, keepdims=True)

        W1 -= lr * dW1
        b1 -= lr * db1
        W2 -= lr * dW2
        b2 -= lr * db2

    z1 = X @ W1 + b1
    a1 = np.tanh(z1)
    z2 = a1 @ W2 + b2
    y_hat = 1 / (1 + np.exp(-z2))
    predictions = (y_hat > 0.5).astype(int).tolist()

    return {
        "W1": W1.tolist(),
        "b1": b1.tolist(),
        "W2": W2.tolist(),
        "b2": b2.tolist(),
        "predictions": predictions,
    }

def forward_logic(weights_dict, inputs):
    W1 = np.array(weights_dict["W1"], dtype=np.float32)
    b1 = np.array(weights_dict["b1"], dtype=np.float32)
    W2 = np.array(weights_dict["W2"], dtype=np.float32)
    b2 = np.array(weights_dict["b2"], dtype=np.float32)

    z1 = inputs @ W1 + b1
    a1 = np.tanh(z1)
    z2 = a1 @ W2 + b2
    y_hat = 1 / (1 + np.exp(-z2))
    return (y_hat > 0.5).astype(int)

def show_xor_predictions(weights_dict):
    X, Y = logic_dataset()
    preds = forward_logic(weights_dict, X)
    print("XOR predictions (only first column shown):")
    for sample, truth, pred in zip(X.astype(int), Y[:, 0].astype(int), preds[:, 0]):
        print(f"  input={sample.tolist()} -> truth={truth}, predicted={int(pred)}")

def flatten_weights(weights_dict):
    flat = []
    for key in ("W1", "b1", "W2", "b2"):
        flat.extend(np.array(weights_dict[key], dtype=np.float32).ravel())
    return flat

def save_baseline(weights):
    start = time.perf_counter()
    with open(BASELINE_FILE, "w", encoding="utf-8") as f:
        json.dump(weights, f, indent=2)
    return time.perf_counter() - start

def save_embeddb_payload(weights, key=9001, name="logic_net", rows=1):
    cols = len(weights)
    with open(PAYLOAD_FILE, "w", encoding="utf-8") as f:
        f.write(f"{key},{name},{rows},{cols}\n")
        f.write(",".join(f"{w:.6f}" for w in weights))
    start = time.perf_counter()
    subprocess.run([str(EXECUTABLE), "--tensor-ingest", str(PAYLOAD_FILE)], check=True)
    return time.perf_counter() - start

def main():
    if not EXECUTABLE.exists():
        raise FileNotFoundError("tensor_demo.exe not found; build it before running this demo.")
    weights = train_dense_layer()
    baseline_time = save_baseline(weights)
    embed_time = save_embeddb_payload(weights)

    print("Weights:", weights)
    print(f"Baseline JSON save: {baseline_time * 1e3:.3f} ms -> {BASELINE_FILE}")
    print(f"EmbedDb ingest: {embed_time * 1e3:.3f} ms -> {PAYLOAD_FILE}")
    print("Use `tensor_demo.exe --tensor-dryrun` to replay stored tensors.")

    trained = train_logic_model()
    flat_weights = flatten_weights(trained)
    baseline_time = save_baseline(trained)
    embed_time = save_embeddb_payload(flat_weights)

    print("Truth table predictions per row [XOR, AND, OR]:")
    for row_in, row_out in zip(*logic_dataset()):
        print(f"  input={row_in.astype(int).tolist()} -> {row_out.tolist()}")
    print("Model predictions:", trained["predictions"])
    show_xor_predictions(trained)
    print(f"Baseline JSON save: {baseline_time * 1e3:.3f} ms -> {BASELINE_FILE}")
    print(f"EmbedDb ingest: {embed_time * 1e3:.3f} ms -> {PAYLOAD_FILE}")
    print("Replay via `tensor_demo.exe --tensor-dryrun` for summaries.")

if __name__ == "__main__":
    main()
