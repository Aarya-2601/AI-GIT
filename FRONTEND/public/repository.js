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
      const modelRes = await fetch(`/api/models/${repoParam}`);
      if (modelRes.ok) {
        const model = await modelRes.json();
        window.currentCasRepoName = model.casRepoName || null;

        if (model.casRepoName) {
          const backendRes = await fetch(`/api/backend/repos/${encodeURIComponent(model.casRepoName)}`);
          if (backendRes.ok) {
            const backendData = await backendRes.json();
            const headEl = document.getElementById("backendHead");
            const countEl = document.getElementById("backendObjectCount");
            const updatedEl = document.getElementById("backendUpdatedAt");
            if (headEl && backendData.HEAD) headEl.textContent = backendData.HEAD;
            if (countEl && backendData.objectCount !== undefined) countEl.textContent = backendData.objectCount;
            if (updatedEl && backendData.updated_at) updatedEl.textContent = backendData.updated_at;
          }
        }
      }
    } catch (err) {
      console.error("Error fetching repository metadata:", err);
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
      config: document.getElementById("tabContentConfig")
    };

    Object.keys(panes).forEach(k => {
      if (panes[k]) {
        panes[k].style.display = k === tab ? "block" : "none";
      }
    });
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
      alert(`📁 Directory: ${filename}\nOpening sub-tree explorer...`);
    } else {
      alert(`📄 File: ${filename}\nContent-addressed hash: 8f9b231c9e47...`);
    }
  };

  
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

    // Populate inspector dock
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
  };

  window.copyCheckoutCmd = function () {
    const cmd = `ai-git checkout ${activeCommitHash}`;
    navigator.clipboard.writeText(cmd).then(() => {
      const btn = document.querySelector(".btn-copy-cli");
      if (btn) {
        const orig = btn.textContent;
        btn.textContent = "✓";
        setTimeout(() => { btn.textContent = orig; }, 1200);
      }
    }).catch(() => {
      alert(`Command to run:\n${cmd}`);
    });
  };

  if (btnCheckpointToggle && bottomCheckpointPanel) {
    btnCheckpointToggle.addEventListener("click", () => {
      bottomCheckpointPanel.classList.toggle("open");
      if (bottomCheckpointPanel.classList.contains("open")) {
        // Focus on HEAD node & select it
        window.selectDagCommit("ebb6a18");
        if (dagGraphScrollArea && dagNodeHead) {
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
