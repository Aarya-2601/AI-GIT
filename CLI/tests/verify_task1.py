#!/usr/bin/env python3
"""
AI-GIT Task 1.1 Verification Test Suite
Executes aigit-inspect against synthetic model, dataset, media, and document fixtures.
"""

import os
import sys
import glob
import json
import subprocess

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

def run_tests():
    cli_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    exe_name = "aigit-inspect.exe" if os.name == "nt" else "aigit-inspect"
    exe_path = os.path.join(cli_dir, exe_name)

    if not os.path.exists(exe_path):
        print(f"❌ Error: {exe_name} not found at {exe_path}. Build it first.")
        sys.exit(1)

    fixtures_dir = os.path.join(cli_dir, "fixtures")
    if not os.path.exists(fixtures_dir):
        print(f"Generating fixtures in {fixtures_dir}...")
        gen_script = os.path.join(cli_dir, "tests", "generate_fixtures.py")
        subprocess.run([sys.executable, gen_script], check=True)

    expected_specs = {
        "model.safetensors": {
            "format": "SafeTensors",
            "category": "ModelWeights",
            "mime_type": "application/x-safetensors"
        },
        "llama-3-q4_k_m.gguf": {
            "format": "GGUF",
            "category": "ModelWeights",
            "mime_type": "application/x-gguf"
        },
        "train_dataset.parquet": {
            "format": "Parquet",
            "category": "Dataset",
            "mime_type": "application/vnd.apache.parquet"
        },
        "image.png": {
            "format": "PNG",
            "category": "Media",
            "mime_type": "image/png"
        },
        "photo.jpg": {
            "format": "JPEG",
            "category": "Media",
            "mime_type": "image/jpeg"
        },
        "video.mp4": {
            "format": "MP4",
            "category": "Media",
            "mime_type": "video/mp4"
        },
        "paper.pdf": {
            "format": "PDF",
            "category": "Document",
            "mime_type": "application/pdf"
        },
        "model_card.txt": {
            "format": "Text",
            "category": "Text",
            "mime_type": "text/plain"
        }
    }

    print("\n========================================================")
    print("      🦎 AI-GIT TASK 1.1 VERIFICATION TEST SUITE        ")
    print("========================================================\n")

    passed = 0
    total = len(expected_specs)

    for filename, expected in expected_specs.items():
        file_path = os.path.join(fixtures_dir, filename)
        if not os.path.exists(file_path):
            print(f"❌ FAIL: Missing fixture {filename}")
            continue

        res = subprocess.run([exe_path, file_path, "--json"], capture_output=True, text=True)
        if res.returncode != 0:
            print(f"❌ FAIL: {filename} exited with code {res.returncode}")
            print(res.stderr)
            continue

        try:
            data = json.loads(res.stdout.strip())
        except Exception as e:
            print(f"❌ FAIL: {filename} output could not be parsed as JSON: {e}")
            print(res.stdout)
            continue

        mismatches = []
        for key, exp_val in expected.items():
            act_val = data.get(key)
            if act_val != exp_val:
                mismatches.append(f"{key}: expected '{exp_val}', got '{act_val}'")

        if mismatches:
            print(f"❌ FAIL: {filename} format mismatch: {', '.join(mismatches)}")
        else:
            method_str = data.get('detection_method', 'unknown')
            print(f"✔ PASS: {filename:<24} ➔ Format: {data['format']:<12} Category: {data['category']:<12} Method: {method_str}()")
            if data['format'] == 'SafeTensors':
                print(f"         └─ Metadata Header: {data['header_length']} B | Payload Offset: {data['payload_offset']} B")
            passed += 1

    print("\n--------------------------------------------------------")
    print(f"Results: {passed}/{total} tests passed ({passed/total*100:.1f}%)")
    print("--------------------------------------------------------\n")

    if passed == total:
        print("🎉 ALL TESTS PASSED! Task 1.1 Zero-Copy Inspection is fully verified.\n")
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == "__main__":
    run_tests()
