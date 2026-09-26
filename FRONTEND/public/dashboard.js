/* =========================================================
   AI-GIT MODEL HUB & DASHBOARD CLIENT JAVASCRIPT
========================================================= */

(function () {
  async function loadDashboardData() {
    try {
      try {
        const metricsRes = await fetch("/api/metrics");
        if (metricsRes.ok) {
          const m = await metricsRes.json();
          renderMetrics(m);
        }
      } catch (e) {}

      let models = [];
      try {
        const modelsRes = await fetch("/api/models");
        if (modelsRes.ok) {
          models = await modelsRes.json();
          renderModelRepositories(models);
          renderRecentActivity(models);
        }
      } catch (e) {}

      initLeftPanel(models);
    } catch (e) {
      console.error("Error loading dashboard data:", e);
      initLeftPanel([]);
    }
  }

  function renderMetrics(m) {
    const elSaved = document.getElementById("metricSpaceSaved");
    const elSavedSub = document.getElementById("metricSpaceSavedSub");
    const elDedup = document.getElementById("metricDedupRate");
    const elDedupSub = document.getElementById("metricDedupSub");
    const elModels = document.getElementById("metricTotalModels");
    const elBandwidth = document.getElementById("metricBandwidth");

    if (elSaved) elSaved.textContent = `${m.spaceSavedGB} GB`;
    if (elSavedSub) elSavedSub.textContent = `${m.savingsPercent}% Physical Disk Reduction`;
    if (elDedup) elDedup.textContent = `${m.deduplicationRate}%`;
    if (elDedupSub) elDedupSub.textContent = `${m.deduplicatedChunks.toLocaleString()} / ${m.totalChunks.toLocaleString()} Chunks Shared`;
    if (elModels) elModels.textContent = `${m.totalModels} Models`;
    if (elBandwidth) elBandwidth.textContent = `${(parseFloat(m.spaceSavedGB) * 0.92).toFixed(1)} GB`;

    const barAigit = document.getElementById("barFillAigit");
    const aigiLabel = document.getElementById("aigitBarLabel");
    if (barAigit && m.savingsPercent) {
      const remainingPercent = (100 - parseFloat(m.savingsPercent)).toFixed(1);
      barAigit.style.width = `${Math.max(15, remainingPercent)}%`;
      if (aigiLabel) aigiLabel.textContent = `AI-GIT FastCDC: ${m.totalCasGB} GB (${m.savingsPercent}% Saved)`;
    }
  }

  function renderModelRepositories(models) {
    const container = document.getElementById("repoCardsContainer");
    if (!container) return;

    if (!models || !models.length) {
      container.innerHTML = '<div style="padding: 30px; text-align: center; color: var(--text-muted); background: var(--bg-card); border-radius: var(--radius-md);">No model repositories tracked yet. Click "+ New Model" to create one.</div>';
      return;
    }

    container.innerHTML = models.map(m => {
      const savingsPct = m.metrics ? m.metrics.savingsPercentage : 0;
      return `
        <a href="repository.html?id=${encodeURIComponent(m.id)}" class="repo-card">
          <div class="repo-card-top">
            <div class="repo-title-row">
              <span class="repo-name">${m.name}</span>
              <span class="pill-badge badge-purple">${m.parameters || 'Weights'}</span>
              <span class="pill-badge badge-blue">${m.framework}</span>
              <span class="pill-badge badge-pink">${m.precision}</span>
            </div>
            <div style="font-size: 11.5px; font-weight: 700; color: var(--accent-pink);">
              ⚡ ${savingsPct}% Space Saved
            </div>
          </div>
          <p class="repo-desc">${m.description}</p>
          <div class="repo-meta-row">
            <span class="meta-item">
              <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
              <span>${m.epoch || 'Trained'}</span>
            </span>
            <span class="meta-item">
              <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"/><polyline points="22 4 12 14.01 9 11.01"/></svg>
              <span>val_loss: ${m.valLoss || '0.00'}</span>
            </span>
            <span class="meta-item">
              <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg>
              <span>${m.stars || 0}</span>
            </span>
            <span class="meta-item" style="margin-left: auto;">Updated ${m.updatedAt || 'Recent'}</span>
          </div>
        </a>
      `;
    }).join("");
  }

  function renderRecentActivity(models) {
    const list = document.getElementById("activityFeedList");
    if (!list) return;

    const activities = [
      { icon: "⚡", title: "Epoch 18 weights checkpoint pushed to <strong>llama-3-8b-instruct</strong>", time: "2 hours ago" },
      { icon: "🔄", title: "FastCDC deduplicated 950 weight chunks in <strong>stable-diffusion-xl-base</strong>", time: "4 hours ago" },
      { icon: "🐛", title: "New issue opened: <em>FP16 precision overflow in Attention</em>", time: "6 hours ago" },
      { icon: "💬", title: "Discussion started: <em>How FastCDC Deduplication Cuts Costs</em>", time: "Yesterday" },
      { icon: "🚀", title: "Multilingual ASR weights released in <strong>whisper-large-v3</strong>", time: "2 days ago" }
    ];

    list.innerHTML = activities.map(act => `
      <li class="activity-item">
        <div class="activity-icon">${act.icon}</div>
        <div class="activity-content">
          <div>${act.title}</div>
          <div class="activity-time">${act.time}</div>
        </div>
      </li>
    `).join("");
  }

  const DEFAULT_NODES = [
    "Aarya-2601/AI-GIT",
    "rushil23shah-hue/PixelPlot",
    "chaitijain593-prog/sagar-mitra-2.0",
    "Aarya-2601/Keya_Sishterrr_26",
    "Aarya-2601/bw3_website",
    "Aarya-2601/task3_submission",
    "Aarya-2601/gaming-arcade",
    "meta-llama/llama-3-8b-instruct",
    "stabilityai/sdxl-lightning",
    "openai/whisper-large-v3",
    "mistralai/mistral-7b-v0.2",
    "google/gemma-2-9b"
  ];

  let isShowingAllNodes = false;
  let activeNodesList = [...DEFAULT_NODES];

  function initLeftPanel(serverModels = []) {
    try {
      const stored = localStorage.getItem("aigit_user");
      if (stored) {
        const u = JSON.parse(stored);
        const nameEl = document.getElementById("panelUserName");
        const avatarEl = document.getElementById("sideUserAvatar");
        if (nameEl && u.username) nameEl.textContent = u.username;
        if (avatarEl && (u.username || u.name)) {
          avatarEl.textContent = (u.username || u.name).charAt(0).toUpperCase();
        }
      }
    } catch (e) {}

    if (serverModels && serverModels.length) {
      serverModels.forEach(m => {
        const fullName = `Aarya-2601/${m.name || m.id}`;
        if (!activeNodesList.includes(fullName)) {
          activeNodesList.unshift(fullName);
        }
      });
    }

    renderTreeNodes();

    const btnToggle = document.getElementById("btnToggleShowMoreNodes");
    if (btnToggle) {
      btnToggle.addEventListener("click", () => {
        isShowingAllNodes = !isShowingAllNodes;
        const container = document.getElementById("nodesTreeContainer");
        if (container) {
          container.classList.toggle("scrollable", isShowingAllNodes);
        }
        btnToggle.textContent = isShowingAllNodes ? "Show less" : "Show more";
        applyNodeVisibility();
      });
    }

    const searchInput = document.getElementById("nodeSearchInput");
    if (searchInput) {
      searchInput.addEventListener("input", (e) => {
        const query = e.target.value.toLowerCase().trim();
        filterNodes(query);
      });
    }

    const btnSideNewNode = document.getElementById("btnSideNewNode");
    if (btnSideNewNode) {
      btnSideNewNode.addEventListener("click", () => {
        const topNewBtn = document.getElementById("btnOpenNewRepoModal");
        if (topNewBtn) {
          topNewBtn.click();
        } else {
          const modal = document.getElementById("newRepoModal");
          if (modal) modal.classList.add("active");
        }
      });
    }
  }

  function renderTreeNodes() {
    const container = document.getElementById("nodesTreeContainer");
    if (!container) return;

    container.innerHTML = activeNodesList.map((repoName, idx) => {
      const isHidden = idx >= 5 && !isShowingAllNodes;
      return `
        <div class="node-tree-item ${isHidden ? 'hidden-node' : ''}" data-repo-name="${repoName}" data-index="${idx}">
          <div class="node-branch-connector">
            <svg class="node-branch-svg" width="28" height="34" viewBox="0 0 28 34" fill="none">
              <line class="branch-vline" x1="10" y1="0" x2="10" y2="34" stroke="#ffffff" stroke-width="1.8" />
              <line x1="10" y1="17" x2="22" y2="17" stroke="#ffffff" stroke-width="1.8" />
              <polygon points="21,13 28,17 21,21" fill="#ffffff" />
            </svg>
          </div>
          <a href="repository.html?name=${encodeURIComponent(repoName)}" class="node-repo-link" title="${repoName}">
            <span class="node-repo-text">${repoName}</span>
          </a>
        </div>
      `;
    }).join("");

    updateTreeConnectorLines();
  }

  function applyNodeVisibility() {
    const items = document.querySelectorAll(".node-tree-item");
    items.forEach((item, idx) => {
      if (!isShowingAllNodes && idx >= 5) {
        item.classList.add("hidden-node");
      } else {
        item.classList.remove("hidden-node");
      }
    });
    updateTreeConnectorLines();
  }

  function filterNodes(query) {
    const items = document.querySelectorAll(".node-tree-item");
    const btnToggle = document.getElementById("btnToggleShowMoreNodes");
    const container = document.getElementById("nodesTreeContainer");

    if (!query) {
      if (btnToggle) btnToggle.style.display = "inline-flex";
      applyNodeVisibility();
      return;
    }

    if (btnToggle) btnToggle.style.display = "none";
    if (container) container.classList.add("scrollable");

    items.forEach(item => {
      const name = (item.getAttribute("data-repo-name") || "").toLowerCase();
      if (name.includes(query)) {
        item.classList.remove("hidden-node");
      } else {
        item.classList.add("hidden-node");
      }
    });

    updateTreeConnectorLines();
  }

  function updateTreeConnectorLines() {
    const visibleItems = Array.from(document.querySelectorAll(".node-tree-item:not(.hidden-node)"));
    visibleItems.forEach((item, idx) => {
      const vline = item.querySelector(".branch-vline");
      if (!vline) return;
      if (idx === visibleItems.length - 1) {
        vline.setAttribute("y2", "17");
      } else {
        vline.setAttribute("y2", "34");
      }
    });
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", loadDashboardData);
  } else {
    loadDashboardData();
  }
})();
