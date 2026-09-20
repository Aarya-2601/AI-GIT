/* ==========================================================================
   Dashboard — repo list / repo detail
   Uses mock data shaped like realistic BACKEND responses. Real endpoints
   (see js/api.js TODOs) are /api/v1/push/negotiate, /api/v1/pull/urls,
   and /clone/:repoName — none of them return repo/commit listings today,
   so this mock data stands in until a GET /api/v1/repos-style endpoint
   exists on the BACKEND.
   ========================================================================== */
(function () {
  "use strict";

  var MOCK_REPOS = [
    {
      name: "llama-7b-finetune",
      sizeBytes: 13_400_000_000,
      branchCount: 4,
      lastCommit: {
        hash: "9f2a1c7e4b8d",
        message: "Add LoRA adapter weights for instruction tuning",
        author: "sagar.mitra",
        timestamp: "2026-09-17T14:22:00Z",
      },
      branches: [
        { name: "main", head: "9f2a1c7e4b8d" },
        { name: "quantized-int8", head: "72bbf01a" },
        { name: "eval/rlhf-v2", head: "d41e88c0" },
        { name: "archive/v1-base", head: "0a9c3fe1" },
      ],
      commits: [
        { hash: "9f2a1c7e4b8d", message: "Add LoRA adapter weights for instruction tuning", author: "sagar.mitra", timestamp: "2026-09-17T14:22:00Z" },
        { hash: "3c7d09b21f4a", message: "Rebalance tokenizer merges, drop unused vocab", author: "sagar.mitra", timestamp: "2026-09-15T09:05:00Z" },
        { hash: "e15af90c3d2b", message: "Checkpoint after epoch 3 (loss 1.842)", author: "priya.nair", timestamp: "2026-09-12T21:40:00Z" },
        { hash: "0a9c3fe1b7c4", message: "Initial import of base weights", author: "priya.nair", timestamp: "2026-09-08T11:00:00Z" },
      ],
      tree: [
        { type: "dir", name: "checkpoints", children: [
          { type: "file", name: "model-00001-of-00003.safetensors", size: "4.6 GB", cas: "sha256:8a1f…c02e" },
          { type: "file", name: "model-00002-of-00003.safetensors", size: "4.6 GB", cas: "sha256:2b77…91af" },
          { type: "file", name: "model-00003-of-00003.safetensors", size: "4.2 GB", cas: "sha256:f403…7d19" },
        ]},
        { type: "dir", name: "adapters", children: [
          { type: "file", name: "lora-instruction-v2.safetensors", size: "184 MB", cas: "sha256:aa30…5e6b" },
        ]},
        { type: "file", name: "config.json", size: "1.4 KB", cas: "sha256:1109…c4a2" },
        { type: "file", name: "tokenizer.model", size: "488 KB", cas: "sha256:66e0…33bd" },
      ],
    },
    {
      name: "diffusion-photo-restore",
      sizeBytes: 5_800_000_000,
      branchCount: 2,
      lastCommit: {
        hash: "614dabf9",
        message: "Swap VAE for higher-fidelity variant",
        author: "priya.nair",
        timestamp: "2026-09-16T08:11:00Z",
      },
      branches: [
        { name: "main", head: "614dabf9" },
        { name: "experiment/vae-swap", head: "614dabf9" },
      ],
      commits: [
        { hash: "614dabf9", message: "Swap VAE for higher-fidelity variant", author: "priya.nair", timestamp: "2026-09-16T08:11:00Z" },
        { hash: "b201f77a", message: "Add safety checker weights", author: "sagar.mitra", timestamp: "2026-09-10T17:30:00Z" },
      ],
      tree: [
        { type: "file", name: "unet.safetensors", size: "3.4 GB", cas: "sha256:c9e1…40b7" },
        { type: "file", name: "vae.safetensors", size: "335 MB", cas: "sha256:77aa…0912" },
        { type: "file", name: "safety_checker.bin", size: "1.2 GB", cas: "sha256:5510…fe22" },
      ],
    },
    {
      name: "speech-asr-en-small",
      sizeBytes: 890_000_000,
      branchCount: 3,
      lastCommit: {
        hash: "a02fc118",
        message: "Prune unused decoder heads",
        author: "team-audio",
        timestamp: "2026-09-11T13:47:00Z",
      },
      branches: [
        { name: "main", head: "a02fc118" },
        { name: "quantized", head: "8871cd0a" },
        { name: "wip/streaming", head: "d0129ff3" },
      ],
      commits: [
        { hash: "a02fc118", message: "Prune unused decoder heads", author: "team-audio", timestamp: "2026-09-11T13:47:00Z" },
        { hash: "d0129ff3", message: "Add streaming inference config", author: "team-audio", timestamp: "2026-09-05T10:02:00Z" },
        { hash: "8871cd0a", message: "Initial int8 quantized checkpoint", author: "team-audio", timestamp: "2026-08-29T16:15:00Z" },
      ],
      tree: [
        { type: "file", name: "encoder.onnx", size: "410 MB", cas: "sha256:9931…aa02" },
        { type: "file", name: "decoder.onnx", size: "295 MB", cas: "sha256:2214…bb17" },
        { type: "file", name: "vocab.json", size: "62 KB", cas: "sha256:0044…9c3d" },
      ],
    },
  ];

  function formatBytes(bytes) {
    var units = ["B", "KB", "MB", "GB", "TB"];
    var i = 0;
    var n = bytes;
    while (n >= 1024 && i < units.length - 1) {
      n /= 1024;
      i++;
    }
    return n.toFixed(n < 10 && i > 0 ? 1 : 0) + " " + units[i];
  }

  function formatDate(iso) {
    var d = new Date(iso);
    return d.toLocaleDateString(undefined, { year: "numeric", month: "short", day: "numeric" });
  }

  function timeAgo(iso) {
    var diffMs = Date.now() - new Date(iso).getTime();
    var days = Math.floor(diffMs / (1000 * 60 * 60 * 24));
    if (days <= 0) return "today";
    if (days === 1) return "1 day ago";
    if (days < 30) return days + " days ago";
    return formatDate(iso);
  }

  var iconFile = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M6 2h9l5 5v15H6z"/><path d="M15 2v5h5"/></svg>';
  var iconFolder = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M3 6a1 1 0 0 1 1-1h5l2 2h9a1 1 0 0 1 1 1v10a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1z"/></svg>';
  var iconRepo = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><rect x="3" y="4" width="18" height="16" rx="2"/><path d="M3 9h18M9 4v16"/></svg>';
  var iconBranch = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><circle cx="6" cy="6" r="2.2"/><circle cx="6" cy="18" r="2.2"/><circle cx="18" cy="9" r="2.2"/><path d="M6 8.2V15.8M6 9.5C6 12 8.5 13 12 13c3.5 0 6-2 6-4"/></svg>';

  function renderRepoList(repos) {
    var list = document.getElementById("repo-list");
    if (!list) return;
    list.innerHTML = repos.map(function (repo, idx) {
      return (
        '<div class="repo-row" data-reveal data-reveal-delay="' + (idx * 40) + '" tabindex="0" role="button" data-repo="' + repo.name + '">' +
          '<div class="repo-name">' + iconRepo + '<span>' + repo.name + '</span></div>' +
          '<div class="repo-meta-item">Size<strong>' + formatBytes(repo.sizeBytes) + '</strong></div>' +
          '<div class="repo-meta-item">Last commit<strong class="mono">' + repo.lastCommit.hash.slice(0, 8) + '</strong></div>' +
          '<div class="repo-meta-item">Branches<strong>' + repo.branchCount + '</strong></div>' +
        '</div>'
      );
    }).join("");

    list.querySelectorAll(".repo-row").forEach(function (row) {
      row.addEventListener("click", function () {
        window.location.href = "dashboard.html?repo=" + encodeURIComponent(row.getAttribute("data-repo"));
      });
      row.addEventListener("keydown", function (e) {
        if (e.key === "Enter" || e.key === " ") {
          e.preventDefault();
          row.click();
        }
      });
    });
  }

  function renderTree(nodes) {
    return "<ul class=\"file-tree\">" + nodes.map(function (node) {
      if (node.type === "dir") {
        return "<li><div class=\"tree-node\">" + iconFolder + "<span>" + node.name + "</span></div>" + renderTree(node.children) + "</li>";
      }
      return (
        "<li><div class=\"tree-node\">" + iconFile + "<span>" + node.name + "</span>" +
        "<span class=\"tree-hash\">" + node.size + " · " + node.cas + "</span></div></li>"
      );
    }).join("") + "</ul>";
  }

  function renderRepoDetail(repo) {
    var container = document.getElementById("repo-detail");
    if (!container) return;

    document.getElementById("repo-detail-name").textContent = repo.name;
    document.title = repo.name + " · Dashboard · AI-GIT";

    document.getElementById("stat-size").textContent = formatBytes(repo.sizeBytes);
    document.getElementById("stat-branches").textContent = repo.branchCount;
    document.getElementById("stat-commits").textContent = repo.commits.length;
    document.getElementById("stat-updated").textContent = timeAgo(repo.lastCommit.timestamp);

    document.getElementById("file-tree-container").innerHTML = renderTree(repo.tree);

    document.getElementById("commit-log").innerHTML = repo.commits.map(function (c) {
      return (
        '<div class="commit-item">' +
          '<span class="commit-dot" aria-hidden="true"></span>' +
          '<div>' +
            '<div class="commit-msg">' + c.message + '</div>' +
            '<div class="commit-meta"><span class="commit-hash mono">' + c.hash + '</span><span>' + c.author + '</span><span>' + formatDate(c.timestamp) + '</span></div>' +
          '</div>' +
        '</div>'
      );
    }).join("");

    document.getElementById("branch-list").innerHTML = repo.branches.map(function (b) {
      return (
        '<div class="branch-item">' +
          '<span class="branch-name">' + iconBranch + b.name + '</span>' +
          '<span class="mono text-faint">' + b.head + '</span>' +
        '</div>'
      );
    }).join("");
  }

  function initHealthCheck() {
    var badge = document.querySelector("[data-api-status]");
    if (!badge || !window.AIGIT_API) return;

    function setBadge(state, label) {
      badge.className = "badge badge-" + state;
      badge.innerHTML = '<span class="badge-dot" aria-hidden="true"></span>' + label;
    }

    setBadge("loading", "Checking API…");

    window.AIGIT_API.getHealth().then(function (result) {
      if (result.ok) {
        setBadge("online", "API online (" + result.latencyMs + "ms)");
      } else {
        setBadge("offline", "API offline");
      }
    });
  }

  function init() {
    initHealthCheck();

    var params = new URLSearchParams(window.location.search);
    var repoName = params.get("repo");

    var listView = document.getElementById("repo-list-view");
    var detailView = document.getElementById("repo-detail-view");
    if (!listView || !detailView) return;

    if (repoName) {
      var repo = MOCK_REPOS.find(function (r) { return r.name === repoName; });
      if (repo) {
        listView.hidden = true;
        detailView.hidden = false;
        renderRepoDetail(repo);
        return;
      }
    }

    listView.hidden = false;
    detailView.hidden = true;
    renderRepoList(MOCK_REPOS);
  }

  document.addEventListener("DOMContentLoaded", init);
})();
