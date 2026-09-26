require("dotenv").config();

const express = require("express");
const session = require("express-session");
const cors = require("cors");
const fs = require("fs");
const path = require("path");
const crypto = require("crypto");

const app = express();
const PORT = process.env.PORT || 3001;
const BACKEND_URL = process.env.BACKEND_URL || 'http://localhost:3000';

const USERS_FILE = path.join(__dirname, "data", "users.json");
const REPOS_FILE = path.join(__dirname, "data", "models.json");
const ISSUES_FILE = path.join(__dirname, "data", "issues.json");
const DISCUSSIONS_FILE = path.join(__dirname, "data", "discussions.json");

/* =========================================================
   MIDDLEWARE & STATIC FILES
========================================================= */
app.use(cors());
app.use(express.json({ limit: "10mb" }));
app.use(express.urlencoded({ extended: true }));

app.use(
    session({
        secret: process.env.SESSION_SECRET || "ai-git-vcs-secret-key-2026",
        resave: false,
        saveUninitialized: false,
        cookie: {
            httpOnly: true,
            sameSite: "lax",
            secure: false,
            maxAge: 7 * 24 * 60 * 60 * 1000
        }
    })
);

app.use(express.static(path.join(__dirname, "public")));

/* =========================================================
   DATA PERSISTENCE HELPERS
========================================================= */
function ensureDataFiles() {
    const dataDir = path.join(__dirname, "data");
    if (!fs.existsSync(dataDir)) {
        fs.mkdirSync(dataDir, { recursive: true });
    }

    if (!fs.existsSync(USERS_FILE)) {
        const defaultUsers = {
            "aarya-ml": {
                name: "Aarya Doshi",
                username: "aarya-ml",
                email: "aaryadoshi7@gmail.com",
                role: "Lead AI Researcher & Systems Architect",
                bio: "Training multi-billion parameter diffusion models & building decentralized content-addressed VCS for large neural weights.",
                avatar: "https://images.unsplash.com/photo-1534528741775-53994a69daeb?w=150&auto=format&fit=crop&q=80",
                skills: ["PyTorch", "FastCDC", "CUDA", "SafeTensors", "Distributed Training", "JAX", "C++17"],
                organization: "AI-GIT Foundation",
                location: "San Francisco, CA",
                website: "https://github.com/Aarya-2601/AI-GIT",
                passwordHash: hashPassword("demo1234")
            }
        };
        fs.writeFileSync(USERS_FILE, JSON.stringify(defaultUsers, null, 2), "utf8");
    }

    if (!fs.existsSync(REPOS_FILE)) {
        fs.writeFileSync(REPOS_FILE, JSON.stringify(getDefaultModels(), null, 2), "utf8");
    }

    if (!fs.existsSync(ISSUES_FILE)) {
        fs.writeFileSync(ISSUES_FILE, JSON.stringify(getDefaultIssues(), null, 2), "utf8");
    }

    if (!fs.existsSync(DISCUSSIONS_FILE)) {
        fs.writeFileSync(DISCUSSIONS_FILE, JSON.stringify(getDefaultDiscussions(), null, 2), "utf8");
    }
}

function hashPassword(password) {
    const salt = crypto.randomBytes(16).toString("hex");
    const hash = crypto.scryptSync(password, salt, 64).toString("hex");
    return `${salt}:${hash}`;
}

function verifyPassword(password, storedHash) {
    if (!storedHash || !storedHash.includes(":")) return false;
    const [salt, key] = storedHash.split(":");
    const keyBuffer = Buffer.from(key, "hex");
    const derivedKey = crypto.scryptSync(password, salt, 64);
    return crypto.timingSafeEqual(keyBuffer, derivedKey);
}

function readJSON(file, fallback = {}) {
    try {
        if (!fs.existsSync(file)) return fallback;
        return JSON.parse(fs.readFileSync(file, "utf8"));
    } catch (e) {
        console.error(`Error reading ${file}:`, e.message);
        return fallback;
    }
}

function writeJSON(file, data) {
    try {
        fs.writeFileSync(file, JSON.stringify(data, null, 2), "utf8");
        return true;
    } catch (e) {
        console.error(`Error writing ${file}:`, e.message);
        return false;
    }
}

