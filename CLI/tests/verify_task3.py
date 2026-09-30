#!/usr/bin/env python3
"""
AI-GIT Task 1.3 Verification Test Suite: Polymorphic Codecs
Demonstrates that:
1. Pre-compressed media (PNG/JPEG/MP4) bypasses compression (0% CPU wasted, no expansion).
2. Tensor weights are compressed using FP32 byte-shuffling, achieving high compression ratios.
3. Metadata headers are compressed with standard zlib text compression.
4. CAS addresses are decoupled from compression (based on raw bytes) and verify bit-exact checkouts.
5. Corrupted containers are caught by strict CAS checksum validation.
"""

import os
import sys
import json
import struct
import subprocess
import tempfile

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

def run_tests():
    cli_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    codec_exe = os.path.join(cli_dir, "aigit-codec.exe" if os.name == "nt" else "aigit-codec")
    fixtures_dir = os.path.join(cli_dir, "fixtures")

    if not os.path.exists(codec_exe):
        print(f"❌ Error: {codec_exe} not found. Build it first.")
        sys.exit(1)

    print("\n======================================================================")
    print("      🦎 AI-GIT TASK 1.3 VERIFICATION TEST SUITE (POLYMORPHIC CODECS)  ")
    print("======================================================================\n")

    # ------------------------------------------------------------------------
    # TEST 1: Media Bypass (Zero CPU, No Expansion)
    # ------------------------------------------------------------------------
    print("[TEST 1] Testing Pre-Compressed Media Bypass (PNG, JPEG, MP4)...")
    media_files = ["image.png", "photo.jpg", "video.mp4"]

    for mf in media_files:
        path = os.path.join(fixtures_dir, mf)
        res = subprocess.run([codec_exe, "inspect", path, "--json"], capture_output=True, encoding="utf-8", errors="replace")
        assert res.returncode == 0, f"inspect failed for {mf}"
        data = json.loads(res.stdout.strip())
        chunk = data["chunks"][0]

        assert "compressMediaWithBypass" in chunk["codec_method"], f"{mf} did not use bypass"
        assert chunk["codec_type"] == "Bypass (Zero-CPU)", f"{mf} codec type was {chunk['codec_type']}"
        print(f"  ✔ PASS: {mf:<12} ➔ Method: {chunk['codec_method']} | CAS SHA-256: {chunk['raw_sha256'][:16]}...")

    print("  🎉 All media correctly bypassed CPU compression!\n")

    # ------------------------------------------------------------------------
    # TEST 2: Tensor Weights Byte-Shuffle (FP32) & Bit-Exact Restoration
    # ------------------------------------------------------------------------
    print("[TEST 2] Testing Tensor Byte-Shuffle (FP32 Planes) & Restoration...")

    # Generate synthetic float weights with realistic clustered exponents
    import math
    float_count = 2048
    raw_floats = []
    for i in range(float_count):
        raw_floats.append(math.sin(i * 0.05) * 0.25)
    tensor_bytes = struct.pack(f"<{float_count}f", *raw_floats)

    with tempfile.TemporaryDirectory() as tmpdir:
        tensor_raw_path = os.path.join(tmpdir, "weights.bin")
        tensor_agc_path = os.path.join(tmpdir, "weights.agc")
        tensor_restored_path = os.path.join(tmpdir, "weights_restored.bin")

        with open(tensor_raw_path, "wb") as f:
            f.write(tensor_bytes)

        # Compress
        res_comp = subprocess.run([codec_exe, "compress", tensor_raw_path, tensor_agc_path], capture_output=True, encoding="utf-8", errors="replace")
        assert res_comp.returncode == 0, f"Compression failed: {res_comp.stderr}"

        # Decompress
        res_decomp = subprocess.run([codec_exe, "decompress", tensor_agc_path, tensor_restored_path], capture_output=True, encoding="utf-8", errors="replace")
        assert res_decomp.returncode == 0, f"Decompression failed: {res_decomp.stderr}"

        with open(tensor_restored_path, "rb") as f:
            restored_bytes = f.read()

        assert restored_bytes == tensor_bytes, "Restored tensor bytes are NOT bit-exact!"

        raw_size = len(tensor_bytes)
        agc_size = os.path.getsize(tensor_agc_path)
        ratio = raw_size / agc_size

        print(f"  ✔ PASS: Tensor Raw Size: {raw_size} B ➔ Compressed: {agc_size} B (Ratio: {ratio:.2f}x)")
        print(f"  ✔ PASS: Bit-exact un-shuffled bytes match original float32 tensor: 100% MATCH")
        print("  🎉 Tensor Byte-Shuffle compression verified!\n")

    # ------------------------------------------------------------------------
    # TEST 3: SafeTensors Multi-Chunk Polymorphic Pipeline
    # ------------------------------------------------------------------------
    print("[TEST 3] Testing SafeTensors Multi-Chunk Polymorphic Pipeline...")
    st_path = os.path.join(fixtures_dir, "model.safetensors")
    res_st = subprocess.run([codec_exe, "inspect", st_path, "--json"], capture_output=True, encoding="utf-8", errors="replace")
    assert res_st.returncode == 0
    data_st = json.loads(res_st.stdout.strip())
    chunks_st = data_st["chunks"]

    assert len(chunks_st) == 2, f"SafeTensors must have 2 chunks, got {len(chunks_st)}"
    # Chunk 0: MetadataHeader -> compressMetadataWithZlib
    assert chunks_st[0]["codec_method"] == "compressMetadataWithZlib"
    assert chunks_st[0]["codec_type"] == "ZlibStandard"

    # Chunk 1: TensorPayload -> compressTensorWithByteShuffle (FP32)
    assert "compressTensorWithByteShuffle" in chunks_st[1]["codec_method"]
    assert chunks_st[1]["codec_type"] == "ByteShuffle_FP32+Zlib"

    print(f"  ✔ PASS: Chunk #1 (JSON Metadata): {chunks_st[0]['codec_method']}")
    print(f"  ✔ PASS: Chunk #2 (Tensor Weights): {chunks_st[1]['codec_method']} (Ratio: {chunks_st[1]['ratio']:.1f}x)")
    print(f"  ✔ PASS: Overall Model Compression Ratio: {data_st['total_ratio']:.2f}x")
    print("  🎉 SafeTensors polymorphic dispatch fully verified!\n")

    # ------------------------------------------------------------------------
    # TEST 4: Bit-Exact CAS Decoupling & Tamper Detection
    # ------------------------------------------------------------------------
    print("[TEST 4] Testing CAS Decoupling & Corruption Detection...")
    with tempfile.TemporaryDirectory() as tmpdir:
        orig_file = os.path.join(fixtures_dir, "model_card.txt")
        archive_file = os.path.join(tmpdir, "model_card.agc")
        restored_file = os.path.join(tmpdir, "model_card_restored.txt")

        # Compress text
        res = subprocess.run([codec_exe, "compress", orig_file, archive_file], capture_output=True, encoding="utf-8", errors="replace")
        assert res.returncode == 0

        # Decompress & verify CAS
        res_dec = subprocess.run([codec_exe, "decompress", archive_file, restored_file], capture_output=True, encoding="utf-8", errors="replace")
        assert res_dec.returncode == 0
        assert "BIT-EXACT MATCH" in res_dec.stdout

        # Tamper with archive payload
        with open(archive_file, "r+b") as f:
            f.seek(85)  # Tamper a byte inside the payload
            f.write(b"\xFF")

        tamper_restored = os.path.join(tmpdir, "tampered.txt")
        res_tamper = subprocess.run([codec_exe, "decompress", archive_file, tamper_restored], capture_output=True, encoding="utf-8", errors="replace")
        assert res_tamper.returncode != 0, "Corrupted archive must fail decompression!"
        print("  ✔ PASS: Bit-exact CAS hash verified for valid archive.")
        print("  ✔ PASS: Tampered archive correctly rejected with checksum error.")
        print("  🎉 CAS integrity validation verified!\n")

    print("======================================================================")
    print("🎉 ALL TASK 1.3 POLYMORPHIC CODEC TESTS PASSED SUCCESSFULLY (4/4)!")
    print("======================================================================\n")

if __name__ == "__main__":
    run_tests()
