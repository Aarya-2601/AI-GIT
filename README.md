# AI-GIT: High-Performance Version Control System for AI/ML Models

AI-GIT is a standalone, decentralized Version Control System (VCS) engineered from the ground up in modern **C++17**, complemented by a **Node.js/Express backend**, **MinIO/S3 Content-Addressed Storage (CAS)**, and an **interactive Web Portal** with visual **Commit DAGs** and **Model Hubs**.

Traditional version control tools like **Git LFS** treat machine learning checkpoints as opaque, monolithic binary blobs: a 1% fine-tuning adjustment to a 7GB model checkpoint forces a full re-upload of all 7GB, duplicating storage footprints and saturating network bandwidth. AI-GIT redesigns the VCS architecture around tensor structures, delivering **over 70% storage and bandwidth deduplication** across training epochs.

---

## Key Architectural Innovations

### 1. Tensor-Aware FastCDC (Content-Defined Chunking)
- **Problem**: Fixed-size chunking divides files arbitrarily across floating-point values, creating boundaries that shift unpredictably when weights change.
- **Solution**: AI-GIT parses the structural metadata of **SafeTensors** and **GGUF** formats, aligning chunk cut points with tensor matrix offsets.
- **Mechanism**: A **Gear Matrix rolling hash scanner** scans mapped memory segments (256 KB minimum, 1 MB average, 4 MB maximum). Identical attention and feed-forward layers across checkpoints are recognized and deduplicated automatically.

### 2. Zero-Copy Memory Mapping (`MMapFile`)
- Uses native OS virtual memory mapping (`CreateFileMapping` / `MapViewOfFile` on Windows, POSIX `mmap` on Linux) to stream multi-gigabyte (70B+) model files directly from disk into Content-Addressed Storage.
- Eliminates heap allocation bottlenecks and out-of-memory (`std::bad_alloc`) failures.

### 3. Bitshuffle Transposition & SIMD Delta Engine
- **Bitshuffle Codec**: Raw floating-point weights (FP32, FP16, BF16) possess high entropy that limits conventional compressors. Bitshuffle transposes bit-planes to group identical sign and exponent bits together before running Zstandard compression, significantly enhancing compression ratios.
- **AVX2 SIMD Delta Engine**: For modified tensor slices, vectorized SIMD block matching computes binary deltas, allowing weight reconstruction at memory bandwidth speeds (30–40 GB/s) directly in RAM.

### 4. Dual-Tier Content-Addressed Storage (CAS)
- **Local CAS (`.aigit/objects`)**: Local leaf indexing utilizes **xxHash128** (up to 30 GB/s hashing throughput) to construct cryptographic **SHA-256 Merkle trees**.
- **Remote CAS (`MinIO / S3`)**: The CLI communicates with the backend via presigned URLs and chunk negotiation (`/api/v1/push/negotiate`), ensuring only truly novel chunks are transmitted over the wire.

### 5. Structural Tensor Diffing (`ai-git diff`)
- Replaces uninformative `"Binary files differ"` notifications with ML-specific architectural comparisons:
  - Added or removed attention layers
  - Tensor dimension and shape shifts (e.g., vocabulary expansion)
  - Precision changes (e.g., FP32 to BF16)
  - Exact parameter delta counts and layer-by-layer divergence

### 6. Interactive Web Portal & Visual Training DAG
- Full GitHub/HuggingFace-style dashboard running on Node.js/Express.
- Visual **Commit DAG** displaying branch trees, training epochs, validation loss trajectories, and live deduplication meters.
- Interactive SafeTensors Inspector showing parameter counts, shapes, and FastCDC chunk maps.

---

## End-to-End File Ingestion Lifecycle

```text
[ Model File (.safetensors / .gguf / .bin) ]
                     │
                     ▼
       Core::FileInspector (Format & Header Detection)
                     │
                     ▼
        Core::MMapFile (Zero-Copy Virtual Memory Map)
                     │
         ┌───────────┴────────────────────────┐
         ▼                                    ▼
TensorAwareFastCDC                   MetadataHarvester
(Gear Matrix Rolling Hash)       (Layer Names, Shapes, Dtypes)
         │                                    │
         ▼                                    ▼
Bitshuffle + Zstandard Compression    .aigit/metadata.db (SQLite3)
         │
         ▼
xxHash128 + OpenSSL SHA-256 Merkle Tree
         │
         ▼
.aigit/objects (Local CAS)  <───>  .aigit/index (Staging)
         │
         ▼
   ai-git commit  ===>  Models::TreeNode + Models::Commit (HEAD Update)
         │
         ▼
   ai-git push    ===>  Backend Negotiation (/api/v1/push/negotiate)
         │
         ├──────────────────────────────┐
         ▼                              ▼
MinIO S3 Bucket (aigit-chunks)    Web Portal Sync (/api/models/sync)
(Novel Chunks Uploaded)           (Live DAG, Metrics, SafeTensors View)
```

---

## Technology Stack

| Layer | Technologies |
|---|---|
| **Core CLI Engine** | C++17, MSVC / GCC / Clang, CMake |
| **Cryptography & Hashing** | xxHash128, OpenSSL SHA-256 |
| **I/O & Memory Management** | Windows Virtual Memory APIs / POSIX `mmap` |
| **Compression & Encoding** | Bitshuffle, Zstandard, SIMD (AVX2) Delta Matching |
| **Embedded Metadata** | SQLite3 |
| **Networking & Protocols** | libcurl (CURL_STATICLIB), REST HTTP/JSON |
| **Object Storage (Remote CAS)** | MinIO (Amazon S3 compatible) |
| **Backend API** | Node.js, Express, PostgreSQL, MinIO JavaScript SDK |
| **Web Portal** | Vanilla JS, HTML5, CSS3 (Dark Theme), Canvas / SVG DAG |