/* =========================================================
   INITIAL DATASETS: AI-ML MODELS, CHUNKS & METRICS
========================================================= */
function getDefaultModels() {
    return [
        {
            id: "llama-3-8b-instruct",
            casRepoName: "aigit-demo",
            name: "llama-3-8b-instruct",
            description: "Meta Llama 3 8B fine-tuned for high-performance instruction following, dialogue, and code synthesis.",
            family: "Large Language Model",
            framework: "SafeTensors",
            precision: "bfloat16",
            parameters: "8.03B",
            contextLength: "8,192 tokens",
            architecture: "LlamaForCausalLM (32 Layers, 4096 Hidden Dim, 32 Heads)",
            epoch: "Epoch 18 / 20",
            valLoss: "1.142",
            stars: 342,
            forks: 89,
            updatedAt: "2 hours ago",
            author: "aarya-ml",
            tags: ["LLM", "SafeTensors", "Instruction-Tuned", "PyTorch", "FastCDC"],
            metrics: {
                rawSizeBytes: 17244160000, // 16.06 GB
                casSizeBytes: 4939212000,  // 4.60 GB
                spaceSavedBytes: 12304948000,
                savingsPercentage: 71.4,
                totalChunks: 3840,
                deduplicatedChunks: 2740,
                compressionRatio: "3.5x"
            },
            tree: [
                {
                    name: "checkpoints",
                    type: "directory",
                    size: "15.8 GB",
                    children: [
                        { name: "epoch_18_best.safetensors", type: "file", size: "4.82 GB", chunks: 1140, deduped: 890, hash: "0742431d714208b339813c68158c171a119133f70e72169ee4432e16d473d18c", epoch: "18", loss: "1.142" },
                        { name: "epoch_15.safetensors", type: "file", size: "4.82 GB", chunks: 1140, deduped: 1020, hash: "109df5378cc6675359146c1a331293be7f238f874867498427a20294b386d21c", epoch: "15", loss: "1.198" },
                        { name: "epoch_10.safetensors", type: "file", size: "4.82 GB", chunks: 1140, deduped: 830, hash: "3df1aee7d468df0b1a3f259baef8a9448ab69201a598a547aeebc98aea0d9927", epoch: "10", loss: "1.284" },
                        { name: "checkpoint_manifest.json", type: "file", size: "128 KB", chunks: 1, deduped: 0, hash: "497467d24740c08a58fb0d1b70a4f9862bdbee330c74f759c1c14c069e2b5b5c" }
                    ]
                },
                {
                    name: "configs",
                    type: "directory",
                    size: "42 KB",
                    children: [
                        { name: "config.json", type: "file", size: "4.2 KB", chunks: 1, deduped: 0, hash: "683a47e3e1630d19b2e069555d8879aa3bf77609d320d7c5d0ad80b12371bfa9" },
                        { name: "generation_config.json", type: "file", size: "1.8 KB", chunks: 1, deduped: 0, hash: "844730bdc7dce7cf9e4331999d4ece051df3d4b5f3f72f4e64766837ea8ab329" },
                        { name: "hyperparameters.yaml", type: "file", size: "36 KB", chunks: 1, deduped: 0, hash: "88402a6933ca84b63f8f6e6cea244a452a2e8c6deaa10b20eedeb1f595dd8d64" }
                    ]
                },
                { name: "tokenizer.json", type: "file", size: "1.84 MB", chunks: 2, deduped: 1, hash: "c7dd71e5dcee2bf5e76314f9efdf9f9cc6599a554797baa3f141ab558fb87c24" },
                { name: "tokenizer_config.json", type: "file", size: "12.4 KB", chunks: 1, deduped: 0, hash: "dcb22345bc735eac3c3cfe2a68fc741361c711dd61d51f57410834310e1aaa9f" },
                { name: "README.md", type: "file", size: "8.6 KB", chunks: 1, deduped: 0, hash: "ea09635eb007798102deb9a9373f8fcb48dc9f126f1fe25c8444491e4bef1a36" }
            ],
            commits: [
                {
                    hash: "c7dd71e5dcee",
                    fullHash: "c7dd71e5dcee2bf5e76314f9efdf9f9cc6599a554797baa3f141ab558fb87c24",
                    branch: "main",
                    epoch: "Epoch 18",
                    valLoss: "1.142",
                    message: "checkpoint: Epoch 18 best model weights; validation perplexity drops to 3.13",
                    author: "Aarya Doshi",
                    date: "2 hours ago",
                    parents: ["844730bdc7dc"],
                    newChunks: 250,
                    reusedChunks: 890
                },
                {
                    hash: "844730bdc7dc",
                    fullHash: "844730bdc7dce7cf9e4331999d4ece051df3d4b5f3f72f4e64766837ea8ab329",
                    branch: "main",
                    epoch: "Epoch 15",
                    valLoss: "1.198",
                    message: "train: Epoch 15 checkpoint saved with CosineAnnealingLR decay",
                    author: "Aarya Doshi",
                    date: "1 day ago",
                    parents: ["683a47e3e163"],
                    newChunks: 120,
                    reusedChunks: 1020
                },
                {
                    hash: "683a47e3e163",
                    fullHash: "683a47e3e1630d19b2e069555d8879aa3bf77609d320d7c5d0ad80b12371bfa9",
                    branch: "lora-finetune",
                    epoch: "Epoch 12",
                    valLoss: "1.240",
                    message: "experiment: Merge LoRA rank-64 adapter into base weights",
                    author: "Elena Rostova",
                    date: "3 days ago",
                    parents: ["3df1aee7d468"],
                    newChunks: 95,
                    reusedChunks: 1045
                },
                {
                    hash: "3df1aee7d468",
                    fullHash: "3df1aee7d468df0b1a3f259baef8a9448ab69201a598a547aeebc98aea0d9927",
                    branch: "main",
                    epoch: "Epoch 10",
                    valLoss: "1.284",
                    message: "checkpoint: Initial baseline fine-tuning weights for Llama-3-8B",
                    author: "Aarya Doshi",
                    date: "5 days ago",
                    parents: ["0742431d7142"],
                    newChunks: 310,
                    reusedChunks: 830
                },
                {
                    hash: "0742431d7142",
                    fullHash: "0742431d714208b339813c68158c171a119133f70e72169ee4432e16d473d18c",
                    branch: "main",
                    epoch: "Init",
                    valLoss: "N/A",
                    message: "init: Import base Llama-3 model architecture, tokenizer & FastCDC manifests",
                    author: "Aarya Doshi",
                    date: "1 week ago",
                    parents: [],
                    newChunks: 1140,
                    reusedChunks: 0
                }
            ]
        },
        {
            id: "stable-diffusion-xl-base",
            name: "stable-diffusion-xl-base",
            description: "High-resolution latent diffusion model for photorealistic text-to-image synthesis with dual text encoders.",
            family: "Computer Vision / Diffusion",
            framework: "SafeTensors",
            precision: "fp16",
            parameters: "3.5B",
            contextLength: "1024x1024 px",
            architecture: "SDXL-UNet (3.1B) + OpenCLIP-ViT/bigG + CLIP-ViT/L (812M)",
            epoch: "Epoch 45 / 50",
            valLoss: "0.082",
            stars: 289,
            forks: 64,
            updatedAt: "4 hours ago",
            author: "aarya-ml",
            tags: ["Diffusion", "Text-to-Image", "SafeTensors", "PyTorch", "Vision"],
            metrics: {
                rawSizeBytes: 14925000000,
                casSizeBytes: 3950000000,
                spaceSavedBytes: 10975000000,
                savingsPercentage: 73.5,
                totalChunks: 3200,
                deduplicatedChunks: 2350,
                compressionRatio: "3.8x"
            },
            tree: [
                {
                    name: "unet",
                    type: "directory",
                    size: "5.14 GB",
                    children: [
                        { name: "diffusion_pytorch_model.safetensors", type: "file", size: "5.14 GB", chunks: 1250, deduped: 950, hash: "a1f5d6e3e3b0c442a1f5d6e3e3b0c442a1f5d6e3e3b0c442a1f5d6e3e3b0c442" },
                        { name: "config.json", type: "file", size: "3.8 KB", chunks: 1, deduped: 0, hash: "b2f6e7d4f4c1d553b2f6e7d4f4c1d553b2f6e7d4f4c1d553b2f6e7d4f4c1d553" }
                    ]
                },
                {
                    name: "vae",
                    type: "directory",
                    size: "335 MB",
                    children: [
                        { name: "diffusion_pytorch_model.safetensors", type: "file", size: "335 MB", chunks: 82, deduped: 82, hash: "c3e8a9d1b0c4e7f2c3e8a9d1b0c4e7f2c3e8a9d1b0c4e7f2c3e8a9d1b0c4e7f2" }
                    ]
                },
                {
                    name: "text_encoder_2",
                    type: "directory",
                    size: "1.39 GB",
                    children: [
                        { name: "model.safetensors", type: "file", size: "1.39 GB", chunks: 340, deduped: 290, hash: "d4f9b0c2e1d5a8e3d4f9b0c2e1d5a8e3d4f9b0c2e1d5a8e3d4f9b0c2e1d5a8e3" }
                    ]
                },
                { name: "model_index.json", type: "file", size: "1.2 KB", chunks: 1, deduped: 0, hash: "e5a0c1d3f2e6b9f4e5a0c1d3f2e6b9f4e5a0c1d3f2e6b9f4e5a0c1d3f2e6b9f4" }
            ],
            commits: [
                {
                    hash: "9b0c2e1d5a8e",
                    fullHash: "9b0c2e1d5a8e3d4f9b0c2e1d5a8e3d4f9b0c2e1d5a8e3d4f9b0c2e1d5a8e3d4f",
                    branch: "main",
                    epoch: "Epoch 45",
                    valLoss: "0.082",
                    message: "checkpoint: Fine-tuned UNet for photorealistic lighting & eye clarity",
                    author: "Aarya Doshi",
                    date: "4 hours ago",
                    parents: ["e3b0c442a1f5"],
                    newChunks: 300,
                    reusedChunks: 950
                },
                {
                    hash: "e3b0c442a1f5",
                    fullHash: "e3b0c442a1f5d6e3e3b0c442a1f5d6e3e3b0c442a1f5d6e3e3b0c442a1f5d6e3",
                    branch: "main",
                    epoch: "Epoch 30",
                    valLoss: "0.098",
                    message: "train: Checkpoint after 30 epochs on Laion-HD subset",
                    author: "Aarya Doshi",
                    date: "2 days ago",
                    parents: [],
                    newChunks: 1250,
                    reusedChunks: 0
                }
            ]
        },
        {
            id: "whisper-large-v3",
            name: "whisper-large-v3",
            description: "Multilingual automatic speech recognition and speech translation model robust against background noise.",
            family: "Audio / Speech",
            framework: "PyTorch",
            precision: "fp16",
            parameters: "1.55B",
            contextLength: "30s chunks",
            architecture: "Encoder-Decoder Audio Transformer (32 Layers, 1280 Hidden Dim)",
            epoch: "Epoch 12 / 12",
            valLoss: "0.194",
            stars: 198,
            forks: 41,
            updatedAt: "1 day ago",
            author: "aarya-ml",
            tags: ["Speech", "ASR", "Multilingual", "PyTorch", "Audio"],
            metrics: {
                rawSizeBytes: 6625000000,
                casSizeBytes: 2050000000,
                spaceSavedBytes: 4575000000,
                savingsPercentage: 69.1,
                totalChunks: 1480,
                deduplicatedChunks: 1020,
                compressionRatio: "3.2x"
            },
            tree: [
                { name: "model.safetensors", type: "file", size: "3.08 GB", chunks: 760, deduped: 540, hash: "f6b1d2e4a3c5b8e7f6b1d2e4a3c5b8e7f6b1d2e4a3c5b8e7f6b1d2e4a3c5b8e7" },
                { name: "preprocessor_config.json", type: "file", size: "336 B", chunks: 1, deduped: 0, hash: "a7c2e3f5b4d6c9f8a7c2e3f5b4d6c9f8a7c2e3f5b4d6c9f8a7c2e3f5b4d6c9f8" },
                { name: "tokenizer.json", type: "file", size: "2.41 MB", chunks: 3, deduped: 2, hash: "b8d3f4a6c5e7d0a9b8d3f4a6c5e7d0a9b8d3f4a6c5e7d0a9b8d3f4a6c5e7d0a9" }
            ],
            commits: [
                {
                    hash: "c5e7d0a9b8d3",
                    fullHash: "c5e7d0a9b8d3f4a6c5e7d0a9b8d3f4a6c5e7d0a9b8d3f4a6c5e7d0a9b8d3f4a6",
                    branch: "main",
                    epoch: "Epoch 12",
                    valLoss: "0.194",
                    message: "release: Final multilingual whisper weights, WER 7.2% on CommonVoice",
                    author: "Aarya Doshi",
                    date: "1 day ago",
                    parents: [],
                    newChunks: 220,
                    reusedChunks: 540
                }
            ]
        },
        {
            id: "deepseek-coder-6.7b",
            name: "deepseek-coder-6.7b",
            description: "State-of-the-art open code LLM trained from scratch on 2 trillion tokens of 87 programming languages.",
            family: "Code Intelligence",
            framework: "GGUF",
            precision: "int4 / Q4_K_M",
            parameters: "6.7B",
            contextLength: "16,384 tokens",
            architecture: "Decoder-only Transformer with RoPE embeddings & SwiGLU activations",
            epoch: "Epoch 8 / 10",
            valLoss: "0.942",
            stars: 412,
            forks: 110,
            updatedAt: "2 days ago",
            author: "aarya-ml",
            tags: ["Code", "GGUF", "Quantized", "LLM", "C++", "Python"],
            metrics: {
                rawSizeBytes: 14350000000,
                casSizeBytes: 4210000000,
                spaceSavedBytes: 10140000000,
                savingsPercentage: 70.7,
                totalChunks: 3120,
                deduplicatedChunks: 2210,
                compressionRatio: "3.4x"
            },
            tree: [
                { name: "deepseek-coder-6.7b.Q4_K_M.gguf", type: "file", size: "4.08 GB", chunks: 980, deduped: 710, hash: "d0a9b8d3f4a6c5e7d0a9b8d3f4a6c5e7d0a9b8d3f4a6c5e7d0a9b8d3f4a6c5e7" },
                { name: "tokenizer.model", type: "file", size: "1.37 MB", chunks: 2, deduped: 1, hash: "e1b0c9a8d7f6e5d4e1b0c9a8d7f6e5d4e1b0c9a8d7f6e5d4e1b0c9a8d7f6e5d4" }
            ],
            commits: [
                {
                    hash: "d7f6e5d4e1b0",
                    fullHash: "d7f6e5d4e1b0c9a8d7f6e5d4e1b0c9a8d7f6e5d4e1b0c9a8d7f6e5d4e1b0c9a8",
                    branch: "main",
                    epoch: "Epoch 8",
                    valLoss: "0.942",
                    message: "quant: Export FastCDC chunked GGUF format for llama.cpp edge inference",
                    author: "Marcus Chen",
                    date: "2 days ago",
                    parents: [],
                    newChunks: 270,
                    reusedChunks: 710
                }
            ]
        },
        {
            id: "aigit-e2e-test",
            name: "aigit-e2e-test",
            description: "Internal end-to-end integration test repository tracking synthetic 10 MB neural network weights and FastCDC chunk parity.",
            family: "Test / Benchmark",
            framework: "Raw Binary",
            precision: "int8",
            parameters: "10M",
            contextLength: "Flat Binary",
            architecture: "Synthetic Matrix Weights (.bin)",
            epoch: "Epoch 1 / 1",
            valLoss: "0.000",
            stars: 12,
            forks: 3,
            updatedAt: "2 days ago",
            author: "aarya-ml",
            tags: ["Test", "E2E", "FastCDC", "Benchmark", "MinIO"],
            metrics: {
                rawSizeBytes: 10318894,
                casSizeBytes: 10318894,
                spaceSavedBytes: 0,
                savingsPercentage: 0.0,
                totalChunks: 10,
                deduplicatedChunks: 0,
                compressionRatio: "1.0x"
            },
            tree: [
                { name: "model.bin", type: "file", size: "10.3 MB", chunks: 10, deduped: 0, hash: "0742431d714208b339813c68158c171a119133f70e72169ee4432e16d473d18c" }
            ],
            commits: [
                {
                    hash: "c7dd71e5dcee",
                    fullHash: "c7dd71e5dcee2bf5e76314f9efdf9f9cc6599a554797baa3f141ab558fb87c24",
                    branch: "main",
                    epoch: "Init",
                    valLoss: "0.000",
                    message: "test: Initial E2E push of 10MB synthetic neural weights to MinIO",
                    author: "Aarya Doshi",
                    date: "2 days ago",
                    parents: [],
                    newChunks: 10,
                    reusedChunks: 0
                }
            ]
        }
    ];
}

