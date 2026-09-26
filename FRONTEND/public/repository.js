/* =========================================================
   AI-GIT REPOSITORY & NODE CONTROLS
   Smoothly moving slider bar under active tab
========================================================= */

(function () {
  let activeTab = "artifacts";

  function init() {
    const urlParams = new URLSearchParams(window.location.search);
    const repoParam = urlParams.get("repo") || urlParams.get("id") || "AI-GIT";

    const titleDisplay = document.getElementById("nodeNameDisplay");
    if (titleDisplay) titleDisplay.textContent = repoParam;

    document.title = `${repoParam} · AI-GIT Node`;

    setupTabsWithSmoothSlider();
    setupActions();
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
        const repoName = document.getElementById("nodeNameDisplay")?.textContent || "AI-GIT";
        const cloneCmd = `ai-git clone ${repoName}`;
        navigator.clipboard.writeText(cloneCmd);
        alert(`Copied clone command to clipboard:\n${cloneCmd}`);
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
     CHECKPOINT BUTTON: OPENS BOTTOM PANEL & FOCUSES CURRENT COMMIT
  ========================================================= */
  const btnCheckpointToggle = document.getElementById("btnCheckpointToggle");
  const bottomCheckpointPanel = document.getElementById("bottomCheckpointPanel");
  const btnCloseCheckpointPanel = document.getElementById("btnCloseCheckpointPanel");
  const checkpointSidewaysTrack = document.getElementById("checkpointSidewaysTrack");
  const currentCommitCard = document.getElementById("currentCommitCard");

  if (btnCheckpointToggle && bottomCheckpointPanel) {
    btnCheckpointToggle.addEventListener("click", () => {
      bottomCheckpointPanel.classList.toggle("open");
      if (bottomCheckpointPanel.classList.contains("open")) {
        // Focus on current commit by scrolling sideways to it
        if (currentCommitCard && checkpointSidewaysTrack) {
          setTimeout(() => {
            currentCommitCard.scrollIntoView({ behavior: "smooth", inline: "center", block: "nearest" });
          }, 100);
        }
      }
    });
  }

  if (btnCloseCheckpointPanel && bottomCheckpointPanel) {
    btnCloseCheckpointPanel.addEventListener("click", () => {
      bottomCheckpointPanel.classList.remove("open");
    });
  }

  window.inspectCheckpoint = function (hash, epoch, msg, loss, size) {
    alert(`⚡ Checkpoint Inspector\nHash: ${hash}\nEpoch: ${epoch}\nMessage: ${msg}\nValidation Metric: ${loss}\nFastCDC Safetensors: ${size}\n\nTo pull this specific checkpoint:\nai-git checkout ${hash}`);
  };


  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
