import os
import sys
import json
import struct
import math
import time

def generate_7gb_safetensors(output_path: str, target_size_gb: float = 7.0):
    total_target_bytes = int(target_size_gb * 1024 * 1024 * 1024)
    print(f"Generating realistic {target_size_gb:.2f} GB SafeTensors model...")
    print(f"Target size: {total_target_bytes:,} bytes")
    
    # 28 Transformer layers, 6 tensors per layer = 168 tensors
    layers = 28
    tensor_types = [
        ("self_attn.q_proj.weight", [4096, 4096]),
        ("self_attn.k_proj.weight", [4096, 1024]),
        ("self_attn.v_proj.weight", [4096, 1024]),
        ("self_attn.o_proj.weight", [4096, 4096]),
        ("mlp.gate_up_proj.weight", [4096, 14336]),
        ("mlp.down_proj.weight", [14336, 4096]),
    ]
    
    total_tensors = layers * len(tensor_types)
    # Estimate total elements to scale sizes proportionally
    raw_elements = sum(shape[0] * shape[1] for _, shape in tensor_types) * layers
    bytes_per_elem = 4 # Float32
    
    # Pre-build header schema
    current_offset = 0
    header_dict = {"__metadata__": {"format": "pt", "framework": "pytorch", "model_type": "llama"}}
    
    tensors_meta = []
    for l in range(layers):
        for name_suffix, shape in tensor_types:
            t_name = f"model.layers.{l}.{name_suffix}"
            t_elements = shape[0] * shape[1]
            t_bytes = t_elements * bytes_per_elem
            tensors_meta.append((t_name, shape, t_bytes))
            
    # Calculate scale factor to match total_target_bytes exactly
    raw_payload_needed = total_target_bytes - 100000 # approximate for header
    scale = raw_payload_needed / sum(t[2] for t in tensors_meta)
    
    # Finalize offsets
    offset = 0
    final_spans = []
    for t_name, shape, t_bytes in tensors_meta:
        actual_bytes = int(t_bytes * scale)
        # Ensure 4-byte float alignment
        actual_bytes = (actual_bytes // 4) * 4
        if actual_bytes == 0: actual_bytes = 1024
        header_dict[t_name] = {
            "dtype": "F32",
            "shape": shape,
            "data_offsets": [offset, offset + actual_bytes]
        }
        final_spans.append((t_name, offset, offset + actual_bytes))
        offset += actual_bytes
        
    header_json = json.dumps(header_dict, separators=(',', ':')).encode('utf-8')
    header_len = len(header_json)
    
    print(f"Header length: {header_len:,} bytes ({len(header_dict)} keys)")
    
    start_time = time.time()
    block_size = 64 * 1024 * 1024 # 64 MB write buffer
    # Create reusable realistic float block
    num_floats = block_size // 4
    floats_pattern = [math.sin(i * 0.05) * 0.02 for i in range(1024)]
    pattern_bytes = struct.pack(f'<{len(floats_pattern)}f', *floats_pattern)
    reusable_block = pattern_bytes * (block_size // len(pattern_bytes))
    
    written_bytes = 0
    with open(output_path, "wb") as f:
        # 1. Write SafeTensors 8-byte uint64 header length
        f.write(struct.pack("<Q", header_len))
        written_bytes += 8
        # 2. Write SafeTensors JSON header
        f.write(header_json)
        written_bytes += header_len
        
        # 3. Stream binary float tensor payload
        payload_left = offset
        while payload_left > 0:
            to_write = min(block_size, payload_left)
            f.write(reusable_block[:to_write])
            written_bytes += to_write
            payload_left -= to_write
            if written_bytes % (1024 * 1024 * 1024) < block_size:
                print(f"  Progress: {written_bytes / (1024**3):.2f} GB written...")
                
    elapsed = time.time() - start_time
    file_size = os.path.getsize(output_path)
    print(f"Done! Written {file_size:,} bytes ({file_size / (1024**3):.2f} GB) in {elapsed:.2f}s ({file_size / (1024**2) / elapsed:.1f} MB/s)")
    return file_size

if __name__ == "__main__":
    out_file = sys.argv[1] if len(sys.argv) > 1 else r"C:\Users\Keya\Desktop\AI-GIT\CLI\fixtures\model_7gb_benchmark.safetensors"
    generate_7gb_safetensors(out_file, 7.0)