function getDefaultIssues() {
    return [
        {
            id: "ISSUE-101",
            repoId: "llama-3-8b-instruct",
            title: "FP16 precision overflow in Attention Projection weights on Epoch 18",
            author: "marcus_ai",
            status: "open",
            createdAt: "3 hours ago",
            labels: ["precision", "fp16", "loss-divergence"],
            commentsCount: 4,
            description: "During evaluation of epoch 18 weights on CUDA compute capability 8.9, attention logits periodically produce NaN when prompt exceeds 4k tokens. Investigating bfloat16 clamping.",
            comments: [
                { author: "aarya-ml", time: "2 hours ago", text: "We verified this is caused by lack of scaling factor in RoPE frequency base. FastCDC chunk #412 isolates the attention weights; updating." }
            ]
        },
        {
            id: "ISSUE-102",
            repoId: "stable-diffusion-xl-base",
            title: "UNet chunk manifest hash mismatch on Windows checkout",
            author: "elena_vision",
            status: "open",
            createdAt: "1 day ago",
            labels: ["windows", "fastcdc", "manifest"],
            commentsCount: 2,
            description: "Running `ai-git checkout main` inside PowerShell triggers a checksum retry on `diffusion_pytorch_model.safetensors` due to CRLF stream reading in index parser.",
            comments: [
                { author: "aarya-ml", time: "18 hours ago", text: "Fixed in binary stream mode. The content-addressed hash now matches bit-for-bit." }
            ]
        },
        {
            id: "ISSUE-103",
            repoId: "whisper-large-v3",
            title: "Support INT8 quantization in GGUF export script",
            author: "david_speech",
            status: "closed",
            createdAt: "4 days ago",
            labels: ["quantization", "gguf", "enhancement"],
            commentsCount: 5,
            description: "Requesting an automated export pipeline to quantize the encoder into Q8_0 while keeping the autoregressive decoder in FP16.",
            comments: [
                { author: "aarya-ml", time: "3 days ago", text: "Merged and pushed in commit `c5e7d0a9b8d3`. Reduces model size by 48% with zero WER degradation." }
            ]
        }
    ];
}

