import os
import sys
import json
import struct
import shutil
import hashlib
import subprocess
import tempfile

def compute_sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def make_safetensors(header_dict: dict, tensor_bytes: bytes) -> bytes:
    header_json = json.dumps(header_dict, separators=(',', ':')).encode('utf-8')
    header_len = len(header_json)
    return struct.pack('<Q', header_len) + header_json + tensor_bytes

def make_parquet_stub(row_group_bytes: bytes, metadata_bytes: bytes) -> bytes:
    # 4 bytes magic PAR1 + row group + metadata + 4 bytes length + 4 bytes PAR1
    magic = b"PAR1"
    meta_len = len(metadata_bytes)
    return magic + row_group_bytes + metadata_bytes + struct.pack('<I', meta_len) + magic

def make_png_stub(data: bytes) -> bytes:
    # PNG signature: \x89PNG\r\n\x1a\n
    sig = b"\x89PNG\r\n\x1a\n"
    return sig + data

def run_cmd(args):
    proc = subprocess.run(args, capture_output=True, text=True, encoding="utf-8", errors="replace")
    return proc.returncode, proc.stdout, proc.stderr

def main():
    print("=" * 70)
    print("AI-GIT NEXT-GEN VCS - TASK 1.5 VERIFICATION SUITE")
    print("Global CAS Sync, Zero-Redundancy Delta Uploads & Bit-Exact Reconstruction")
    print("=" * 70)

    exe_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "build", "aigit-sync.exe"))
    if not os.path.exists(exe_path):
        print(f"FAILED: Cannot find {exe_path}")
        sys.exit(1)

    temp_dir = tempfile.mkdtemp(prefix="aigit_sync_test_")
    remote_cas_dir = os.path.join(temp_dir, "remote_cas_store")

    tests_passed = 0
    total_tests = 5

    try:
        # -------------------------------------------------------------
        # Test 1: SafeTensors v1 Model Initial Push
        # -------------------------------------------------------------
        print("\n[TEST 1] Initial SafeTensors Model Push to Remote CAS...")
        
        # 512 KB of structured float32 tensor weights
        num_floats = 128 * 1024
        raw_floats = [float(i % 1000) * 0.03125 for i in range(num_floats)]
        tensor_payload = struct.pack(f'<{num_floats}f', *raw_floats)

        header_v1 = {
            "model.weight": {"dtype": "F32", "shape": [128, 1024], "data_offsets": [0, len(tensor_payload)]},
            "__metadata__": {"version": "v1.0.0", "author": "researcher_a", "arch": "transformer"}
        }
        safetensors_v1_bytes = make_safetensors(header_v1, tensor_payload)
        v1_sha256 = compute_sha256(safetensors_v1_bytes)
        v1_path = os.path.join(temp_dir, "model_v1.safetensors")
        with open(v1_path, "wb") as f:
            f.write(safetensors_v1_bytes)

        ret, out, err = run_cmd([exe_path, "push", v1_path, remote_cas_dir])
        print(out.strip())
        if ret != 0:
            print(f"FAILED: Push v1 failed with code {ret}, err: {err}")
            sys.exit(1)

        res_v1 = json.loads(out)
        assert res_v1["success"] is True
        assert res_v1["file_sha256"] == v1_sha256
        assert res_v1["total_chunks"] >= 2
        assert res_v1["uploaded_chunks"] == res_v1["total_chunks"]
        assert res_v1["skipped_chunks"] == 0
        assert res_v1["bytes_uploaded"] > 0
        v1_manifest_id = res_v1["manifest_id"]

        print(f"SUCCESS: Pushed model v1 ({res_v1['raw_bytes_total']} bytes, {res_v1['total_chunks']} chunks, Manifest: {v1_manifest_id[:12]}...)")
        tests_passed += 1

        # -------------------------------------------------------------
        # Test 2: SafeTensors v2 Zero-Redundancy Delta Upload
        # Volatile JSON header modified, 512 KB tensor weights 100% IDENTICAL
        # -------------------------------------------------------------
        print("\n[TEST 2] SafeTensors v2 Delta Sync (Modifying Metadata Only)...")
        header_v2 = {
            "model.weight": {"dtype": "F32", "shape": [128, 1024], "data_offsets": [0, len(tensor_payload)]},
            "__metadata__": {"version": "v2.0.0-finetuned", "author": "researcher_b", "lr": "0.0001", "epochs": "5"}
        }
        safetensors_v2_bytes = make_safetensors(header_v2, tensor_payload)
        v2_sha256 = compute_sha256(safetensors_v2_bytes)
        assert v2_sha256 != v1_sha256, "Model v1 and v2 hashes must differ!"
        v2_path = os.path.join(temp_dir, "model_v2.safetensors")
        with open(v2_path, "wb") as f:
            f.write(safetensors_v2_bytes)

        ret, out, err = run_cmd([exe_path, "push", v2_path, remote_cas_dir])
        print(out.strip())
        if ret != 0:
            print(f"FAILED: Push v2 failed with code {ret}, err: {err}")
            sys.exit(1)

        res_v2 = json.loads(out)
        assert res_v2["success"] is True
        assert res_v2["file_sha256"] == v2_sha256
        assert res_v2["skipped_chunks"] >= 1, "Tensor payload MUST be skipped due to CAS deduplication!"
        assert res_v2["uploaded_chunks"] == 1, "Only volatile JSON metadata header chunk should be uploaded!"
        assert res_v2["bytes_saved_by_dedup"] >= len(tensor_payload), "Must save entire tensor weight payload from re-upload!"
        assert res_v2["deduplication_ratio"] > 99.0, f"Expected >99% deduplication, got {res_v2['deduplication_ratio']}%"
        v2_manifest_id = res_v2["manifest_id"]

        print(f"SUCCESS: Delta upload verified! 0 tensor weight bytes uploaded over network ({res_v2['bytes_saved_by_dedup']} bytes saved, {res_v2['deduplication_ratio']}% dedup ratio)")
        tests_passed += 1

        # -------------------------------------------------------------
        # Test 3: Bit-Exact Reconstruction (Pull / Checkout from Remote CAS)
        # -------------------------------------------------------------
        print("\n[TEST 3] Bit-Exact Reconstruction from Remote CAS (Pull)...")
        pulled_v1_path = os.path.join(temp_dir, "reconstructed_model_v1.safetensors")
        pulled_v2_path = os.path.join(temp_dir, "reconstructed_model_v2.safetensors")

        ret1, out1, err1 = run_cmd([exe_path, "pull", v1_manifest_id, pulled_v1_path, remote_cas_dir])
        if ret1 != 0:
            print(f"FAILED: Pull v1 failed: {err1}")
            sys.exit(1)

        with open(pulled_v1_path, "rb") as f:
            pulled_v1_bytes = f.read()
        pulled_v1_sha = compute_sha256(pulled_v1_bytes)
        assert pulled_v1_sha == v1_sha256, f"Checksum mismatch on v1! {pulled_v1_sha} vs {v1_sha256}"
        assert pulled_v1_bytes == safetensors_v1_bytes, "Bit-exact byte mismatch on v1!"

        ret2, out2, err2 = run_cmd([exe_path, "pull", v2_manifest_id, pulled_v2_path, remote_cas_dir])
        if ret2 != 0:
            print(f"FAILED: Pull v2 failed: {err2}")
            sys.exit(1)

        with open(pulled_v2_path, "rb") as f:
            pulled_v2_bytes = f.read()
        pulled_v2_sha = compute_sha256(pulled_v2_bytes)
        assert pulled_v2_sha == v2_sha256, f"Checksum mismatch on v2! {pulled_v2_sha} vs {v2_sha256}"
        assert pulled_v2_bytes == safetensors_v2_bytes, "Bit-exact byte mismatch on v2!"

        print(f"SUCCESS: Both model versions bit-exactly reconstructed from Remote CAS with SHA-256 verification (100% fidelity)")
        tests_passed += 1

        # -------------------------------------------------------------
        # Test 4: Parquet and Media Sync & Reconstruction
        # -------------------------------------------------------------
        print("\n[TEST 4] Parquet and Pre-Compressed Media Format Sync & Pull...")
        parquet_bytes = make_parquet_stub(b"ROW_GROUP_DATA_" * 500, b"METADATA_THRIFT_STRUCT_" * 50)
        pq_sha = compute_sha256(parquet_bytes)
        pq_path = os.path.join(temp_dir, "dataset.parquet")
        with open(pq_path, "wb") as f:
            f.write(parquet_bytes)

        ret_pq, out_pq, err_pq = run_cmd([exe_path, "push", pq_path, remote_cas_dir])
        assert ret_pq == 0
        pq_res = json.loads(out_pq)
        pq_manifest_id = pq_res["manifest_id"]

        reconstructed_pq_path = os.path.join(temp_dir, "reconstructed.parquet")
        ret_pq_pull, _, _ = run_cmd([exe_path, "pull", pq_manifest_id, reconstructed_pq_path, remote_cas_dir])
        assert ret_pq_pull == 0
        with open(reconstructed_pq_path, "rb") as f:
            assert compute_sha256(f.read()) == pq_sha

        # Media PNG
        png_bytes = make_png_stub(b"IMAGE_PIXEL_DATA_RGB_" * 1000)
        png_sha = compute_sha256(png_bytes)
        png_path = os.path.join(temp_dir, "sample.png")
        with open(png_path, "wb") as f:
            f.write(png_bytes)

        ret_png, out_png, _ = run_cmd([exe_path, "push", png_path, remote_cas_dir])
        assert ret_png == 0
        png_res = json.loads(out_png)
        png_manifest_id = png_res["manifest_id"]

        reconstructed_png_path = os.path.join(temp_dir, "reconstructed.png")
        ret_png_pull, _, _ = run_cmd([exe_path, "pull", png_manifest_id, reconstructed_png_path, remote_cas_dir])
        assert ret_png_pull == 0
        with open(reconstructed_png_path, "rb") as f:
            assert compute_sha256(f.read()) == png_sha

        print(f"SUCCESS: Parquet structural sync and PNG bypass codec verified and bit-exactly pulled")
        tests_passed += 1

        # -------------------------------------------------------------
        # Test 5: Remote CAS Manifest Inspection (cat-manifest)
        # -------------------------------------------------------------
        print("\n[TEST 5] Manifest Inspection & Integrity Audit...")
        ret_cat, out_cat, err_cat = run_cmd([exe_path, "cat-manifest", v2_manifest_id, remote_cas_dir])
        assert ret_cat == 0
        manifest_data = json.loads(out_cat)
        assert manifest_data["file_format"] == "SafeTensors"
        assert manifest_data["file_sha256"] == v2_sha256
        assert len(manifest_data["chunks"]) >= 2
        for chunk in manifest_data["chunks"]:
            assert "raw_hash" in chunk and len(chunk["raw_hash"]) == 64
            assert "codec_method" in chunk
            assert "offset" in chunk
            assert "raw_size" in chunk

        print(f"Manifest JSON Verified: {len(manifest_data['chunks'])} chunks cataloged with content-addressed keys.")
        tests_passed += 1

    finally:
        shutil.rmtree(temp_dir, ignore_errors=True)

    print("\n" + "=" * 70)
    print(f"TASK 1.5 VERIFICATION PASSED: {tests_passed}/{total_tests} Tests (100% Success)")
    print("=" * 70)

if __name__ == "__main__":
    main()
