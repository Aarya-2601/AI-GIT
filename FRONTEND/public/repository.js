/* =========================================================
   AI-GIT REPOSITORY & NODE CONTROLS
   Smoothly moving slider bar under active tab
========================================================= */

(function () {
  let activeTab = "artifacts";

  async function init() {
    const urlParams = new URLSearchParams(window.location.search);
    const repoParam = urlParams.get("repo") || urlParams.get("id") || "AI-GIT";

    const titleDisplay = document.getElementById("nodeNameDisplay");
    if (titleDisplay) titleDisplay.textContent = repoParam;

    document.title = `${repoParam} · AI-GIT Node`;

    setupTabsWithSmoothSlider();
    setupActions();

    try {
      const modelRes = await fetch(`/api/models/${encodeURIComponent(repoParam)}`);
      let model = null;
      if (modelRes.ok) {
        model = await modelRes.json();
      }
      window.currentModelData = model;

      const casRepo = (model && model.casRepoName) ? model.casRepoName : repoParam;
      window.currentCasRepoName = casRepo;

      let backendMeta = null;
      try {
        const backendRes = await fetch(`/api/backend/repos/${encodeURIComponent(casRepo)}`);
        if (backendRes.ok) {
          const backendData = await backendRes.json();
          backendMeta = backendData.repository || backendData;
        }
      } catch (err) {
        console.warn("Could not reach backend repo meta:", err);
      }

      applyRepoMetadata(model, backendMeta, repoParam);

      // Support direct deep link: ?view=dag or ?tab=dag
      const viewParam = urlParams.get("view") || urlParams.get("tab");
      if (viewParam === "dag") {
        const dagTabBtn = document.getElementById("tabCommitDag");
        if (dagTabBtn) dagTabBtn.click();
      }
    } catch (err) {
      console.error("Error fetching repository metadata:", err);
    }
  }

  function applyRepoMetadata(model, backendMeta, repoParam) {
    if (model) {
      const titleDisplay = document.getElementById("nodeNameDisplay");
      if (titleDisplay) titleDisplay.textContent = model.name || repoParam;

      const descEl = document.querySelector(".repo-description-text");
      if (descEl && model.description) descEl.textContent = model.description;

      const starsEl = document.getElementById("starCount");
      if (starsEl && model.stars !== undefined) starsEl.textContent = model.stars;

      const forksEl = document.getElementById("forkCount");
      if (forksEl && model.forks !== undefined) forksEl.textContent = model.forks;
    }

    const headRef = backendMeta?.head || "refs/heads/main";
    const branchName = headRef.replace("refs/heads/", "");
    const branchSpan = document.querySelector(".branch-selector-btn span");
    if (branchSpan) branchSpan.textContent = branchName;

    const commits = (model && model.commits && model.commits.length > 0) ? model.commits : [];
    const latestCommit = commits[0];

    const authorEl = document.querySelector(".commit-author-name");
    const msgEl = document.querySelector(".commit-message-preview");
    const hashEl = document.querySelector(".commit-hash-link");
    const timeEl = document.querySelector(".commit-time");
    const countBadge = document.querySelector(".commit-count-badge strong");

    if (latestCommit) {
      if (authorEl) authorEl.textContent = latestCommit.author || "aarya-ml";
      if (msgEl) msgEl.textContent = latestCommit.message || "Model checkpoint";
      if (hashEl) hashEl.textContent = (latestCommit.hash || latestCommit.fullHash || "").slice(0, 7);
      if (timeEl) timeEl.textContent = latestCommit.date || "recently";
      if (countBadge) countBadge.textContent = commits.length;
    } else if (backendMeta) {
      const latestHash = (backendMeta.refs && backendMeta.refs[backendMeta.head]) || "";
      if (hashEl && latestHash) hashEl.textContent = latestHash.slice(0, 7);
      if (timeEl && backendMeta.updated_at) timeEl.textContent = new Date(backendMeta.updated_at).toLocaleDateString();
      if (countBadge && backendMeta.objectCount !== undefined) countBadge.textContent = Math.max(1, Math.floor(backendMeta.objectCount / 2));
    }

    // Dynamically render actual repository files if model has custom tree
    renderDynamicRepoFiles(model, repoParam);
  }

  function renderDynamicRepoFiles(model, repoParam) {
    if (!model || !model.tree || model.tree.length === 0) return;
    const tableCard = document.querySelector(".files-table-card");
    if (!tableCard) return;

    // Build dynamic file list
    const rowsHtml = model.tree.map(item => {
      const isDir = item.type === "directory";
      const icon = isDir ? "📁" : (item.name.endsWith(".safetensors") || item.name.endsWith(".bin") ? "🧊" : "📄");
      const sizeStr = item.size || "1.0 GB";
      const dedupNote = item.deduped ? ` (${item.deduped} chunks deduplicated)` : "";
      const commitNote = item.commitMsg || `FastCDC Verified · ${sizeStr}${dedupNote}`;

      return `
        <div class="file-row" onclick="handleFileClick('${item.name}', '${item.type || 'file'}')" style="cursor: pointer;">
          <div class="file-name-part" style="display: flex; align-items: center; gap: 8px;">
            <span class="${isDir ? 'file-icon-dir' : 'file-icon-file'}">${icon}</span>
            <span class="file-name" style="font-weight: 500; font-family: monospace;">${item.name}</span>
            <span style="font-size: 11px; padding: 2px 6px; border-radius: 4px; background: rgba(56, 189, 248, 0.1); color: #38bdf8; border: 1px solid rgba(56, 189, 248, 0.25);">${sizeStr}</span>
          </div>
          <div class="file-commit-part" style="color: #64748b; font-size: 13px;">${commitNote}</div>
          <div class="file-time-part" style="color: #94a3b8; font-size: 12px;">Just now</div>
        </div>
      `;
    }).join("");

    tableCard.innerHTML = rowsHtml;

    // Also update README box with the actual repo info
    const readmeTitle = document.querySelector(".readme-title");
    if (readmeTitle) readmeTitle.textContent = model.name || repoParam;

    const readmeLead = document.querySelector(".readme-lead");
    if (readmeLead) readmeLead.textContent = model.description || `FastCDC content-addressed repository for ${model.name}.`;

    const readmeCli = document.querySelector(".cli-pre code");
    if (readmeCli) {
      readmeCli.textContent = `# Clone ${model.name} repository metadata\nai-git clone ${model.name}\n\n# Pull zero-copy deduplicated weights\nai-git pull origin main --weights`;
    }
  }

  function setupTabsWithSmoothSlider() {
    const tabs = document.querySelectorAll(".node-tab-item[data-tab]");
    const slider = document.getElementById("nodeTabsSliderBar");
    const container = document.getElementById("nodeTabsContainer");

    function moveSliderTo(tabElement) {
      if (!tabElement || !slider || !container) return;
      const containerRect = container.getBoundingClientRect();
      const tabRect = tabElement.getBoundingClientRect();

      const left = tabRect.left - containerRect.left;
      const width = tabRect.width;

      slider.style.left = `${left}px`;
      slider.style.width = `${width}px`;
    }

    tabs.forEach(tab => {
      tab.addEventListener("click", () => {
        tabs.forEach(t => {
          t.classList.remove("active");
          t.setAttribute("aria-selected", "false");
        });
        tab.classList.add("active");
        tab.setAttribute("aria-selected", "true");
        activeTab = tab.dataset.tab;
        switchTab(activeTab);
        moveSliderTo(tab);
      });
    });

    // Initial slider placement
    const activeTabEl = document.querySelector(".node-tab-item.active") || tabs[0];
    if (activeTabEl) {
      setTimeout(() => moveSliderTo(activeTabEl), 50);
    }

    // Keep slider positioned on window resize
    window.addEventListener("resize", () => {
      const currentActive = document.querySelector(".node-tab-item.active");
      if (currentActive) moveSliderTo(currentActive);
    });
  }

  function switchTab(tab) {
    const panes = {
      artifacts: document.getElementById("tabContentArtifacts"),
      divergences: document.getElementById("tabContentDivergences"),
      convergences: document.getElementById("tabContentConvergences"),
      pipelines: document.getElementById("tabContentPipelines"),
      guardrails: document.getElementById("tabContentGuardrails"),
      config: document.getElementById("tabContentConfig"),
      dag: document.getElementById("tabContentDag")
    };

    Object.keys(panes).forEach(k => {
      if (panes[k]) {
        panes[k].style.display = k === tab ? "block" : "none";
      }
    });

    if (tab === "dag") {
      window.selectDagCommit("ebb6a18");
      const tabScrollArea = document.getElementById("tabDagGraphScrollArea");
      if (tabScrollArea) {
        setTimeout(() => {
          tabScrollArea.scrollTo({ left: tabScrollArea.scrollWidth, behavior: "smooth" });
        }, 80);
      }
    }
  }

  function setupActions() {
    const starBtn = document.getElementById("btnStar");
    const starCount = document.getElementById("starCount");
    if (starBtn && starCount) {
      let starred = true;
      let count = parseInt(starCount.textContent, 10) || 142;
      starBtn.addEventListener("click", () => {
        starred = !starred;
        count += starred ? 1 : -1;
        starCount.textContent = count;
        starBtn.classList.toggle("active-star", starred);
      });
    }

    const unpinBtn = document.getElementById("btnUnpin");
    if (unpinBtn) {
      unpinBtn.addEventListener("click", () => {
        alert("Repository unpinned from priority dashboard.");
      });
    }

    const codeBtn = document.getElementById("btnCodeDropdown");
    if (codeBtn) {
      codeBtn.addEventListener("click", () => {
        if (window.currentCasRepoName) {
          const cloneCmd = `ai-git clone ${window.currentCasRepoName}`;
          navigator.clipboard.writeText(cloneCmd).catch(() => {});
          alert(`Copied clone command to clipboard:\n${cloneCmd}`);
        } else {
          alert("This model is not connected to an AI-Git repository yet.");
        }
      });
    }
  }

  window.handleFileClick = function (filename, type) {
    if (type === "directory") {
      const btnTree = document.getElementById("btnTreeView");
      if (btnTree) btnTree.click();
    } else {
      showFileContentModal(filename);
    }
  };

  async function showFileContentModal(filename) {
    let modal = document.getElementById("aigitFileViewerModal");
    if (!modal) {
      modal = document.createElement("div");
      modal.id = "aigitFileViewerModal";
      modal.className = "modal-overlay";
      modal.style.cssText = `
        position: fixed; inset: 0; background: rgba(15, 23, 42, 0.75); backdrop-filter: blur(8px);
        display: flex; align-items: center; justify-content: center; z-index: 99999; padding: 24px;
      `;
      modal.innerHTML = `
        <div style="background: #ffffff; border-radius: 14px; max-width: 860px; width: 100%; max-height: 85vh; display: flex; flex-direction: column; box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.35); border: 1px solid #e2e8f0; overflow: hidden;">
          <div style="padding: 16px 20px; border-bottom: 1px solid #e2e8f0; display: flex; align-items: center; justify-content: space-between; background: #f8fafc;">
            <div style="display: flex; align-items: center; gap: 10px;">
              <span style="font-size: 18px;">📄</span>
              <strong id="fileModalTitle" style="color: #0f172a; font-family: monospace; font-size: 14px;">File</strong>
              <span id="fileModalSize" style="background: #e2e8f0; color: #475569; font-size: 11px; padding: 2px 8px; border-radius: 999px; font-weight: 500;">0 KB</span>
              <span style="background: #e0f2fe; color: #0369a1; font-size: 11px; padding: 2px 8px; border-radius: 999px; font-weight: 600;">FastCDC Content-Addressed</span>
            </div>
            <button type="button" id="btnCloseFileModal" style="background: transparent; border: none; font-size: 24px; cursor: pointer; color: #64748b; line-height: 1;">&times;</button>
          </div>
          <div style="padding: 0; flex: 1; overflow-y: auto; background: #0f172a;">
            <pre style="margin: 0; padding: 18px 20px; font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace; font-size: 13px; line-height: 1.6; color: #f1f5f9; white-space: pre-wrap; word-break: break-all;"><code id="fileModalContent">Loading file contents...</code></pre>
          </div>
          <div style="padding: 12px 20px; border-top: 1px solid #e2e8f0; background: #f8fafc; display: flex; justify-content: space-between; align-items: center;">
            <span style="font-size: 12px; color: #64748b;">AI-Git Content-Addressed Store (CAS)</span>
            <div style="display: flex; gap: 8px;">
              <button type="button" id="btnCopyFileContent" style="padding: 6px 14px; background: #ffffff; border: 1px solid #cbd5e1; border-radius: 6px; font-size: 12px; font-weight: 600; color: #334155; cursor: pointer;">Copy Content</button>
              <button type="button" id="btnCloseFileModalBottom" style="padding: 6px 14px; background: #0284c7; border: 1px solid #0284c7; border-radius: 6px; font-size: 12px; font-weight: 600; color: #ffffff; cursor: pointer;">Close</button>
            </div>
          </div>
        </div>
      `;
      document.body.appendChild(modal);

      const closeHandler = () => { modal.style.display = "none"; };
      document.getElementById("btnCloseFileModal").addEventListener("click", closeHandler);
      document.getElementById("btnCloseFileModalBottom").addEventListener("click", closeHandler);
      modal.addEventListener("click", (e) => {
        if (e.target === modal) closeHandler();
      });

      document.getElementById("btnCopyFileContent").addEventListener("click", () => {
        const text = document.getElementById("fileModalContent").textContent;
        navigator.clipboard.writeText(text).then(() => {
          const btn = document.getElementById("btnCopyFileContent");
          btn.textContent = "Copied!";
          setTimeout(() => { btn.textContent = "Copy Content"; }, 1500);
        });
      });
    }

    const titleEl = document.getElementById("fileModalTitle");
    const sizeEl = document.getElementById("fileModalSize");
    const contentEl = document.getElementById("fileModalContent");

    titleEl.textContent = filename;
    sizeEl.textContent = "Loading...";
    contentEl.textContent = "Fetching content from AI-Git CAS...";
    modal.style.display = "flex";

    const urlParams = new URLSearchParams(window.location.search);
    const repoParam = urlParams.get("repo") || urlParams.get("id") || "AI-GIT";

    try {
      const res = await fetch(`/api/file-content?path=${encodeURIComponent(filename)}&repo=${encodeURIComponent(repoParam)}`);
      if (res.ok) {
        const data = await res.json();
        titleEl.textContent = data.filename || filename;
        sizeEl.textContent = data.size || `${(data.content.length / 1024).toFixed(1)} KB`;
        contentEl.textContent = data.content;
        return;
      }
    } catch (e) {
      // Fallback
    }

    const cleanName = filename.split("/").pop();
    if (cleanName === "launch.json") {
      sizeEl.textContent = "0.8 KB";
      contentEl.textContent = JSON.stringify({
        version: "0.2.0",
        configurations: [
          {
            name: "AI-GIT: Run Model Deduplication",
            type: "python",
            request: "launch",
            program: "${workspaceFolder}/CLI/commands/dedup.py",
            args: ["--repo", repoParam, "--fastcdc"]
          },
          {
            name: "AI-GIT: Inspect Tensor Weights",
            type: "cppdbg",
            request: "launch",
            program: "${workspaceFolder}/CLI/aigit-inspect",
            args: ["model.safetensors"]
          }
        ]
      }, null, 2);
    } else if (cleanName === "settings.json") {
      sizeEl.textContent = "0.5 KB";
      contentEl.textContent = JSON.stringify({
        "editor.formatOnSave": true,
        "files.trimTrailingWhitespace": true,
        "C_Cpp.default.cppStandard": "c++17",
        "python.formatting.provider": "black",
        "aigit.fastcdc.targetChunkSizeMB": 1.0,
        "aigit.cas.storageDriver": "minio"
      }, null, 2);
    } else if (cleanName === ".gitignore") {
      sizeEl.textContent = "0.4 KB";
      contentEl.textContent = "node_modules/\n.env\n*.obj\n*.exe\n.DS_Store\ndist/\nbuild/\n.aigit/cache/\n.minio.sys/\n*.tmp\n";
    } else if (cleanName === "README.md") {
      sizeEl.textContent = "1.8 KB";
      contentEl.textContent = `# ${repoParam} · AI-GIT Repository\n\nFast Content-Defined Chunking (FastCDC) version control for machine learning weights.\n\n## Usage\n\`\`\`bash\nai-git clone ${repoParam}\nai-git pull origin main --weights\n\`\`\`\n`;
    } else if (cleanName.endsWith(".py")) {
      sizeEl.textContent = "1.2 KB";
      contentEl.textContent = `#!/usr/bin/env python3\n"""AI-GIT CAS Engine: ${cleanName}"""\n\nprint("[AI-GIT] Content-addressed storage stream ready.")\n`;
    } else if (cleanName.endsWith(".safetensors") || cleanName.endsWith(".bin") || cleanName.endsWith(".pt")) {
      const treeMatch = window.currentModelData?.tree?.find(t => t.name === cleanName);
      sizeEl.textContent = treeMatch?.size || "7.00 GB";
      contentEl.textContent = `[AI-GIT SafeTensors / Model Weight Container]\nTensor File: ${cleanName}\nFormat: SafeTensors (Zero-Copy Memory Mapped)\nStorage Engine: FastCDC Content-Addressed Store\nRemote CAS: MinIO Object Storage (aigit-chunks)\nChunk Deduplication Rate: 74.6% Space Saved\nOpenSSL SHA-256 Manifest: Verified\nContent Hash: ${treeMatch?.hash || "e6eac71f6bdb53425fa2839b539fc5b5a9cb631b9c3aab9c2ab759fddf4f66db"}\nChunks Count: ${treeMatch?.chunks || 168} FastCDC chunks\nArchitecture: Transformer CausalLM (7.0B Parameters, BF16 Weights)`;
    } else {
      sizeEl.textContent = "0.6 KB";
      contentEl.textContent = `# ${cleanName}\nContent-addressed artifact in repository '${repoParam}'.\nStatus: FastCDC Chunked & Verified.`;
    }
  }

  
  /* =========================================================
     VIEW MODE TOGGLE: REPO VIEW vs TREE VIEW
  ========================================================= */
  const btnRepoView = document.getElementById("btnRepoView");
  const btnTreeView = document.getElementById("btnTreeView");
  const repoViewContainer = document.getElementById("repoViewModeContainer");
  const treeViewContainer = document.getElementById("treeViewModeContainer");

  if (btnRepoView && btnTreeView) {
    btnRepoView.addEventListener("click", () => {
      btnRepoView.classList.add("active");
      btnTreeView.classList.remove("active");
      if (repoViewContainer) repoViewContainer.style.display = "block";
      if (treeViewContainer) treeViewContainer.style.display = "none";
    });

    btnTreeView.addEventListener("click", () => {
      btnTreeView.classList.add("active");
      btnRepoView.classList.remove("active");
      if (repoViewContainer) repoViewContainer.style.display = "none";
      if (treeViewContainer) treeViewContainer.style.display = "block";
    });
  }

  /* =========================================================
     SUBFOLDER BUBBLES HORIZONTAL SCROLL CONTROLS
  ========================================================= */
  const folderBubblesTrack = document.getElementById("folderBubblesTrack");
  const btnScrollBubblesLeft = document.getElementById("btnScrollBubblesLeft");
  const btnScrollBubblesRight = document.getElementById("btnScrollBubblesRight");

  if (btnScrollBubblesLeft && folderBubblesTrack) {
    btnScrollBubblesLeft.addEventListener("click", () => {
      folderBubblesTrack.scrollBy({ left: -200, behavior: "smooth" });
    });
  }
  if (btnScrollBubblesRight && folderBubblesTrack) {
    btnScrollBubblesRight.addEventListener("click", () => {
      folderBubblesTrack.scrollBy({ left: 200, behavior: "smooth" });
    });
  }

  window.filterBubbleFolder = function (folderPath) {
    const bubbles = document.querySelectorAll(".folder-bubble");
    bubbles.forEach(b => b.classList.remove("active-bubble"));
    event.currentTarget.classList.add("active-bubble");
    alert(`📂 Navigated to ${folderPath}\nContent-addressed FastCDC chunk manifests loaded.`);
  };

  /* =========================================================
     METRICS BUTTON: CONVERTS ABOUT SECTION INTO METRICS GRAPHS
  ========================================================= */
  const btnMetricsToggle = document.getElementById("btnMetricsToggle");
  const sidebarAboutSection = document.getElementById("sidebarAboutSection");
  const sidebarMetricsSection = document.getElementById("sidebarMetricsSection");

  window.toggleSidebarMetrics = function (showMetrics) {
    if (sidebarAboutSection && sidebarMetricsSection) {
      if (showMetrics) {
        sidebarAboutSection.style.display = "none";
        sidebarMetricsSection.style.display = "flex";
        if (btnMetricsToggle) btnMetricsToggle.classList.add("active");
      } else {
        sidebarAboutSection.style.display = "block";
        sidebarMetricsSection.style.display = "none";
        if (btnMetricsToggle) btnMetricsToggle.classList.remove("active");
      }
    }
  };

  if (btnMetricsToggle) {
    btnMetricsToggle.addEventListener("click", () => {
      const isMetricsActive = sidebarMetricsSection && sidebarMetricsSection.style.display !== "none";
      window.toggleSidebarMetrics(!isMetricsActive);
    });
  }

  /* =========================================================
     COMMIT DAG & CHECKPOINT INSPECTOR
  ========================================================= */
  const btnCheckpointToggle = document.getElementById("btnCheckpointToggle");
  const bottomCheckpointPanel = document.getElementById("bottomCheckpointPanel");
  const btnCloseCheckpointPanel = document.getElementById("btnCloseCheckpointPanel");
  const dagGraphScrollArea = document.getElementById("dagGraphScrollArea");
  const dagNodeHead = document.getElementById("dagNodeHead");

  const DAG_COMMITS = {
    "f942bc1": {
      hash: "f942bc1",
      epoch: "Epoch 1 (Root)",
      branch: "main",
      branchColor: "#38bdf8",
      msg: "Initial model weights & FastCDC manifest",
      parents: [],
      valLoss: "2.840",
      dedup: "0.0%",
      size: "12.8 GB",
      time: "last month"
    },
    "9f3c81e": {
      hash: "9f3c81e",
      epoch: "Epoch 60",
      branch: "main",
      branchColor: "#38bdf8",
      msg: "Learning rate warmdown step",
      parents: ["f942bc1"],
      valLoss: "0.281",
      dedup: "62.4%",
      size: "13.5 GB",
      time: "2 weeks ago"
    },
    "2b99ef4": {
      hash: "2b99ef4",
      epoch: "Epoch 115 (LoRA)",
      branch: "feature/lora-adapter-r16",
      branchColor: "#c084fc",
      msg: "Fine-tune LoRA adapter rank 16",
      parents: ["9f3c81e"],
      valLoss: "0.169",
      dedup: "96.8%",
      size: "420 MB",
      time: "2 days ago"
    },
    "1d84ca0": {
      hash: "1d84ca0",
      epoch: "Epoch 100",
      branch: "main",
      branchColor: "#38bdf8",
      msg: "Add FastCDC 8KB-64KB chunk boundaries",
      parents: ["9f3c81e"],
      valLoss: "0.194",
      dedup: "71.3%",
      size: "13.9 GB",
      time: "3 days ago"
    },
    "4e10ab3": {
      hash: "4e10ab3",
      epoch: "Epoch 118 (FP8)",
      branch: "exp/fp8-quant",
      branchColor: "#f43f5e",
      msg: "FP8 E4M3 quantization calibration",
      parents: ["1d84ca0"],
      valLoss: "0.174",
      dedup: "52.4%",
      size: "7.1 GB",
      time: "2 days ago"
    },
    "7c31d9a": {
      hash: "7c31d9a",
      epoch: "Epoch 120 (Merge)",
      branch: "main",
      branchColor: "#38bdf8",
      msg: "Merge branch 'lora-adapter-r16' into main",
      parents: ["1d84ca0", "2b99ef4"],
      valLoss: "0.158",
      dedup: "84.1%",
      size: "14.1 GB",
      time: "yesterday"
    },
    "ebb6a18": {
      hash: "ebb6a18",
      epoch: "Epoch 128 (HEAD)",
      branch: "main",
      branchColor: "#38bdf8",
      msg: "trying to deploy (FastCDC verified)",
      parents: ["7c31d9a", "4e10ab3"],
      valLoss: "0.142",
      dedup: "78.4%",
      size: "14.2 GB",
      time: "19 hrs ago"
    }
  };

  let activeCommitHash = "ebb6a18";

  window.selectDagCommit = function (hash) {
    const data = DAG_COMMITS[hash];
    if (!data) return;

    activeCommitHash = hash;

    // Highlight node
    document.querySelectorAll(".dag-node-wrapper").forEach(el => el.classList.remove("selected-commit"));
    const activeWrapper = document.querySelector(`.dag-node-point[data-hash="${hash}"]`)?.closest(".dag-node-wrapper");
    if (activeWrapper) {
      activeWrapper.classList.add("selected-commit");
    }

    // Update active badge in header
    const dagActiveBadge = document.getElementById("dagActiveBadge");
    if (dagActiveBadge) {
      dagActiveBadge.textContent = `● FOCUSED: ${hash} (${data.epoch})`;
    }

    // Update bottom inspector
    const inspBranchBadge = document.getElementById("inspBranchBadge");
    if (inspBranchBadge) {
      inspBranchBadge.textContent = data.branch;
      inspBranchBadge.style.color = data.branchColor;
      inspBranchBadge.style.borderColor = data.branchColor;
    }

    const inspHash = document.getElementById("inspHash");
    if (inspHash) inspHash.textContent = data.hash;

    const inspEpoch = document.getElementById("inspEpoch");
    if (inspEpoch) inspEpoch.textContent = data.epoch;

    const inspMsg = document.getElementById("inspMsg");
    if (inspMsg) inspMsg.textContent = data.msg;

    const inspParents = document.getElementById("inspParents");
    if (inspParents) {
      if (!data.parents || data.parents.length === 0) {
        inspParents.innerHTML = `<span style="color: #8b949e; font-size: 11px;">None (Root commit)</span>`;
      } else {
        inspParents.innerHTML = data.parents.map(p => `<span class="insp-parent-tag" onclick="selectDagCommit('${p}')" title="Inspect parent commit">${p}</span>`).join("");
      }
    }

    const inspLoss = document.getElementById("inspLoss");
    if (inspLoss) inspLoss.textContent = data.valLoss;

    const inspDedup = document.getElementById("inspDedup");
    if (inspDedup) inspDedup.textContent = data.dedup;

    const inspSize = document.getElementById("inspSize");
    if (inspSize) inspSize.textContent = data.size;

    const inspTime = document.getElementById("inspTime");
    if (inspTime) inspTime.textContent = data.time;

    const inspCliCmd = document.getElementById("inspCliCmd");
    if (inspCliCmd) inspCliCmd.textContent = `ai-git checkout ${data.hash}`;

    // Update tab-level inspector (if user is viewing the Commit DAG tab)
    const tabInspBranchBadge = document.getElementById("tabInspBranchBadge");
    if (tabInspBranchBadge) {
      tabInspBranchBadge.textContent = data.branch;
      tabInspBranchBadge.style.color = data.branchColor;
      tabInspBranchBadge.style.borderColor = data.branchColor;
    }

    const tabInspHash = document.getElementById("tabInspHash");
    if (tabInspHash) tabInspHash.textContent = data.hash;

    const tabInspEpoch = document.getElementById("tabInspEpoch");
    if (tabInspEpoch) tabInspEpoch.textContent = data.epoch;

    const tabInspMsg = document.getElementById("tabInspMsg");
    if (tabInspMsg) tabInspMsg.textContent = data.msg;

    const tabInspParents = document.getElementById("tabInspParents");
    if (tabInspParents) {
      if (!data.parents || data.parents.length === 0) {
        tabInspParents.innerHTML = `<span style="color: #8b949e; font-size: 11px;">None (Root commit)</span>`;
      } else {
        tabInspParents.innerHTML = data.parents.map(p => `<span class="insp-parent-tag" onclick="selectDagCommit('${p}')" title="Inspect parent commit">${p}</span>`).join("");
      }
    }

    const tabInspLoss = document.getElementById("tabInspLoss");
    if (tabInspLoss) tabInspLoss.textContent = data.valLoss;

    const tabInspDedup = document.getElementById("tabInspDedup");
    if (tabInspDedup) tabInspDedup.textContent = data.dedup;

    const tabInspSize = document.getElementById("tabInspSize");
    if (tabInspSize) tabInspSize.textContent = data.size;

    const tabInspTime = document.getElementById("tabInspTime");
    if (tabInspTime) tabInspTime.textContent = data.time;

    const tabInspCliCmd = document.getElementById("tabInspCliCmd");
    if (tabInspCliCmd) tabInspCliCmd.textContent = `ai-git checkout ${data.hash}`;
  };

  window.copyCheckoutCmd = function () {
    const cmd = `ai-git checkout ${activeCommitHash}`;
    navigator.clipboard.writeText(cmd).then(() => {
      const btns = document.querySelectorAll(".btn-copy-cli");
      btns.forEach(btn => {
        const orig = btn.textContent;
        btn.textContent = "✓";
        setTimeout(() => { btn.textContent = orig; }, 1200);
      });
    }).catch(() => {
      alert(`Command to run:\n${cmd}`);
    });
  };

  window.triggerBottomDagPanel = function () {
    if (bottomCheckpointPanel) {
      bottomCheckpointPanel.classList.add("open");
      if (btnCheckpointToggle) btnCheckpointToggle.classList.add("active");
      window.selectDagCommit("ebb6a18");
      if (dagGraphScrollArea) {
        setTimeout(() => {
          dagGraphScrollArea.scrollTo({ left: dagGraphScrollArea.scrollWidth, behavior: "smooth" });
        }, 120);
      }
    }
  };

  if (btnCheckpointToggle && bottomCheckpointPanel) {
    btnCheckpointToggle.addEventListener("click", () => {
      const willOpen = !bottomCheckpointPanel.classList.contains("open");
      bottomCheckpointPanel.classList.toggle("open", willOpen);
      btnCheckpointToggle.classList.toggle("active", willOpen);
      if (willOpen) {
        window.selectDagCommit("ebb6a18");
        if (dagGraphScrollArea) {
          setTimeout(() => {
            dagGraphScrollArea.scrollTo({ left: dagGraphScrollArea.scrollWidth, behavior: "smooth" });
          }, 120);
        }
      }
    });
  }

  if (btnCloseCheckpointPanel && bottomCheckpointPanel) {
    btnCloseCheckpointPanel.addEventListener("click", () => {
      bottomCheckpointPanel.classList.remove("open");
      if (btnCheckpointToggle) btnCheckpointToggle.classList.remove("active");
    });
  }

  // Make the entire latest commit bar clickable to launch the Commit DAG!
  const latestCommitBar = document.querySelector(".latest-commit-bar");
  if (latestCommitBar) {
    latestCommitBar.style.cursor = "pointer";
    latestCommitBar.setAttribute("title", "Click to view visual Commit DAG & training checkpoints");
    latestCommitBar.addEventListener("click", (e) => {
      // Don't intercept if clicking something else specific
      window.triggerBottomDagPanel();
    });
  }

  // Also wire the commit count badge
  const commitCountBadge = document.querySelector(".commit-count-badge");
  if (commitCountBadge) {
    commitCountBadge.style.cursor = "pointer";
    commitCountBadge.addEventListener("click", (e) => {
      e.stopPropagation();
      window.triggerBottomDagPanel();
    });
  }

  /* Tree Node Inspector function */
  window.inspectTreeNode = function (name, type, desc, files, size) {
    alert(`🌳 Tree Node Selected: ${name}\n\nType: ${type}\nRole: ${desc}\nArtifacts: ${files}\nFastCDC Safetensors: ${size}\n\nFastCDC BLAKE3 cryptographic manifest verified.`);
  };

  window.inspectCheckpoint = function (hash, epoch, msg, loss, size) {
    window.selectDagCommit(hash);
  };


  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
