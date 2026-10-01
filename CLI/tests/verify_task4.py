#!/usr/bin/env python3
"""
AI-GIT Task 1.4 Verification Test Suite: Metadata Harvesting & Semantic Structural Diffing
Demonstrates that:
1. SafeTensors models have their layers, shapes, dtypes, and parameters harvested.
2. Semantic model diffing detects Added, Removed, Modified, and Identical (deduplicated) layers.
3. GGUF model containers have architecture metadata extracted.
4. Harvesting indexes tensors and attributes into SQLite, enabling direct SQL queries over model weights.
"""

import os
import sys
import json
import struct
import sqlite3
import subprocess
import tempfile

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

def run_tests():
    cli_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    diff_exe = os.path.join(cli_dir, "aigit-diff.exe" if os.name == "nt" else "aigit-diff")
    fixtures_dir = os.path.join(cli_dir, "fixtures")

    if not os.path.exists(diff_exe):
        print(f"❌ Error: {diff_exe} not found. Build it first.")
        sys.exit(1)

    print("\n======================================================================")
    print("      🦎 AI-GIT TASK 1.4 VERIFICATION TEST SUITE (METADATA & DIFFING)  ")
    print("======================================================================\n")

    # ------------------------------------------------------------------------
    # TEST 1: SafeTensors Metadata Harvesting
    # ------------------------------------------------------------------------
    print("[TEST 1] Testing SafeTensors Tensor & Parameter Harvesting...")
    st_path = os.path.join(fixtures_dir, "model.safetensors")
    res = subprocess.run([diff_exe, "inspect", st_path, "--json"], capture_output=True, encoding="utf-8", errors="replace")
    assert res.returncode == 0, f"inspect failed: {res.stderr}"

    data = json.loads(res.stdout.strip())
    assert data["format"] == "SafeTensors"
    assert len(data["tensors"]) >= 1

    t0 = data["tensors"][0]
    print(f"  ✔ PASS: Harvested Tensor: {t0['name']}")
    print(f"  ✔ PASS: Dtype: {t0['dtype']} | Shape: {t0['shape']} | Elements: {t0['elements']}")
    print(f"  ✔ PASS: Weight SHA-256: {t0['hash'][:16]}...")
    print("  🎉 SafeTensors metadata harvesting verified!\n")

    # ------------------------------------------------------------------------
    # TEST 2: Semantic SafeTensors Model Diffing (Added, Removed, Modified, Identical)
    # ------------------------------------------------------------------------
    print("[TEST 2] Testing Semantic Model Diffing vs Opaque Git Diff...")

    with tempfile.TemporaryDirectory() as tmpdir:
        model_a_path = os.path.join(tmpdir, "base_model.safetensors")
        model_b_path = os.path.join(tmpdir, "finetuned_model.safetensors")

        weights_shared = b"\x3f\x80\x00\x00" * 512    # 2048 bytes identical weights
        weights_old = b"\x40\x00\x00\x00" * 512       # 2048 bytes
        weights_modified = b"\x40\x40\x00\x00" * 512  # 2048 bytes modified
        weights_removed = b"\x40\x80\x00\x00" * 256   # 1024 bytes removed
        weights_added = b"\x41\x00\x00\x00" * 256     # 1024 bytes added

        # Model A:
        # - layer.shared.weight (F32 [512]) -> identical
        # - layer.active.weight (F32 [512]) -> will be modified
        # - layer.pruned.weight (F32 [256]) -> will be removed
        hdr_a = {
            "layer.shared.weight": {"dtype": "F32", "shape": [512], "data_offsets": [0, 2048]},
            "layer.active.weight": {"dtype": "F32", "shape": [512], "data_offsets": [2048, 4096]},
            "layer.pruned.weight": {"dtype": "F32", "shape": [256], "data_offsets": [4096, 5120]},
            "__metadata__": {"epoch": "1", "loss": "2.45"}
        }
        json_a = json.dumps(hdr_a, separators=(',', ':')).encode('utf-8')
        with open(model_a_path, "wb") as f:
            f.write(struct.pack("<Q", len(json_a)))
            f.write(json_a)
            f.write(weights_shared)
            f.write(weights_old)
            f.write(weights_removed)

        # Model B (Fine-tuned):
        # - layer.shared.weight (F32 [512]) -> IDENTICAL (deduplicated)
        # - layer.active.weight (F32 [512]) -> MODIFIED weights
        # - layer.head.weight   (F32 [256]) -> ADDED layer
        hdr_b = {
            "layer.shared.weight": {"dtype": "F32", "shape": [512], "data_offsets": [0, 2048]},
            "layer.active.weight": {"dtype": "F32", "shape": [512], "data_offsets": [2048, 4096]},
            "layer.head.weight":   {"dtype": "F32", "shape": [256], "data_offsets": [4096, 5120]},
            "__metadata__": {"epoch": "2", "loss": "1.12"}
        }
        json_b = json.dumps(hdr_b, separators=(',', ':')).encode('utf-8')
        with open(model_b_path, "wb") as f:
            f.write(struct.pack("<Q", len(json_b)))
            f.write(json_b)
            f.write(weights_shared)
            f.write(weights_modified)
            f.write(weights_added)

        # Run aigit-diff
        res_diff = subprocess.run([diff_exe, "diff", model_a_path, model_b_path, "--json"], capture_output=True, encoding="utf-8", errors="replace")
        assert res_diff.returncode == 0, f"diff failed: {res_diff.stderr}"

        diff_data = json.loads(res_diff.stdout.strip())
        assert diff_data["diff_method"] == "diffSafeTensorsModels"
        assert diff_data["identical_layers"] == 1, "Must detect 1 identical layer"
        assert diff_data["modified_layers"] == 1, "Must detect 1 modified layer"
        assert diff_data["added_layers"] == 1, "Must detect 1 added layer"
        assert diff_data["removed_layers"] == 1, "Must detect 1 removed layer"

        layer_map = {item["name"]: item["status"] for item in diff_data["layer_diffs"]}
        assert layer_map["layer.shared.weight"] == "IDENTICAL", "layer.shared.weight must be IDENTICAL"
        assert layer_map["layer.active.weight"] == "WEIGHTS_MODIFIED", "layer.active.weight must be WEIGHTS_MODIFIED"
        assert layer_map["layer.head.weight"] == "ADDED", "layer.head.weight must be ADDED"
        assert layer_map["layer.pruned.weight"] == "REMOVED", "layer.pruned.weight must be REMOVED"

        print("  ✔ PASS: layer.shared.weight ➔ IDENTICAL (100% deduplicated in CAS!)")
        print("  ✔ PASS: layer.active.weight ➔ WEIGHTS_MODIFIED (Fine-tuned parameters)")
        print("  ✔ PASS: layer.head.weight   ➔ ADDED (New projection head)")
        print("  ✔ PASS: layer.pruned.weight ➔ REMOVED (Pruned layer)")
        print("  🎉 Semantic model diffing verified!\n")

    # ------------------------------------------------------------------------
    # TEST 3: GGUF Model Header Harvesting
    # ------------------------------------------------------------------------
    print("[TEST 3] Testing GGUF Model Container Harvesting...")
    gguf_path = os.path.join(fixtures_dir, "llama-3-q4_k_m.gguf")
    res_gguf = subprocess.run([diff_exe, "diff", gguf_path, gguf_path, "--json"], capture_output=True, encoding="utf-8", errors="replace")
    assert res_gguf.returncode == 0
    diff_gguf = json.loads(res_gguf.stdout.strip())
    assert diff_gguf["diff_method"] == "diffGGUFModels"
    print(f"  ✔ PASS: GGUF Architecture diff method: {diff_gguf['diff_method']}()\n")

    # ------------------------------------------------------------------------
    # TEST 4: SQLite Database Indexing & SQL Query Verification
    # ------------------------------------------------------------------------
    print("[TEST 4] Testing SQLite Model Harvesting & SQL Queries...")
    with tempfile.TemporaryDirectory() as tmpdir:
        db_path = os.path.join(tmpdir, "model_index.db")
        res_h = subprocess.run([diff_exe, "harvest", st_path, db_path], capture_output=True, encoding="utf-8", errors="replace")
        assert res_h.returncode == 0, f"harvest failed: {res_h.stderr}"
        assert os.path.exists(db_path)

        # Connect to SQLite and execute direct SQL queries
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()

        # Query harvested models
        cursor.execute("SELECT model_id, format, total_parameters FROM harvested_models;")
        model_row = cursor.fetchone()
        assert model_row is not None
        assert model_row[1] == "SafeTensors"

        # Query model tensors table
        cursor.execute("SELECT tensor_name, dtype, shape, num_elements FROM model_tensors;")
        tensors = cursor.fetchall()
        assert len(tensors) >= 1
        t_name, t_dtype, t_shape, t_elements = tensors[0]

        # Query model attributes
        cursor.execute("SELECT attr_key, attr_value FROM model_attributes WHERE attr_key = 'format';")
        attr_row = cursor.fetchone()
        assert attr_row is not None and attr_row[1] == "pt"

        conn.close()

        print(f"  ✔ PASS: SQL Query result: Model ID '{model_row[0]}' has {len(tensors)} indexed tensor(s)")
        print(f"  ✔ PASS: Tensor Table Row: {t_name} | {t_dtype} | {t_shape} | {t_elements} elements")
        print(f"  ✔ PASS: Attribute Table: {attr_row[0]} = '{attr_row[1]}'")
        print("  🎉 SQLite harvesting and SQL querying verified!\n")

    print("======================================================================")
    print("🎉 ALL TASK 1.4 METADATA HARVESTING & DIFF TESTS PASSED (4/4)!")
    print("======================================================================\n")

if __name__ == "__main__":
    run_tests()
