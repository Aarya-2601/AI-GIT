# AI-GIT: Distributed Version Control for AI/ML Models

AI-GIT is a specialized, high-performance distributed version control system architected specifically for AI/ML models, datasets, and large binary artifacts. Unlike traditional VCS systems optimized primarily for small text files, AI-GIT pairs a native high-throughput C++ client with a distributed Content-Addressable Storage (CAS) backend and an interactive web exploration portal.

---

## Architecture Overview

```
                   +---------------------------------------+
                   |           AI-GIT Web Portal           |
                   |   (Node.js / Express / Dynamic UI)    |
                   +-------------------+-------------------+
                                       |
                                       v
+-------------------+      HTTP Sync & Negotiation       +-------------------+
|    AI-GIT CLI     | <================================> |   AI-GIT Backend  |
|  (C++20 Client)   |                                    | (Express API CAS) |
+---------+---------+                                    +---------+---------+
          |                                                        |
          v                                                        v
+-------------------+                                    +-------------------+
|  Local SQLite DB  |                                    |    MinIO Object   |
|   & Local CAS     |                                    |   Store (S3 CAS)  |
+-------------------+                                    +-------------------+
```

---

## Technology Stack

### 1. Client Core (CLI)
* **Language & Standard**: C++20 (`CMake` build system)
* **Package Management**: `vcpkg`
* **Cryptographic Hashing**: `OpenSSL` (SHA-256 for deterministic Content-Addressable Storage IDs)
* **Compression**: `zlib` (Chunk & object payload compression)
* **Local Metadata & Staging**: `SQLite3` (Embedded database for staging index, local object manifest, and branch refs)
* **Networking & Transfer**: `libcurl` (Resilient streaming HTTP client for remote CAS synchronization)
* **Serialization**: `nlohmann/json` (Manifest and API payload interchange)

### 2. Remote CAS Backend
* **Runtime**: Node.js & Express.js
* **Distributed Object Store**: `MinIO` (S3-compatible Object Storage for large model chunks and manifests)
* **Metadata Database**: `PostgreSQL` / Structured JSON data stores
* **Core Endpoints**:
  * Chunk uploads and streaming downloads (`/objects/:id`)
  * Repository registration and metadata inspection (`/repos`)
  * Multi-branch push and pull negotiation

### 3. Web Portal (Frontend)
* **Server**: Express.js server with session management (`express-session`) and CORS
* **Interface**: Modern, dark-themed responsive single-page web portal
* **Key Features**:
  * **Dashboard**: Model registry overview and storage savings metrics
  * **Repository Explorer**: Directory tree inspection with chunk map visualization
  * **Commit DAG**: Visual DAG representation of commit graphs across branches
  * **Issues & Checkpoints**: Dedicated AI model issue tracker and checkpoint bug reporting
  * **Community & Discussions**: Model card discussions and researcher collaboration
  * **Profile Studio**: AI researcher profile management and automated README generator

---

## Repository Structure

```
AI-GIT/
├── CLI/                     # C++20 Client Application
│   ├── CMakeLists.txt       # CMake build definitions
│   ├── vcpkg.json           # vcpkg package manifest (curl, sqlite3, zlib, openssl)
│   └── src/
│       ├── main.cpp         # Command dispatcher
│       ├── commands/        # CLI commands (init, add, commit, push, pull, etc.)
│       ├── core/            # Core models (blobs, trees, commits, repository state)
│       ├── storage/         # CAS storage manager, chunking, and compression
│       ├── helpers/         # Utility functions and string formatting
│       └── RemoteCASClient/ # HTTP client communicating with the backend API
│
├── BACKEND/                 # Node.js CAS Backend
│   ├── package.json         # Express, MinIO, pg dependencies
│   ├── src/                 # REST API routes and CAS storage adapters
│   └── data/                # Local repository metadata and definitions
│
├── FRONTEND/                # Web Portal
│   ├── server.js            # Express server hosting the portal
│   ├── package.json         # Frontend server dependencies
│   └── public/              # Web application views, styles, and client scripts
│
├── DEPLOY/                  # Deployment scripts and container configurations
├── aigit-e2e-test/          # End-to-end integration test suites
└── clone-e2e-test/          # Remote clone verification tests
```

---

## Core System Concepts

### Content-Addressable Storage (CAS)
Every artifact, manifest, tree, and commit in AI-GIT is uniquely addressed by the cryptographic SHA-256 hash of its contents:
* **Blobs & Chunks**: Large model checkpoints and weights are divided into chunks, compressed, and stored under their SHA-256 digest.
* **Trees & Manifests**: Hierarchical file and directory representations linking chunks to paths.
* **Commits**: Immutable snapshots containing a root tree hash, parent commit hash, author metadata, timestamp, and message.

### Local SQLite Metadata Engine
The CLI leverages an embedded SQLite database inside the `.aigit` directory to ensure ACID-compliant tracking of:
* The staging index (staged files and chunk descriptors).
* Local object verification and integrity checks.
* Head references, branch pointers, and remote tracking state.

---

## Getting Started

### 1. Building the C++ CLI

#### Prerequisites
* C++20 compliant compiler (MSVC 2019+, GCC 11+, or Clang 13+)
* CMake 3.20+
* [vcpkg](https://github.com/microsoft/vcpkg)

#### Build Instructions
```bash
cd CLI
vcpkg install
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

### 2. Running the Backend Service

```bash
cd BACKEND
npm install
npm run dev
```

Ensure MinIO is running and configure `.env` with your MinIO endpoint, access key, and secret key.

### 3. Launching the Web Portal

```bash
cd FRONTEND
npm install
node server.js
```

Open `http://localhost:3000` in your browser to explore repositories, visual commit DAGs, and model cards.

---

## CLI Command Reference

| Command | Description |
| :--- | :--- |
| `ai-git init` | Initialize a new local AI-GIT repository |
| `ai-git add <path>` | Stage files or models for chunking and hashing |
| `ai-git commit -m "<msg>"` | Create an immutable commit snapshot |
| `ai-git status` | Display working directory and staging index status |
| `ai-git log` | Show commit history DAG |
| `ai-git branch <name>` | Create, list, or switch branches |
| `ai-git checkout <ref>` | Switch branches or restore working tree files |
| `ai-git diff` | Inspect differences between working tree and staging index |
| `ai-git push` | Sync local commits and missing CAS chunks to remote MinIO storage |
| `ai-git pull` | Fetch remote commits and reconstruct models from remote CAS chunks |
| `ai-git clone <url>` | Clone a remote repository and reconstruct files |