function getDefaultDiscussions() {
    return [
        {
            id: "DISC-201",
            category: "Model Architecture",
            title: "How FastCDC Deduplication Cuts Storage Costs by 75% for Iterative Checkpoints",
            author: "aarya-ml",
            upvotes: 48,
            repliesCount: 12,
            createdAt: "Yesterday",
            tags: ["FastCDC", "Storage-Optimization", "CAS", "Cost-Savings"],
            content: "Traditional Git LFS stores duplicate 15 GB checkpoints for every epoch. With AI-GIT's content-defined chunking (Gear Matrix + dynamic boundaries between 256KB and 4MB), only weights whose gradient updates shifted beyond threshold generate new chunk IDs. Here are our exact benchmarks from Llama-3-8B fine-tuning.",
            replies: [
                { author: "sophia_ml", time: "18 hours ago", text: "This is a game changer for our cluster. We were burning 500 GB every training run on S3." },
                { author: "marcus_ai", time: "12 hours ago", text: "Are you planning to support differential quantization masks on top of FastCDC?" }
            ]
        },
        {
            id: "DISC-202",
            category: "Fine-Tuning & LoRA",
            title: "Optimal Learning Rate & Weight Decay for Llama-3-8B Instruct Fine-Tuning",
            author: "marcus_ai",
            upvotes: 31,
            repliesCount: 7,
            createdAt: "3 days ago",
            tags: ["Llama3", "Hyperparameters", "CosineAnnealing"],
            content: "Sharing our hyperparameter ablation study: 2e-5 initial LR with Cosine decay down to 2e-6, warmup ratio 0.05, batch size 64 with gradient accumulation gives minimal loss divergence.",
            replies: [
                { author: "aarya-ml", time: "2 days ago", text: "Agreed. That matches our findings in checkpoint `epoch_18_best.safetensors`." }
            ]
        },
        {
            id: "DISC-203",
            category: "Quantization & Edge Inference",
            title: "SafeTensors vs GGUF: Choosing the right format for production serving",
            author: "elena_vision",
            upvotes: 27,
            repliesCount: 9,
            createdAt: "5 days ago",
            tags: ["SafeTensors", "GGUF", "Deployment", "vLLM"],
            content: "We benchmarked memory-mapped zero-copy deserialization in SafeTensors against GGUF for edge inference on Apple Silicon and NVIDIA RTX 4090s. Here is what we found...",
            replies: [
                { author: "david_speech", time: "4 days ago", text: "Great breakdown! SafeTensors is ideal for server-side vLLM, while GGUF shines on llama.cpp edge devices." }
            ]
        }
    ];
}

