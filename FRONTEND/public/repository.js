/* =========================================================
   AI-GIT REPOSITORY DETAIL & VISUAL COMMIT GRAPH JAVASCRIPT
========================================================= */

(function () {
  let modelData = null;
  let activeTab = "files";
  let activeCommit = null;

  async function init() {
    const urlParams = new URLSearchParams(window.location.search);
    const modelId = urlParams.get("id") || "llama-3-8b-instruct";

    setupTabs();
    await loadModel(modelId);
  }

  function setupTabs() {
    const tabs = document.querySelectorAll(".repo-tab-link");
    tabs.forEach(tab => {
      tab.addEventListener("click", () => {
        tabs.forEach(t => t.classList.remove("active"));
        tab.classList.add("active");
        activeTab = tab.dataset.tab;
        switchTab(activeTab);
      });
    });

    if (window.location.hash === "#graph") {
      const graphTab = document.querySelector('[data-tab="graph"]');
      if (graphTab) graphTab.click();
    }
  }

  function switchTab(tab) {
    const viewFiles = document.getElementById("tabViewFiles");
    const viewGraph = document.getElementById("tabViewGraph");
    const viewSpecs = document.getElementById("tabViewSpecs");
    const viewDedup = document.getElementById("tabViewDedup");

    if (viewFiles) viewFiles.style.display = tab === "files" ? "block" : "none";
    if (viewGraph) viewGraph.style.display = tab === "graph" ? "block" : "none";
    if (viewSpecs) viewSpecs.style.display = tab === "specs" ? "block" : "none";
    if (viewDedup) viewDedup.style.display = tab === "dedup" ? "block" : "none";

    if (tab === "graph" && modelData) {
      renderVisualCommitGraph(modelData.commits || []);
    }
  }

  async function loadModel(id) {
    try {
      const res = await fetch(`/api/models/${encodeURIComponent(id)}`);
      if (res.ok) {
        modelData = await res.json();
        renderHeader(modelData);
        renderFiles(modelData.tree || []);
        renderSpecs(modelData);
        renderDedupStats(modelData);
        if (modelData.commits && modelData.commits.length) {
          activeCommit = modelData.commits[0];
          inspectCommit(activeCommit);
          if (activeTab === "graph") {
            renderVisualCommitGraph(modelData.commits);
          }
        }
      } else {
        document.querySelector(".repo-detail-container").innerHTML = `
          <div style="padding: 40px; text-align: center; color: var(--text-muted); background: var(--bg-card); border-radius: var(--radius-md);">
            Repository "${id}" not found. <a href="repositories.html">Return to Model Hub</a>
          </div>`;
      }
    } catch (e) {
      console.error("Failed to load model:", e);
    }
  }

  function renderHeader(m) {
    document.title = `${m.name} · AI-GIT Repository`;
    const repoTitleEl = document.getElementById("repoTitleText");
    const repoDescEl = document.getElementById("repoDescText");
    const cloneInputEl = document.getElementById("cloneCmdText");
    const copyBtn = document.getElementById("btnCopyClone");

    if (repoTitleEl) repoTitleEl.textContent = m.name;
    if (repoDescEl) repoDescEl.textContent = m.description;
    if (cloneInputEl) cloneInputEl.textContent = `ai-git clone ${m.name}`;

    if (copyBtn) {
      copyBtn.addEventListener("click", () => {
        navigator.clipboard.writeText(`ai-git clone ${m.name}`);
        copyBtn.textContent = "Copied!";
        setTimeout(() => copyBtn.textContent = "Copy", 2000);
      });
    }

    const starBtn = document.getElementById("btnStarRepo");
    if (starBtn) {
      let starred = false;
      let count = m.stars || 0;
      starBtn.addEventListener("click", () => {
        starred = !starred;
        count += starred ? 1 : -1;
        starBtn.innerHTML = `<span>${starred ? '★ Starred' : '☆ Star'}</span> <span class="pill-badge badge-purple" style="font-size: 11px;">${count}</span>`;
      });
    }
  }

  /* =========================================================
     RENDER: FILE BROWSER
  ========================================================= */
  function renderFiles(tree) {
    const listEl = document.getElementById("fileBrowserList");
    if (!listEl) return;

    listEl.innerHTML = tree.map(item => {
      const isDir = item.type === "directory";
      const isModelWeights = item.name.endsWith(".safetensors") || item.name.endsWith(".bin") || item.name.endsWith(".gguf") || item.name.endsWith(".pt");

      return `
        <div class="file-item-row" data-name="${item.name}" data-type="${item.type}">
          <div class="file-name-cell" onclick="handleFileClick('${item.name}', '${item.type}')">
            <span style="color: ${isDir ? 'var(--accent-purple)' : isModelWeights ? 'var(--accent-pink)' : 'var(--text-muted)'};">
              ${isDir ? '📁' : isModelWeights ? '🧠' : '📄'}
            </span>
            <span style="font-weight: 600; color: ${isDir ? 'var(--accent-purple)' : 'var(--text-primary)'};">${item.name}</span>
            ${isModelWeights ? `<span class="pill-badge badge-pink" style="font-size: 10px;">FastCDC Chunks</span>` : ''}
          </div>
          <div class="file-msg-cell">
            ${isModelWeights ? `checkpoint: Saved weights @ epoch ${modelData.epoch || 'best'} (${item.chunks || 12} chunks)` : `Add ${item.name} architecture configuration`}
          </div>
          <div class="file-size-cell">${item.size || '1 KB'}</div>
          <div class="file-time-cell">${modelData.updatedAt || 'Recently'}</div>
        </div>
      `;
    }).join("");
  }

  window.handleFileClick = function (name, type) {
    const item = modelData.tree.find(t => t.name === name);
    if (!item) return;

    if (type === "directory") {
      alert(`Directory: ${name}\nContains ${item.children?.length || 0} sub-artifacts. Navigate to "Whole Tree Explorer" in Model Repos tab to explore deep hierarchies.`);
    } else {
      const isModelWeights = name.endsWith(".safetensors") || name.endsWith(".bin") || name.endsWith(".gguf") || name.endsWith(".pt");
      if (isModelWeights) {
        alert(`[AI-GIT FastCDC Chunker]\nFile: ${name}\nSize: ${item.size}\nTotal Chunks: ${item.chunks || 12}\nDeduplicated: ${item.deduped || 9} chunks shared from previous epoch!\nContent Hash: ${item.hash || 'N/A'}`);
      } else {
        alert(`File: ${name}\nSize: ${item.size}\nHash: ${item.hash || 'N/A'}`);
      }
    }
  };

  /* =========================================================
     RENDER: VISUAL COMMIT GRAPH (INTERACTIVE DAG)
  ========================================================= */
  function renderVisualCommitGraph(commits) {
    const svg = document.getElementById("commitGraphSvg");
    if (!svg || !commits.length) return;

    const startX = 60;
    const nodeSpacingX = 140;
    const mainY = 60;
    const branchY = 140;

    let elementsHTML = "";

    // Draw connecting lines first
    for (let i = 0; i < commits.length - 1; i++) {
      const cCurrent = commits[i];
      const cPrev = commits[i + 1];

      const x1 = startX + i * nodeSpacingX;
      const y1 = cCurrent.branch === "lora-finetune" ? branchY : mainY;

      const x2 = startX + (i + 1) * nodeSpacingX;
      const y2 = cPrev.branch === "lora-finetune" ? branchY : mainY;

      if (y1 === y2) {
        // Straight line
        elementsHTML += `
          <line x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" stroke="#3b4d6e" stroke-width="3" stroke-linecap="round"/>
        `;
      } else {
        // Smooth bezier curve for branch / merge
        const midX = (x1 + x2) / 2;
        elementsHTML += `
          <path d="M ${x1} ${y1} C ${midX} ${y1}, ${midX} ${y2}, ${x2} ${y2}" fill="none" stroke="#a855f7" stroke-width="3" stroke-dasharray="4,4"/>
        `;
      }
    }

    // Draw nodes
    commits.forEach((c, i) => {
      const cx = startX + i * nodeSpacingX;
      const cy = c.branch === "lora-finetune" ? branchY : mainY;
      const isSelected = activeCommit?.hash === c.hash;
      const nodeColor = c.branch === "lora-finetune" ? "#c084fc" : (i === 0 ? "#f43f5e" : "#38bdf8");

      elementsHTML += `
        <g class="graph-node-group" onclick="selectCommitByHash('${c.hash}')">
          <!-- Outer glow -->
          <circle cx="${cx}" cy="${cy}" r="${isSelected ? 16 : 12}" fill="${nodeColor}" fill-opacity="${isSelected ? 0.4 : 0.15}"/>
          <!-- Main node -->
          <circle cx="${cx}" cy="${cy}" r="${isSelected ? 9 : 7}" fill="${nodeColor}" stroke="#ffffff" stroke-width="2" class="graph-node-circle"/>
          
          <!-- Epoch Tag Badge -->
          <rect x="${cx - 36}" y="${cy - 38}" width="72" height="20" rx="4" fill="#131929" stroke="${nodeColor}" stroke-width="1.2"/>
          <text x="${cx}" y="${cy - 24}" text-anchor="middle" font-size="10.5" font-weight="700" fill="${nodeColor}" font-family="monospace">${c.epoch || 'Commit'}</text>

          <!-- Short hash -->
          <text x="${cx}" y="${cy + 25}" text-anchor="middle" font-size="11" fill="#94a3b8" font-family="monospace">${c.hash}</text>
          
          <!-- Val Loss tag if present -->
          ${c.valLoss && c.valLoss !== 'N/A' ? `
            <text x="${cx}" y="${cy + 39}" text-anchor="middle" font-size="10" fill="#4ade80" font-weight="600">loss: ${c.valLoss}</text>
          ` : ''}
        </g>
      `;
    });

    svg.innerHTML = elementsHTML;
    svg.setAttribute("viewBox", `0 0 ${Math.max(800, startX + commits.length * nodeSpacingX + 80)} 200`);
  }

  window.selectCommitByHash = function (hash) {
    if (!modelData || !modelData.commits) return;
    const commit = modelData.commits.find(c => c.hash === hash);
    if (commit) {
      activeCommit = commit;
      inspectCommit(commit);
      renderVisualCommitGraph(modelData.commits);
    }
  };

  function inspectCommit(c) {
    const drawer = document.getElementById("commitDetailsDrawer");
    if (!drawer || !c) return;

    drawer.innerHTML = `
      <div class="inspect-commit-header">
        <div style="display: flex; align-items: center; gap: 10px;">
          <span class="pill-badge badge-purple" style="font-size: 13px;">${c.epoch || 'Commit'}</span>
          <span style="font-family: var(--font-mono); font-size: 13px; color: var(--accent-blue);">${c.fullHash}</span>
        </div>
        <div style="font-size: 12px; color: var(--text-muted);">${c.date} by <strong>${c.author}</strong></div>
      </div>

      <div style="font-size: 15px; font-weight: 600; color: var(--text-primary); margin: 6px 0;">
        ${c.message}
      </div>

      <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 12px; margin-top: 8px;">
        <div style="background: var(--bg-card); padding: 12px; border-radius: var(--radius-sm); border: 1px solid var(--border-subtle);">
          <div style="font-size: 11px; color: var(--text-muted); text-transform: uppercase;">Validation Loss</div>
          <div style="font-size: 18px; font-weight: 700; color: #4ade80;">${c.valLoss || 'N/A'}</div>
        </div>
        <div style="background: var(--bg-card); padding: 12px; border-radius: var(--radius-sm); border: 1px solid var(--border-subtle);">
          <div style="font-size: 11px; color: var(--text-muted); text-transform: uppercase;">Branch Pointer</div>
          <div style="font-size: 16px; font-weight: 700; color: var(--accent-purple);">refs/heads/${c.branch}</div>
        </div>
        <div style="background: var(--bg-card); padding: 12px; border-radius: var(--radius-sm); border: 1px solid var(--border-subtle);">
          <div style="font-size: 11px; color: var(--text-muted); text-transform: uppercase;">FastCDC Chunks</div>
          <div style="font-size: 16px; font-weight: 700; color: var(--accent-pink);">
            +${c.newChunks || 0} new &middot; ${c.reusedChunks || 0} deduplicated
          </div>
        </div>
      </div>
    `;
  }

  /* =========================================================
     RENDER: MODEL SPECS & HYPERPARAMETERS
  ========================================================= */
  function renderSpecs(m) {
    const container = document.getElementById("tabViewSpecs");
    if (!container) return;

    container.innerHTML = `
      <div style="background: var(--bg-card); border: 1px solid var(--border-default); border-radius: var(--radius-lg); padding: 26px;">
        <div class="section-title" style="margin-bottom: 20px;">
          <span>Model Architecture & Deep Learning Hyperparameters</span>
        </div>

        <div style="display: grid; grid-template-columns: repeat(auto-fill, minmax(280px, 1fr)); gap: 18px;">
          <div style="background: var(--bg-surface); padding: 16px; border-radius: var(--radius-md); border: 1px solid var(--border-subtle);">
            <div style="font-size: 12px; color: var(--text-muted);">Parameter Count</div>
            <div style="font-size: 20px; font-weight: 700; color: var(--accent-purple); margin-top: 4px;">${m.parameters}</div>
          </div>
          <div style="background: var(--bg-surface); padding: 16px; border-radius: var(--radius-md); border: 1px solid var(--border-subtle);">
            <div style="font-size: 12px; color: var(--text-muted);">Weight Storage Format</div>
            <div style="font-size: 20px; font-weight: 700; color: var(--accent-blue); margin-top: 4px;">${m.framework}</div>
          </div>
          <div style="background: var(--bg-surface); padding: 16px; border-radius: var(--radius-md); border: 1px solid var(--border-subtle);">
            <div style="font-size: 12px; color: var(--text-muted);">Precision / Quantization</div>
            <div style="font-size: 20px; font-weight: 700; color: var(--accent-pink); margin-top: 4px;">${m.precision}</div>
          </div>
          <div style="background: var(--bg-surface); padding: 16px; border-radius: var(--radius-md); border: 1px solid var(--border-subtle);">
            <div style="font-size: 12px; color: var(--text-muted);">Context Window Length</div>
            <div style="font-size: 20px; font-weight: 700; color: var(--text-primary); margin-top: 4px;">${m.contextLength || '4,096 tokens'}</div>
          </div>
        </div>

        <div style="margin-top: 24px;">
          <h4 style="font-size: 14px; margin-bottom: 8px;">Neural Architecture Details</h4>
          <p style="font-size: 13.5px; color: var(--text-secondary); line-height: 1.6;">${m.architecture}</p>
        </div>
      </div>
    `;
  }

  /* =========================================================
     RENDER: STORAGE DEDUPLICATION STATS
  ========================================================= */
  function renderDedupStats(m) {
    const container = document.getElementById("tabViewDedup");
    if (!container) return;

    const rawGB = (m.metrics?.rawSizeBytes / (1024 ** 3)).toFixed(1);
    const casGB = (m.metrics?.casSizeBytes / (1024 ** 3)).toFixed(1);
    const savedGB = (m.metrics?.spaceSavedBytes / (1024 ** 3)).toFixed(1);

    container.innerHTML = `
      <div style="background: var(--bg-card); border: 1px solid var(--border-default); border-radius: var(--radius-lg); padding: 26px;">
        <div class="section-title" style="margin-bottom: 12px;">
          <span>FastCDC Storage Deduplication Breakdown</span>
        </div>
        <p style="font-size: 13.5px; color: var(--text-secondary); margin-bottom: 24px;">
          AI-GIT applies dynamic content-defined boundaries to prevent storing duplicate neural weights between checkpoint iterations.
        </p>

        <div style="display: grid; grid-template-columns: repeat(3, 1fr); gap: 16px; text-align: center; margin-bottom: 26px;">
          <div style="background: var(--bg-surface); padding: 20px; border-radius: var(--radius-md); border: 1px solid var(--border-subtle);">
            <div style="font-size: 12px; color: var(--text-muted);">Total Virtual Size</div>
            <div style="font-size: 26px; font-weight: 800; color: var(--text-primary); margin-top: 4px;">${rawGB} GB</div>
          </div>
          <div style="background: var(--bg-surface); padding: 20px; border-radius: var(--radius-md); border: 1px solid var(--border-subtle);">
            <div style="font-size: 12px; color: var(--text-muted);">Actual CAS Stored on Disk</div>
            <div style="font-size: 26px; font-weight: 800; color: var(--accent-blue); margin-top: 4px;">${casGB} GB</div>
          </div>
          <div style="background: var(--bg-surface); padding: 20px; border-radius: var(--radius-md); border: 1px solid var(--border-subtle);">
            <div style="font-size: 12px; color: var(--text-muted);">Disk Space Saved</div>
            <div style="font-size: 26px; font-weight: 800; color: var(--accent-pink); margin-top: 4px;">${savedGB} GB (${m.metrics?.savingsPercentage}%)</div>
          </div>
        </div>
      </div>
    `;
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