---

## Directory Structure

```text
AI-GIT/
├── CLI/                     # C++17 Core Engine & Command-Line Client
│   ├── CMakeLists.txt       # Build system configuration & vcpkg bindings
│   ├── src/
│   │   ├── main.cpp         # Command dispatcher & execution benchmark timer
│   │   ├── chunking/        # Tensor-aware FastCDC & asymmetric chunkers
│   │   ├── commands/        # CLI subcommands (init, add, commit, push, pull, diff, etc.)
│   │   ├── compression/     # Bitshuffle codec & polymorphic compression
│   │   ├── core/            # Memory-mapped I/O, index, refs, filesystem helpers
│   │   ├── diffing/         # Structural differ, metadata harvester, SIMD delta engine
│   │   ├── hashing/         # xxHash128 engine & SHA-256 Merkle root generators
│   │   ├── inspection/      # Format-aware binary file inspectors
│   │   ├── models/          # Blobs, trees, commit models, and object stores
│   │   ├── storage/         # Local CAS object store & SQLite metadata DB
│   │   └── sync/            # Remote CAS sync engine & network streaming
│   ├── tests/               # Test suites, CPU/IO probes, and verification scripts
│   └── tui/                 # Terminal User Interface (Python Textual/Curses)
│
├── BACKEND/                 # Node.js / Express Remote CAS Service
│   ├── src/
│   │   ├── index.js         # REST endpoints for chunk negotiation & health
│   │   ├── config/          # PostgreSQL & MinIO connection configurations
│   │   └── services/        # MinIO presigned URL generator & bucket management
│   ├── package.json
│   └── .env.example
│
├── FRONTEND/                # Interactive Web Application & Observability Portal
│   ├── server.js            # Express portal server & live model sync API
│   ├── data/                # Seed models, issues, discussions, and user registries
│   └── public/              # Web assets: Dashboard, Model Hub, Visual DAG, Profile Studio
│
├── generate_7gb_model.py    # Synthetic 7GB SafeTensors benchmark generator
└── README.md                # Project documentation
```

---

## Prerequisites & Installation

### 1. Build Requirements
- **C++ Compiler**: MSVC 2019+ (Windows) or GCC 11+ / Clang 13+ (Linux/macOS) supporting C++17.
- **CMake**: Version 3.15 or newer.
- **vcpkg**: C++ library package manager.
- **Node.js**: Version 18.x or newer (for Backend & Frontend).
- **MinIO Server** (optional for local CAS testing).

### 2. Building the C++ CLI

#### Windows (PowerShell)
```powershell
# Clone repository
git clone https://github.com/Aarya-2601/AI-GIT.git
cd AI-GIT/CLI

# Configure with vcpkg
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="<path_to_vcpkg>/scripts/buildsystems/vcpkg.cmake"

# Build Release binary
cmake --build build --config Release --target ai-git
```
The compiled executable will be located at `CLI/build/Release/ai-git.exe`.

---

## Quickstart Guide

### 1. Initialize a Repository
```bash
ai-git init
```
This generates the `.aigit` directory containing the staging index, configuration, and object store.

### 2. Stage Model Checkpoints
```bash
ai-git add model.safetensors
```
Outputs chunk counts, layer metadata format, and the computed Merkle root.

### 3. Commit Checkpoint
```bash
ai-git commit -m "feat: epoch 1 checkpoint with LoRA adapter"
```

### 4. Inspect Model Architecture & Differences
```bash
# Structural inspection of weights
ai-git inspect model.safetensors

# Deep architectural diff between two model checkpoints
ai-git diff model_epoch1.safetensors model_epoch2.safetensors
```

### 5. Push to Remote CAS & Sync with Web Portal
```bash
ai-git push
```
Outputs the FastCDC space savings and synchronizes the model lineage directly to the web portal.

---

## Running the Web Portal & Remote CAS

### 1. Start MinIO Object Store
```powershell
# Using the preconfigured batch script (or via MinIO binary)
C:\minio\start-minio.bat
```
- **S3 API Endpoint**: `http://localhost:9000`
- **Web Console**: `http://localhost:9001` (Credentials: `aigit_minio` / `aigiteam`)

### 2. Start the Backend Negotiation Service
```bash
cd BACKEND
npm install
npm start
```
Runs on `http://localhost:3000`.

### 3. Start the Web Portal
```bash
cd FRONTEND
npm install
node server.js
```
Open **`http://localhost:3001`** in your browser to access:
- **Model Hub**: Browse models, parameter sizes, and frameworks.
- **Visual Commit DAG**: Inspect training checkpoints, epoch loss trajectories, and parent linkages.
- **SafeTensors Inspector**: View internal layer matrices, shapes, and deduplication statistics.
- **Profile Studio**: Automated README and research metric generation.

---

## Performance Benchmarks

On an end-to-end benchmark using a synthetic **7.00 GB Llama-style SafeTensors model**:

| Metric | Git LFS | AI-GIT | Improvement |
|---|---|---|---|
| **Storage per Delta Epoch** | 7.00 GB | 1.82 GB | **74.6% Space Saved** |
| **Network Transfer per Push** | 7.00 GB | 1.82 GB | **3.85x Throughput Gain** |
| **Chunk Hashing Throughput** | ~450 MB/s (SHA-1) | 30+ GB/s (xxHash128) | **66x Acceleration** |
| **Weight Reconstruction** | Re-download full file | 30–40 GB/s (SIMD RAM copy) | **Instant Checkout** |

---

## License

This project is licensed under the MIT License.