/* =========================================================
   AUTHENTICATION & SESSION REST ENDPOINTS
========================================================= */
const otps = new Map();

app.post("/api/auth/send-otp", async (req, res) => {
    const email = (req.body.email || "").trim().toLowerCase();
    if (!email || !/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email)) {
        return res.status(400).json({ error: "A valid email address is required." });
    }
    const code = Math.floor(100000 + Math.random() * 900000).toString();
    otps.set(email, { code, expires: Date.now() + 5 * 60 * 1000, tries: 0 });

    console.log(`[AI-GIT AUTH] Generated verification code for ${email}: [ ${code} ]`);

    let emailSent = false;
    if (process.env.GMAIL_USER && process.env.GMAIL_APP_PASSWORD) {
        try {
            const nodemailer = require("nodemailer");
            const transporter = nodemailer.createTransport({
                service: "gmail",
                auth: { user: process.env.GMAIL_USER, pass: process.env.GMAIL_APP_PASSWORD.replace(/\s+/g, "") }
            });
            await transporter.sendMail({
                from: `"AI-GIT Portal" <${process.env.GMAIL_USER}>`,
                to: email,
                subject: `Your AI-GIT Verification Code: ${code}`,
                text: `Your AI-GIT verification code is ${code}. It expires in 5 minutes.`
            });
            emailSent = true;
        } catch (e) {
            console.warn("[AI-GIT AUTH] SMTP send failed; falling back to direct response:", e.message);
        }
    }

    return res.json({
        ok: true,
        message: "Verification code generated successfully.",
        otp: (process.env.SHOW_OTP_IN_UI !== "false" || !emailSent) ? code : undefined
    });
});

app.post("/api/auth/login", (req, res) => {
    const { username, password, email, otp, mode } = req.body;
    const users = readJSON(USERS_FILE, {});

    if (mode === "otp") {
        const cleanEmail = (email || "").trim().toLowerCase();
        if (!cleanEmail || !otp) {
            return res.status(400).json({ error: "Email and OTP code are required." });
        }
        const record = otps.get(cleanEmail);
        if (!record) return res.status(400).json({ error: "Please request an OTP first." });
        if (Date.now() > record.expires) return res.status(400).json({ error: "OTP expired. Request a new one." });
        if (record.code !== otp.trim()) return res.status(400).json({ error: "Invalid OTP code." });

        otps.delete(cleanEmail);
        let user = Object.values(users).find(u => (u.email || "").toLowerCase() === cleanEmail);
        if (!user) {
            const tempUser = cleanEmail.split("@")[0].replace(/[^a-zA-Z0-9-]/g, "") || "ai-user";
            user = {
                name: tempUser,
                username: tempUser,
                email: cleanEmail,
                role: "AI Developer",
                bio: "Exploring AI-GIT content-addressed version control for models.",
                skills: ["PyTorch", "SafeTensors"]
            };
            users[tempUser] = user;
            writeJSON(USERS_FILE, users);
        }
        req.session.user = user;
        return res.json({ ok: true, user });
    }

    // Username + Password mode
    const cleanUser = (username || "").trim();
    if (!cleanUser || !password) {
        return res.status(400).json({ error: "Username and password are required." });
    }

    const user = users[cleanUser] || Object.values(users).find(u => u.email?.toLowerCase() === cleanUser.toLowerCase());
    if (!user) {
        return res.status(401).json({ error: "Invalid username or password." });
    }

    if (user.passwordHash && !verifyPassword(password, user.passwordHash)) {
        return res.status(401).json({ error: "Invalid username or password." });
    }

    req.session.user = user;
    return res.json({ ok: true, user });
});

app.post("/api/auth/demo-login", (req, res) => {
    const users = readJSON(USERS_FILE, {});
    const demoUser = users["aarya-ml"] || {
        name: "Aarya Doshi",
        username: "aarya-ml",
        email: "aaryadoshi7@gmail.com",
        role: "Lead AI Researcher & Systems Architect",
        bio: "Training multi-billion parameter diffusion models & building decentralized content-addressed VCS for large neural weights.",
        skills: ["PyTorch", "FastCDC", "CUDA", "SafeTensors", "Distributed Training"],
        organization: "AI-GIT Foundation"
    };
    req.session.user = demoUser;
    return res.json({ ok: true, user: demoUser });
});

app.post("/api/auth/signup", (req, res) => {
    const { name, username, email, password, otp } = req.body;
    if (!name || !username || !email || !password) {
        return res.status(400).json({ error: "All fields are required." });
    }

    const cleanUser = username.trim().toLowerCase();
    const cleanEmail = email.trim().toLowerCase();
    const users = readJSON(USERS_FILE, {});

    if (users[cleanUser]) {
        return res.status(409).json({ error: "Username already taken." });
    }

    if (otp) {
        const record = otps.get(cleanEmail);
        if (!record || record.code !== otp.trim()) {
            return res.status(400).json({ error: "Invalid or expired OTP." });
        }
        otps.delete(cleanEmail);
    }

    const newUser = {
        name: name.trim(),
        username: cleanUser,
        email: cleanEmail,
        role: "AI Engineer",
        bio: "Exploring AI-GIT decentralized model storage.",
        skills: ["Python", "PyTorch"],
        passwordHash: hashPassword(password),
        createdAt: new Date().toISOString()
    };

    users[cleanUser] = newUser;
    writeJSON(USERS_FILE, users);
    req.session.user = newUser;
    return res.json({ ok: true, user: newUser });
});

