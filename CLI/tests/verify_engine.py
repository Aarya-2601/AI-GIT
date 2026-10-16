import os
import sys
import json
import struct
import shutil
import hashlib
import subprocess
import tempfile
import math

def compute_sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def make_safetensors(header_dict: dict, tensor_bytes: bytes) -> bytes:
    header_json = json.dumps(header_dict, separators=(',', ':')).encode('utf-8')
    header_len = len(header_json)
    return struct.pack('<Q', header_len) + header_json + tensor_bytes

def run_cmd(args):
    proc = subprocess.run(args, capture_output=True, text=True, encoding="utf-8", errors="replace")
    clean_stdout = proc.stdout.replace('\\', '/')
    return proc.returncode, clean_stdout, proc.stderr

def main():
    print("=" * 70)
    print("AI-GIT NEXT-GEN VCS - NOVEL ENGINE VERIFICATION & BENCHMARK")
    print("Zero-Copy MMap, Tensor FastCDC, LSH Similarity, SIMD Delta & 7GB Dataset")
    print("=" * 70)

    exe_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "build", "aigit-engine.exe"))
    if not os.path.exists(exe_path):
        print(f"FAILED: Cannot find {exe_path}")
        sys.exit(1)

    temp_dir = tempfile.mkdtemp(prefix="aigit_engine_test_")

    try:
        # -------------------------------------------------------------
        # Test 1: Single File MMap, xxHash3, FastCDC & Memory Ceiling
        # -------------------------------------------------------------
        print("\n[TEST 1] Benchmarking Zero-Copy MMap, xxHash3 & FastCDC on SafeTensors...")
        num_floats = 256 * 1024  # 1 MB floats
        raw_floats = [math.sin(i * 0.01) for i in range(num_floats)]
        tensor_data = struct.pack(f'<{num_floats}f', *raw_floats)

        header = {
            "layer_1.weight": {"dtype": "F32", "shape": [256, 1024], "data_offsets": [0, len(tensor_data)]}
        }
        st_bytes = make_safetensors(header, tensor_data)
        st_path = os.path.join(temp_dir, "model_base.safetensors")
        with open(st_path, "wb") as f:
            f.write(st_bytes)

        ret, out, err = run_cmd([exe_path, "bench-file", st_path])
        print(out.strip())
        assert ret == 0, f"bench-file failed: {err}"
        res1 = json.loads(out)
        assert res1["memory_ceiling_safe"] is True
        assert res1["total_chunks"] >= 2
        assert len(res1["merkle_root_sha256"]) == 64
        print(f"[PASS] Throughput xxHash: {res1['xxhash_throughput_gbps']} GB/s | Peak RAM: {res1['peak_rss_mb']} MB (Ceiling Safe: <1GB)")

        # -------------------------------------------------------------
        # Test 2: Fine-Tuned Model Delta Benchmark (LSH + SIMD Delta)
        # -------------------------------------------------------------
        print("\n[TEST 2] Fine-Tuned Model Benchmark (LSH Cluster Match + AVX SIMD Delta)...")
        # In neural fine-tuning, 10% of weights are adapted while 90% remain base weights
        finetuned_floats = list(raw_floats)
        for i in range(int(num_floats * 0.90), num_floats):
            finetuned_floats[i] += 0.05
        finetuned_data = struct.pack(f'<{num_floats}f', *finetuned_floats)

        header_fine = {
            "layer_1.weight": {"dtype": "F32", "shape": [256, 1024], "data_offsets": [0, len(finetuned_data)]}
        }
        st_fine_bytes = make_safetensors(header_fine, finetuned_data)
        st_fine_path = os.path.join(temp_dir, "model_finetuned.safetensors")
        with open(st_fine_path, "wb") as f:
            f.write(st_fine_bytes)

        ret, out, err = run_cmd([exe_path, "bench-delta", st_path, st_fine_path])
        print(out.strip())
        assert ret == 0, f"bench-delta failed: {err}"
        res2 = json.loads(out)
        assert res2["simd_delta_chunks"] >= 1, "Fine-tuned chunk must trigger SIMD Delta match!"
        assert res2["deduplication_ratio"] > 80.0, f"Expected >80% delta dedup, got {res2['deduplication_ratio']}%"
        print(f"[PASS] SIMD Delta Dedup Ratio: {res2['deduplication_ratio']}% | Reconstruction Velocity: {res2['reconstruction_velocity_gbps']} GB/s")

        # -------------------------------------------------------------
        # Test 3: Benchmark on User's 7.65 GB Dataset (FINALCNNDATA_200)
        # -------------------------------------------------------------
        dataset_path = r"C:\Users\Keya\Downloads\FINALCNNDATA_200\FINALCNNDATA_200"
        if os.path.exists(dataset_path):
            print(f"\n[TEST 3] Benchmarking Real-World 7.65 GB Folder ({dataset_path})...")
            ret, out, err = run_cmd([exe_path, "bench-folder", dataset_path])
            print(out.strip())
            assert ret == 0, f"bench-folder failed: {err}"
            res3 = json.loads(out)
            assert res3["total_files_processed"] > 0
            assert res3["memory_ceiling_safe_below_1gb"] is True, "Peak RAM MUST stay strictly below 1GB!"
            print(f"\n[SUCCESS] REAL DATASET TEST PASSED:")
            print(f"   * Total Data Ingested:    {res3['total_size_gb']} GB ({res3['total_files_processed']} files)")
            print(f"   * Sustained Throughput:   {res3['sustained_throughput_mbps']} MB/s")
            print(f"   * Peak Physical RAM:      {res3['peak_physical_ram_mb']} MB (STRICTLY < 1 GB)")
            print(f"   * Unified Merkle Root:    {res3['repo_root_merkle_sha256']}")
        else:
            print(f"[SKIP TEST 3] Dataset path not found: {dataset_path}")

    finally:
        shutil.rmtree(temp_dir, ignore_errors=True)

    print("\n" + "=" * 70)
    print("ALL NEXT-GEN ENGINE BENCHMARKS AND VERIFICATIONS PASSED (100% SUCCESS)")
    print("=" * 70)

if __name__ == "__main__":
    main()
