#!/usr/bin/env python3
"""
AI-GIT Task 1.2 Verification Test Suite: Asymmetric & Boundary-Aligned Chunking
Demonstrates that:
1. Volatile SafeTensors JSON header is isolated into a MetadataHeader chunk.
2. Tensor weights are isolated into TensorPayload chunks.
3. Modifying JSON hyperparameters produces 0% boundary drift and 100% deduplication on tensor chunks.
4. Parquet datasets have their PAR1 headers and footers isolated from data records.
5. Pre-compressed media (PNG/JPEG) are preserved without unwanted fragmentation.
"""

import os
import sys
import json
import struct
import subprocess

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

def run_tests():
    cli_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    chunker_exe = os.path.join(cli_dir, "aigit-chunk.exe" if os.name == "nt" else "aigit-chunk")
    fixtures_dir = os.path.join(cli_dir, "fixtures")

    if not os.path.exists(chunker_exe):
        print(f"❌ Error: {chunker_exe} not found. Build it first.")
        sys.exit(1)

    print("\n======================================================================")
    print("      🦎 AI-GIT TASK 1.2 VERIFICATION TEST SUITE (ASYMMETRIC CHUNKER)  ")
    print("======================================================================\n")

    # --------------------------------------------------------
    # TEST 1: SafeTensors Boundary Snapping & Deduplication Proof
    # --------------------------------------------------------
    print("[TEST 1] Testing SafeTensors Boundary Snapping & Fine-Tune Deduplication...")

    # Create model_v1 (Epoch 1) and model_v2 (Epoch 2 with different JSON header length and content)
    dummy_weights = b"\x40\x1f\x80\x3f" * 1024  # 4096 bytes identical tensor weights

    meta_v1 = {
        "model.layers.0.weight": {"dtype": "F32", "shape": [1024]},
        "training": {"epoch": 1, "loss": 2.45, "author": "Alice"}
    }
    json_v1 = json.dumps(meta_v1, separators=(',', ':')).encode('utf-8')
    v1_path = os.path.join(fixtures_dir, "model_v1.safetensors")
    with open(v1_path, "wb") as f:
        f.write(struct.pack("<Q", len(json_v1)))
        f.write(json_v1)
        f.write(dummy_weights)

    meta_v2 = {
        "model.layers.0.weight": {"dtype": "F32", "shape": [1024]},
        "training": {"epoch": 2, "loss": 1.12, "author": "Alice", "notes": "lr decayed with cosine annealing"}
    }
    json_v2 = json.dumps(meta_v2, separators=(',', ':')).encode('utf-8')
    v2_path = os.path.join(fixtures_dir, "model_v2.safetensors")
    with open(v2_path, "wb") as f:
        f.write(struct.pack("<Q", len(json_v2)))
        f.write(json_v2)
        f.write(dummy_weights)

    # Run aigit-chunk on both
    res_v1 = subprocess.run([chunker_exe, v1_path, "--json"], capture_output=True, text=True)
    res_v2 = subprocess.run([chunker_exe, v2_path, "--json"], capture_output=True, text=True)

    data_v1 = json.loads(res_v1.stdout.strip())
    data_v2 = json.loads(res_v2.stdout.strip())

    chunks_v1 = data_v1["chunks"]
    chunks_v2 = data_v2["chunks"]

    # Verify structural categories
    assert chunks_v1[0]["category"] == "MetadataHeader", "Chunk 0 must be MetadataHeader"
    assert chunks_v1[1]["category"] == "TensorPayload", "Chunk 1 must be TensorPayload"
    assert chunks_v2[0]["category"] == "MetadataHeader", "Chunk 0 must be MetadataHeader"
    assert chunks_v2[1]["category"] == "TensorPayload", "Chunk 1 must be TensorPayload"

    # Verify header chunk changed
    assert chunks_v1[0]["sha256"] != chunks_v2[0]["sha256"], "Headers must differ"

    # Verify TENSOR CHUNKS ARE 100% IDENTICAL (Deduplication Proof!)
    assert chunks_v1[1]["sha256"] == chunks_v2[1]["sha256"], "Tensor payload must be 100% identical!"
    assert chunks_v1[1]["length"] == len(dummy_weights), "Tensor chunk length must match weight size"

    print("  ✔ PASS: SafeTensors V1 Header Chunk Hash: " + chunks_v1[0]["sha256"][:16] + "...")
    print("  ✔ PASS: SafeTensors V2 Header Chunk Hash: " + chunks_v2[0]["sha256"][:16] + "...")
    print("  ✔ PASS: SafeTensors V1 Tensor Chunk Hash: " + chunks_v1[1]["sha256"][:16] + "...")
    print("  ✔ PASS: SafeTensors V2 Tensor Chunk Hash: " + chunks_v2[1]["sha256"][:16] + "...")
    print(f"  ✔ Method: {chunks_v1[1].get('method', 'unknown')}")
    print("  🎉 100% TENSOR DEDUPLICATION VERIFIED! Boundary drift eliminated.\n")

    # --------------------------------------------------------
    # TEST 2: Parquet Structural Slicing
    # --------------------------------------------------------
    print("[TEST 2] Testing Parquet Header/Footer Slicing...")
    pq_path = os.path.join(fixtures_dir, "train_dataset.parquet")
    res_pq = subprocess.run([chunker_exe, pq_path, "--json"], capture_output=True, text=True)
    data_pq = json.loads(res_pq.stdout.strip())
    chunks_pq = data_pq["chunks"]

    assert len(chunks_pq) == 3, f"Parquet must have 3 chunks, got {len(chunks_pq)}"
    assert chunks_pq[0]["category"] == "MetadataHeader" and chunks_pq[0]["length"] == 4
    assert chunks_pq[1]["category"] == "DatasetPayload"
    assert chunks_pq[2]["category"] == "MetadataHeader" and chunks_pq[2]["length"] == 4
    assert chunks_pq[0]["sha256"] == chunks_pq[2]["sha256"], "PAR1 magic hashes must match"

    print(f"  ✔ PASS: Parquet Header Chunk: {chunks_pq[0]['length']} B (Offset {chunks_pq[0]['offset']}) ➔ {chunks_pq[0].get('method')}")
    print(f"  ✔ PASS: Parquet Dataset Chunk: {chunks_pq[1]['length']} B (Offset {chunks_pq[1]['offset']}) ➔ {chunks_pq[1].get('method')}")
    print(f"  ✔ PASS: Parquet Footer Chunk: {chunks_pq[2]['length']} B (Offset {chunks_pq[2]['offset']}) ➔ {chunks_pq[2].get('method')}")
    print("  🎉 Parquet structural boundary alignment verified.\n")

    # --------------------------------------------------------
    # TEST 3: Pre-compressed Media Single-Blob Preservation
    # --------------------------------------------------------
    print("[TEST 3] Testing Media Preservation (No Fragmentation)...")
    png_path = os.path.join(fixtures_dir, "image.png")
    res_png = subprocess.run([chunker_exe, png_path, "--json"], capture_output=True, text=True)
    data_png = json.loads(res_png.stdout.strip())
    assert len(data_png["chunks"]) == 1, "Small PNG must not be fragmented"
    assert data_png["chunks"][0]["category"] == "MediaPayload"

    print(f"  ✔ PASS: PNG Media Chunk preserved as single atomic {data_png['chunks'][0]['length']} B payload.")
    print(f"  ✔ Method: {data_png['chunks'][0].get('method')}\n")

    print("======================================================================")
    print("🎉 ALL TASK 1.2 ASYMMETRIC CHUNKING TESTS PASSED SUCCESSFULLY (3/3)!")
    print("======================================================================\n")

if __name__ == "__main__":
    run_tests()