app.get("/api/auth/me", (req, res) => {
    if (req.session.user) {
        return res.json({ authenticated: true, user: req.session.user });
    }
    return res.json({
        authenticated: false,
        user: {
            name: "Guest Researcher",
            username: "guest-researcher",
            email: "guest@ai-git.org",
            role: "Visiting AI Engineer",
            bio: "Exploring AI-GIT model repositories and FastCDC deduplication metrics."
        }
    });
});

app.post("/api/auth/logout", (req, res) => {
    req.session.destroy(() => {
        res.json({ ok: true });
    });
});

/* =========================================================
   AI-GIT REPOSITORIES & MODEL APIS
========================================================= */
app.get("/api/models", (req, res) => {
    const models = readJSON(REPOS_FILE, getDefaultModels());
    const query = (req.query.q || "").toLowerCase().trim();
    const framework = (req.query.framework || "all").toLowerCase();

    let filtered = models;
    if (query) {
        filtered = filtered.filter(m =>
            m.name.toLowerCase().includes(query) ||
            m.description.toLowerCase().includes(query) ||
            m.family.toLowerCase().includes(query) ||
            m.tags.some(t => t.toLowerCase().includes(query))
        );
    }
    if (framework !== "all") {
        filtered = filtered.filter(m => m.framework.toLowerCase() === framework);
    }
    res.json(filtered);
});

app.get("/api/models/:id", (req, res) => {
    const models = readJSON(REPOS_FILE, getDefaultModels());
    const model = models.find(m => m.id === req.params.id);
    if (!model) {
        return res.status(404).json({ error: "Model repository not found." });
    }
    res.json(model);
});

app.post("/api/models", (req, res) => {
    const { name, description, family, framework, parameters, precision } = req.body;
    if (!name) return res.status(400).json({ error: "Model name is required." });

    const models = readJSON(REPOS_FILE, getDefaultModels());
    const id = name.toLowerCase().replace(/[^a-z0-9-_]/g, "-");
    if (models.some(m => m.id === id)) {
        return res.status(409).json({ error: "Repository with this name already exists." });
    }

    const newModel = {
        id,
        name,
        description: description || "New AI-Git model repository.",
        family: family || "Custom Deep Learning Model",
        framework: framework || "SafeTensors",
        precision: precision || "fp16",
        parameters: parameters || "1.0B",
        architecture: "Transformer CausalLM",
        epoch: "Epoch 1 / 10",
        valLoss: "2.14",
        stars: 1,
        forks: 0,
        updatedAt: "Just now",
        author: req.session.user?.username || "aarya-ml",
        tags: [framework || "SafeTensors", "AI-GIT", "PyTorch"],
        metrics: {
            rawSizeBytes: 1048576000,
            casSizeBytes: 314572800,
            spaceSavedBytes: 734003200,
            savingsPercentage: 70.0,
            totalChunks: 250,
            deduplicatedChunks: 175,
            compressionRatio: "3.3x"
        },
        tree: [
            { name: "config.json", type: "file", size: "1.2 KB", chunks: 1, deduped: 0, hash: crypto.randomBytes(32).toString("hex") },
            { name: "model.safetensors", type: "file", size: "1.02 GB", chunks: 250, deduped: 175, hash: crypto.randomBytes(32).toString("hex") },
            { name: "README.md", type: "file", size: "1.5 KB", chunks: 1, deduped: 0, hash: crypto.randomBytes(32).toString("hex") }
        ],
        commits: [
            {
                hash: crypto.randomBytes(6).toString("hex"),
                fullHash: crypto.randomBytes(32).toString("hex"),
                branch: "main",
                epoch: "Init",
                valLoss: "2.14",
                message: "Initial commit of model architecture & manifest",
                author: req.session.user?.name || "Aarya Doshi",
                date: "Just now",
                parents: [],
                newChunks: 250,
                reusedChunks: 0
            }
        ]
    };

    models.unshift(newModel);
    writeJSON(REPOS_FILE, models);
    res.status(201).json(newModel);
});

/* =========================================================
   ISSUES & DISCUSSIONS REST ENDPOINTS
========================================================= */
app.get("/api/issues", (req, res) => {
    const issues = readJSON(ISSUES_FILE, getDefaultIssues());
    const repoId = req.query.repoId;
    const status = req.query.status;
    let filtered = issues;
    if (repoId) filtered = filtered.filter(i => i.repoId === repoId);
    if (status && status !== "all") filtered = filtered.filter(i => i.status === status);
    res.json(filtered);
});

app.post("/api/issues", (req, res) => {
    const { title, repoId, description, labels } = req.body;
    if (!title || !repoId) return res.status(400).json({ error: "Title and Repository are required." });

    const issues = readJSON(ISSUES_FILE, getDefaultIssues());
    const newIssue = {
        id: `ISSUE-${100 + issues.length + 1}`,
        repoId,
        title,
        description: description || "No description provided.",
        author: req.session.user?.username || "aarya-ml",
        status: "open",
        createdAt: "Just now",
        labels: Array.isArray(labels) ? labels : (labels || "general").split(",").map(l => l.trim()),
        commentsCount: 0,
        comments: []
    };
    issues.unshift(newIssue);
    writeJSON(ISSUES_FILE, issues);
    res.status(201).json(newIssue);
});

app.post("/api/issues/:id/comments", (req, res) => {
    const { text } = req.body;
    if (!text) return res.status(400).json({ error: "Comment text is required." });

    const issues = readJSON(ISSUES_FILE, getDefaultIssues());
    const issue = issues.find(i => i.id === req.params.id);
    if (!issue) return res.status(404).json({ error: "Issue not found." });

    const comment = {
        author: req.session.user?.username || "aarya-ml",
        time: "Just now",
        text
    };
    issue.comments.push(comment);
    issue.commentsCount = issue.comments.length;
    writeJSON(ISSUES_FILE, issues);
    res.json(issue);
});

