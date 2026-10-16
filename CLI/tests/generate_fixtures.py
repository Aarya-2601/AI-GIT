import os
import struct
import json

def generate_fixtures(out_dir="fixtures"):
    os.makedirs(out_dir, exist_ok=True)

    # 1. SafeTensors
    metadata = {
        "model.layers.0.mlp.weight": {
            "dtype": "F16",
            "shape": [4096, 14336],
            "data_offsets": [0, 117440512]
        },
        "__metadata__": {
            "format": "pt",
            "model_type": "llama",
            "framework": "SafeTensors"
        }
    }
    json_bytes = json.dumps(metadata, separators=(',', ':')).encode('utf-8')
    header_len = len(json_bytes)
    # SafeTensors format: 8 bytes LE uint64 length + UTF-8 JSON + tensor bytes
    dummy_tensor = b"\x00\x3c" * 256  # 512 bytes dummy FP16 weights
    safetensors_path = os.path.join(out_dir, "model.safetensors")
    with open(safetensors_path, "wb") as f:
        f.write(struct.pack("<Q", header_len))
        f.write(json_bytes)
        f.write(dummy_tensor)
    print(f"Created: {safetensors_path} (Header len: {header_len}, Total: {os.path.getsize(safetensors_path)} bytes)")

    # 2. GGUF
    gguf_magic = b"GGUF"
    gguf_version = struct.pack("<I", 3)
    gguf_path = os.path.join(out_dir, "llama-3-q4_k_m.gguf")
    with open(gguf_path, "wb") as f:
        f.write(gguf_magic + gguf_version + b"\x00" * 128)
    print(f"Created: {gguf_path}")

    # 3. Parquet
    parquet_path = os.path.join(out_dir, "train_dataset.parquet")
    with open(parquet_path, "wb") as f:
        f.write(b"PAR1" + b"\x01\x02\x03\x04" * 32 + b"PAR1")
    print(f"Created: {parquet_path}")

    # 4. PNG
    png_path = os.path.join(out_dir, "image.png")
    with open(png_path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + b"\x00\x00\x00\rIHDR" + b"\x00" * 16)
    print(f"Created: {png_path}")

    # 5. JPEG
    jpg_path = os.path.join(out_dir, "photo.jpg")
    with open(jpg_path, "wb") as f:
        f.write(b"\xff\xd8\xff\xe0\x00\x10JFIF\x00\x01\x01\x00" + b"\x00" * 32)
    print(f"Created: {jpg_path}")

    # 6. MP4
    mp4_path = os.path.join(out_dir, "video.mp4")
    with open(mp4_path, "wb") as f:
        # 4-byte box size + 'ftyp' + 'mp42'
        f.write(struct.pack(">I", 24) + b"ftypisom" + b"\x00\x00\x02\x00" + b"isommp42")
    print(f"Created: {mp4_path}")

    # 7. PDF
    pdf_path = os.path.join(out_dir, "paper.pdf")
    with open(pdf_path, "wb") as f:
        f.write(b"%PDF-1.7\n1 0 obj\n<< /Type /Catalog >>\nendobj\n%%EOF")
    print(f"Created: {pdf_path}")

    # 8. Plaintext
    txt_path = os.path.join(out_dir, "model_card.txt")
    with open(txt_path, "w", encoding="utf-8") as f:
        f.write("# Model Card\nLlama-3 fine-tuned with LoRA on code instruct.\n")
    print(f"Created: {txt_path}")

    print("\nAll fixtures generated successfully!")

if __name__ == "__main__":
    generate_fixtures()