app.get("/api/discussions", (req, res) => {
    const discussions = readJSON(DISCUSSIONS_FILE, getDefaultDiscussions());
    res.json(discussions);
});

app.post("/api/discussions", (req, res) => {
    const { title, category, content, tags } = req.body;
    if (!title || !content) return res.status(400).json({ error: "Title and content are required." });

    const discussions = readJSON(DISCUSSIONS_FILE, getDefaultDiscussions());
    const newDisc = {
        id: `DISC-${200 + discussions.length + 1}`,
        title,
        category: category || "General AI-Git",
        content,
        author: req.session.user?.username || "aarya-ml",
        upvotes: 1,
        repliesCount: 0,
        createdAt: "Just now",
        tags: Array.isArray(tags) ? tags : (tags || "AI-GIT").split(",").map(t => t.trim()),
        replies: []
    };
    discussions.unshift(newDisc);
    writeJSON(DISCUSSIONS_FILE, discussions);
    res.status(201).json(newDisc);
});

app.post("/api/discussions/:id/reply", (req, res) => {
    const { text } = req.body;
    if (!text) return res.status(400).json({ error: "Reply text is required." });

    const discussions = readJSON(DISCUSSIONS_FILE, getDefaultDiscussions());
    const disc = discussions.find(d => d.id === req.params.id);
    if (!disc) return res.status(404).json({ error: "Discussion thread not found." });

    const reply = {
        author: req.session.user?.username || "aarya-ml",
        time: "Just now",
        text
    };
    disc.replies.push(reply);
    disc.repliesCount = disc.replies.length;
    writeJSON(DISCUSSIONS_FILE, discussions);
    res.json(disc);
});

app.post("/api/discussions/:id/upvote", (req, res) => {
    const discussions = readJSON(DISCUSSIONS_FILE, getDefaultDiscussions());
    const disc = discussions.find(d => d.id === req.params.id);
    if (!disc) return res.status(404).json({ error: "Discussion thread not found." });
    disc.upvotes = (disc.upvotes || 0) + 1;
    writeJSON(DISCUSSIONS_FILE, discussions);
    res.json({ ok: true, upvotes: disc.upvotes });
});

/* =========================================================
   SYSTEM-WIDE STORAGE & DEDUPLICATION METRICS
========================================================= */
app.get("/api/metrics", (req, res) => {
    const models = readJSON(REPOS_FILE, getDefaultModels());
    let totalRawBytes = 0;
    let totalCasBytes = 0;
    let totalChunks = 0;
    let deduplicatedChunks = 0;

    for (const m of models) {
        if (m.metrics) {
            totalRawBytes += m.metrics.rawSizeBytes || 0;
            totalCasBytes += m.metrics.casSizeBytes || 0;
            totalChunks += m.metrics.totalChunks || 0;
            deduplicatedChunks += m.metrics.deduplicatedChunks || 0;
        }
    }

    const spaceSavedBytes = totalRawBytes - totalCasBytes;
    const savingsPercent = totalRawBytes > 0 ? ((spaceSavedBytes / totalRawBytes) * 100).toFixed(1) : 0;
    const deduplicationRate = totalChunks > 0 ? ((deduplicatedChunks / totalChunks) * 100).toFixed(1) : 0;

    res.json({
        totalRawGB: (totalRawBytes / (1024 ** 3)).toFixed(1),
        totalCasGB: (totalCasBytes / (1024 ** 3)).toFixed(1),
        spaceSavedGB: (spaceSavedBytes / (1024 ** 3)).toFixed(1),
        savingsPercent,
        totalChunks,
        deduplicatedChunks,
        deduplicationRate,
        totalModels: models.length,
        avgCompressionRatio: "3.4x",
        fastCdcParameters: {
            minChunk: "256 KB",
            avgChunk: "1.0 MB",
            maxChunk: "4.0 MB",
            hashAlgorithm: "SHA-256 (OpenSSL EVP)",
            boundaryAlgorithm: "Gear Matrix Rolling Hash"
        }
    });
});

/* =========================================================
   PROFILE STUDIO: AUTOMATION & README GENERATION
========================================================= */
app.get("/api/profile", (req, res) => {
    const users = readJSON(USERS_FILE, {});
    const username = req.session.user?.username || "aarya-ml";
    const user = users[username] || req.session.user || {};
    res.json({ profile: user });
});

app.post("/api/profile", (req, res) => {
    const users = readJSON(USERS_FILE, {});
    const username = req.session.user?.username || "aarya-ml";
    users[username] = {
        ...(users[username] || {}),
        ...req.body
    };
    writeJSON(USERS_FILE, users);
    req.session.user = users[username];
    res.json({ ok: true, profile: users[username] });
});

app.post("/api/profile/generate-readme", (req, res) => {
    const profile = req.body;
    const name = profile.name || "AI Researcher";
    const bio = profile.bio || "Machine Learning Engineer";
    const role = profile.role || "Systems Architect";
    const skills = Array.isArray(profile.skills) ? profile.skills : (profile.skills || "PyTorch,FastCDC,C++").split(",");

    const skillBadges = skills.map(s => `![${s.trim()}](https://img.shields.io/badge/${encodeURIComponent(s.trim())}-blue?style=for-the-badge&logo=pytorch&logoColor=white)`).join(" ");

    const markdown = `# Hi there, I'm ${name} 👋 🚀

> **${role}** | ${bio}

---

### 🧠 Core Deep Learning & Systems Stack
${skillBadges}

---

### ⚡ AI-GIT Version Control Highlights
- 📦 **Tracked Models**: Llama-3-8B-Instruct, SDXL-Base, Whisper-Large-V3
- 💾 **Storage Deduplication Efficiency**: **74.6% Space Saved** via FastCDC
- 🔄 **Incremental Checkpoint Deltas**: Only modified weight slices synced to remote CAS
- 🛠️ **Favorite Tools**: SafeTensors, PyTorch, C++17, MinIO, CUDA

---

### 📊 AI-GIT Activity & Checkpoints
\`\`\`text
[AI-GIT] model: llama-3-8b-instruct @ epoch_18_best (val_loss: 1.142) -> 890 chunks deduplicated
[AI-GIT] model: stable-diffusion-xl-base @ epoch_45 (val_loss: 0.082) -> 950 chunks deduplicated
[AI-GIT] model: whisper-large-v3 @ epoch_12 (WER: 7.2%) -> 540 chunks deduplicated
\`\`\`

---
*Generated automatically by [AI-GIT Profile Studio](https://github.com/Aarya-2601/AI-GIT)* ⚡
`;

    res.json({ markdown });
});

/* =========================================================
   BOOTSTRAP SERVER
========================================================= */
ensureDataFiles();

/* =========================================================
   LIVE AI/ML NEWS & COMMUNITY FEEDS API
========================================================= */
app.get("/api/news/ai", async (req, res) => {
    try {
        const https = require("https");
        const fetchDevTo = new Promise((resolve) => {
            const reqDev = https.get("https://dev.to/api/articles?tag=ai&per_page=8", { headers: { "User-Agent": "AI-GIT-App" } }, (resDev) => {
                let data = "";
                resDev.on("data", chunk => data += chunk);
                resDev.on("end", () => {
                    try {
                        const parsed = JSON.parse(data);
                        if (Array.isArray(parsed) && parsed.length > 0) {
                            return resolve(parsed.map(item => ({
                                id: "devto_" + item.id,
                                title: item.title,
                                description: item.description || "Latest AI breakthrough and engineering discussion from the developer community.",
                                url: item.url,
                                cover_image: item.cover_image || item.social_image || "https://images.unsplash.com/photo-1620712943543-bcc4688e7485?w=600&auto=format&fit=crop&q=80",
                                source: "Dev.to AI",
                                author: item.user ? item.user.name : "AI Researcher",
                                author_avatar: item.user ? item.user.profile_image_90 : "https://images.unsplash.com/photo-1534528741775-53994a69daeb?w=80&auto=format&fit=crop&q=80",
                                published_at: item.published_at || new Date().toISOString(),
                                tags: item.tag_list || ["ai", "machinelearning"],
                                reading_time: item.reading_time_minutes ? `${item.reading_time_minutes} min read` : "4 min read",
                                reactions_count: item.positive_reactions_count || Math.floor(Math.random() * 45) + 15
                            })));
                        }
                        resolve([]);
                    } catch (e) {
                        resolve([]);
                    }
                });
            });
            reqDev.on("error", () => resolve([]));
            reqDev.setTimeout(3500, () => { reqDev.destroy(); resolve([]); });
        });

        const devToArticles = await fetchDevTo;

        if (devToArticles && devToArticles.length > 0) {
            return res.json({ articles: devToArticles, source: "live_api" });
        }

        // Guaranteed fallback articles
        const fallbackArticles = [
            {
                id: "fb_1",
                title: "DeepSeek-V3 Architecture: Multi-head Latent Attention and Dual-Pipe Parallelism",
                description: "Exploring the memory-efficient MoE routing algorithms and FP8 mixed-precision training kernels used in DeepSeek-V3.",
                url: "https://arxiv.org/abs/2412.19437",
                cover_image: "https://images.unsplash.com/photo-1620712943543-bcc4688e7485?w=600&auto=format&fit=crop&q=80",
                source: "ArXiv AI",
                author: "DeepSeek AI Research",
                published_at: new Date(Date.now() - 3600000 * 2).toISOString(),
                tags: ["llm", "moe", "deeplearning"],
                reading_time: "7 min read",
                reactions_count: 142
            },
            {
                id: "fb_2",
                title: "Fast Content-Defined Chunking: Scaling Model Version Control to 70B+ Checkpoints",
                description: "How FastCDC rolling hashes eliminate 74% of duplicate attention tensors across fine-tuning epochs.",
                url: "https://github.com/Aarya-2601/AI-GIT",
                cover_image: "https://images.unsplash.com/photo-1618005182384-a83a8bd57fbe?w=600&auto=format&fit=crop&q=80",
                source: "AI-GIT Labs",
                author: "Aarya Doshi",
                published_at: new Date(Date.now() - 3600000 * 5).toISOString(),
                tags: ["fastcdc", "vcs", "safetensors"],
                reading_time: "5 min read",
                reactions_count: 98
            },
            {
                id: "fb_3",
                title: "Llama 3.3 70B: High-Efficiency Open Foundation Model Performance Benchmarks",
                description: "Meta releases updated weights with refined instruction datasets, matching 405B capabilities on mathematical reasoning.",
                url: "https://ai.meta.com/blog/",
                cover_image: "https://images.unsplash.com/photo-1677442136019-21780efad99a?w=600&auto=format&fit=crop&q=80",
                source: "Meta AI",
                author: "Llama Team",
                published_at: new Date(Date.now() - 3600000 * 9).toISOString(),
                tags: ["llama3", "opensource", "benchmarks"],
                reading_time: "6 min read",
                reactions_count: 215
            }
        ];

        res.json({ articles: fallbackArticles, source: "curated_fallback" });
    } catch (err) {
        res.status(500).json({ error: "Failed to fetch AI news", details: err.message });
    }
});

app.get('/api/backend/health', async (req, res) => {
  try {
    const response = await fetch(`${BACKEND_URL}/health`);
    const data = await response.json();
    res.status(response.status).json(data);
  } catch (err) {
    res.status(503).json({
      status: "error",
      message: "AI-Git CAS backend is unreachable",
      detail: err.message
    });
  }
});

app.get('/api/backend/repos/:repoName', async (req, res) => {
  try {
    const response = await fetch(`${BACKEND_URL}/api/v1/repos/${encodeURIComponent(req.params.repoName)}`);
    const data = await response.json();
    res.status(response.status).json(data);
  } catch (err) {
    res.status(503).json({
      status: "error",
      message: "AI-Git CAS backend is unreachable",
      detail: err.message
    });
  }
});

app.listen(PORT, () => {
    console.log(`=======================================================`);
    console.log(`⚡ AI-GIT Frontend Portal Live on: http://localhost:${PORT}`);
    console.log(`📦 Model Hub, FastCDC Metrics, Visual DAG, Profile Studio`);
    console.log(`=======================================================`);
});