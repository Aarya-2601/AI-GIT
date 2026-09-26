/* =========================================================
   AI-GIT REPOSITORIES & TREE EXPLORER CLIENT JAVASCRIPT
========================================================= */

(function () {
  let allModels = [];
  let currentView = "cards"; // 'cards' or 'tree'
  let selectedNode = null;

  async function init() {
    const urlParams = new URLSearchParams(window.location.search);
    const viewParam = urlParams.get("view");
    const queryParam = urlParams.get("q");

    if (viewParam === "tree") {
      currentView = "tree";
    }

    if (queryParam) {
      const searchInput = document.getElementById("repoFilterSearch");
      if (searchInput) searchInput.value = queryParam;
    }

    setupViewToggle();
    setupFilters();
    await loadModels();
  }

  function setupViewToggle() {
    const btnCards = document.getElementById("viewModeCards");
    const btnTree = document.getElementById("viewModeTree");
    const containerCards = document.getElementById("reposGridMode");
    const containerTree = document.getElementById("reposTreeMode");

    function applyView(mode) {
      currentView = mode;
      if (mode === "cards") {
        btnCards.classList.add("active");
        btnTree.classList.remove("active");
        containerCards.style.display = "grid";
        containerTree.style.display = "none";
      } else {
        btnTree.classList.add("active");
        btnCards.classList.remove("active");
        containerCards.style.display = "none";
        containerTree.style.display = "grid";
        renderTreeExplorer(allModels);
      }
    }

    if (btnCards && btnTree) {
      btnCards.addEventListener("click", () => applyView("cards"));
      btnTree.addEventListener("click", () => applyView("tree"));
      applyView(currentView);
    }
  }

  function setupFilters() {
    const searchInput = document.getElementById("repoFilterSearch");
    const frameworkSelect = document.getElementById("frameworkFilter");

    if (searchInput) {
      searchInput.addEventListener("input", filterAndRender);
    }
    if (frameworkSelect) {
      frameworkSelect.addEventListener("change", filterAndRender);
    }
  }

  async function loadModels() {
    try {
      const res = await fetch("/api/models");
      if (res.ok) {
        allModels = await res.json();
        filterAndRender();
        if (currentView === "tree") {
          renderTreeExplorer(allModels);
        }
      }
    } catch (e) {
      console.error("Could not load models:", e);
    }
  }

  function filterAndRender() {
    const query = (document.getElementById("repoFilterSearch")?.value || "").toLowerCase().trim();
    const framework = (document.getElementById("frameworkFilter")?.value || "all").toLowerCase();

    let filtered = allModels;
    if (query) {
      filtered = filtered.filter(m =>
        m.name.toLowerCase().includes(query) ||
        m.description.toLowerCase().includes(query) ||
        m.family.toLowerCase().includes(query) ||
        (m.tags && m.tags.some(t => t.toLowerCase().includes(query)))
      );
    }
    if (framework !== "all") {
      filtered = filtered.filter(m => m.framework.toLowerCase() === framework);
    }

    renderCards(filtered);
    if (currentView === "tree") {
      renderTreeExplorer(filtered);
    }
  }

  /* =========================================================
     RENDER: CARDS GRID VIEW
  ========================================================= */
  function renderCards(models) {
    const container = document.getElementById("reposGridMode");
    if (!container) return;

    if (!models.length) {
      container.innerHTML = `
        <div style="grid-column: 1/-1; padding: 40px; text-align: center; color: var(--text-muted); background: var(--bg-card); border-radius: var(--radius-md);">
          No AI-Git model repositories match your filter criteria.
        </div>`;
      return;
    }

    container.innerHTML = models.map(m => {
      const savingsPct = m.metrics?.savingsPercentage || 0;
      const compression = m.metrics?.compressionRatio || "3.4x";
      const rawGB = (m.metrics?.rawSizeBytes / (1024 ** 3)).toFixed(1);
      const casGB = (m.metrics?.casSizeBytes / (1024 ** 3)).toFixed(1);

      return `
        <div class="model-hub-card">
          <div class="model-card-header">
            <div>
              <a href="repository.html?id=${encodeURIComponent(m.id)}" class="model-card-title">${m.name}</a>
              <div style="font-size: 12px; color: var(--text-muted); margin-top: 2px;">
                ${m.family} &middot; <span style="color: var(--accent-purple);">${m.author}</span>
              </div>
            </div>
            <span class="pill-badge badge-blue">${m.framework}</span>
          </div>

          <p class="model-card-desc">${m.description}</p>

          <div class="model-specs-strip">
            <div>
              <div class="spec-cell-label">Parameters</div>
              <div class="spec-cell-val" style="color: var(--accent-purple);">${m.parameters || 'N/A'}</div>
            </div>
            <div>
              <div class="spec-cell-label">Checkpoint</div>
              <div class="spec-cell-val">${m.epoch || 'Init'}</div>
            </div>
            <div>
              <div class="spec-cell-label">Val Loss</div>
              <div class="spec-cell-val" style="color: #4ade80;">${m.valLoss || '0.00'}</div>
            </div>
          </div>

          <div class="savings-progress-bar">
            <div class="savings-label-row">
              <span style="color: var(--text-secondary);">FastCDC Storage Deduplication</span>
              <span style="color: var(--accent-pink); font-weight: 700;">${savingsPct}% Saved (${casGB} GB stored of ${rawGB} GB)</span>
            </div>
            <div class="savings-track">
              <div class="savings-bar-fill" style="width: ${Math.max(10, savingsPct)}%;"></div>
            </div>
          </div>

          <div class="model-card-actions">
            <div style="display: flex; gap: 8px;">
              <a href="repository.html?id=${encodeURIComponent(m.id)}" class="btn-primary" style="font-size: 12.5px; padding: 6px 12px;">
                📁 Browse Files & Chunks
              </a>
              <a href="repository.html?id=${encodeURIComponent(m.id)}#graph" class="btn-secondary" style="font-size: 12.5px; padding: 6px 12px;">
                🌿 Visual DAG
              </a>
            </div>
            <div style="font-size: 12.5px; color: var(--text-muted); display: flex; align-items: center; gap: 10px;">
              <span>⭐ ${m.stars || 0}</span>
              <span>🍴 ${m.forks || 0}</span>
            </div>
          </div>
        </div>
      `;
    }).join("");
  }

  /* =========================================================
     RENDER: SPLIT-PANE DIRECTORY TREE EXPLORER
  ========================================================= */
  function renderTreeExplorer(models) {
    const treeList = document.getElementById("modelTreeHierarchy");
    if (!treeList) return;

    if (!models.length) {
      treeList.innerHTML = `<li style="padding: 10px; color: var(--text-muted); font-size: 13px;">No models available.</li>`;
      return;
    }

    treeList.innerHTML = models.map((m, idx) => `
      <li class="tree-node ${idx === 0 ? 'open' : ''}" data-model-id="${m.id}">
        <div class="tree-item is-folder" data-type="repo" data-model-id="${m.id}">
          <span class="chevron">&#9654;</span>
          <span class="tree-icon">
            <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>
          </span>
          <span><strong>${m.name}</strong></span>
          <span class="pill-badge badge-purple" style="margin-left: auto; font-size: 10px;">${m.parameters}</span>
        </div>
        <ul class="tree-sublist">
          ${renderSubtreeHTML(m.tree, m.id, m.name)}
        </ul>
      </li>
    `).join("");

    bindTreeEvents();

    // Default inspect the first model or root folder
    if (models.length > 0 && !selectedNode) {
      inspectDirectory(models[0], models[0].name, models[0].tree);
    }
  }

  function renderSubtreeHTML(items, modelId, currentPath) {
    if (!items || !items.length) return "";

    return items.map(item => {
      const itemPath = `${currentPath} / ${item.name}`;
      if (item.type === "directory") {
        return `
          <li class="tree-node open">
            <div class="tree-item is-folder" data-type="dir" data-model-id="${modelId}" data-path="${itemPath}" data-name="${item.name}">
              <span class="chevron">&#9654;</span>
              <span class="tree-icon">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/></svg>
              </span>
              <span>${item.name}</span>
            </div>
            <ul class="tree-sublist">
              ${renderSubtreeHTML(item.children, modelId, itemPath)}
            </ul>
          </li>
        `;
      } else {
        const isModelWeights = item.name.endsWith(".safetensors") || item.name.endsWith(".bin") || item.name.endsWith(".gguf") || item.name.endsWith(".pt");
        return `
          <li class="tree-node">
            <div class="tree-item" data-type="file" data-model-id="${modelId}" data-path="${itemPath}" data-name="${item.name}">
              <span class="tree-icon" style="color: ${isModelWeights ? 'var(--accent-purple)' : 'var(--text-muted)'};">
                ${isModelWeights ? `
                  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"/></svg>
                ` : `
                  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/></svg>
                `}
              </span>
              <span>${item.name}</span>
              <span style="margin-left: auto; font-size: 11px; color: var(--text-muted);">${item.size}</span>
            </div>
          </li>
        `;
      }
    }).join("");
  }

  function bindTreeEvents() {
    document.querySelectorAll(".tree-item").forEach(el => {
      el.addEventListener("click", (e) => {
        e.stopPropagation();

        document.querySelectorAll(".tree-item").forEach(item => item.classList.remove("selected"));
        el.classList.add("selected");

        const parentNode = el.closest(".tree-node");
        if (el.classList.contains("is-folder") && parentNode) {
          parentNode.classList.toggle("open");
        }

        const type = el.dataset.type;
        const modelId = el.dataset.modelId;
        const model = allModels.find(m => m.id === modelId);
        if (!model) return;

        if (type === "repo") {
          inspectDirectory(model, model.name, model.tree);
        } else if (type === "dir") {
          const dirName = el.dataset.name;
          const dir = findNodeInTree(model.tree, dirName);
          inspectDirectory(model, el.dataset.path, dir?.children || []);
        } else if (type === "file") {
          const fileName = el.dataset.name;
          const file = findNodeInTree(model.tree, fileName);
          inspectFile(model, el.dataset.path, file);
        }
      });
    });
  }

  function findNodeInTree(items, name) {
    if (!items) return null;
    for (const item of items) {
      if (item.name === name) return item;
      if (item.children) {
        const found = findNodeInTree(item.children, name);
        if (found) return found;
      }
    }
    return null;
  }

  /* =========================================================
     INSPECTOR PANE: DIRECTORY OVERVIEW & FILES TABLE
  ========================================================= */
  function inspectDirectory(model, path, items) {
    const pane = document.getElementById("treeInspectorPane");
    if (!pane) return;

    const fileCount = items ? items.length : 0;

    pane.innerHTML = `
      <div class="inspector-breadcrumb">
        <span>📦 AI-GIT Registry</span> &rsaquo; <span>${path}</span>
      </div>

      <div class="inspector-card">
        <div class="inspector-title-row">
          <div class="inspector-title">
            <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="color: var(--accent-purple);"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/></svg>
            <span>${path.split("/").pop().trim()}</span>
          </div>
          <span class="pill-badge badge-blue">${fileCount} Item${fileCount === 1 ? '' : 's'}</span>
        </div>
        <p style="font-size: 13.5px; color: var(--text-secondary); margin-bottom: 16px;">
          Model Repository: <strong>${model.name}</strong> &middot; Tracked via AI-GIT Merkle Tree &middot; FastCDC Deduplication Active
        </p>
        <div style="display: flex; gap: 10px;">
          <a href="repository.html?id=${model.id}" class="btn-primary" style="font-size: 12.5px; padding: 6px 12px;">
            Open Model Repository &rarr;
          </a>
        </div>
      </div>

      <div class="section-title" style="margin-bottom: 14px;">
        <span>Directory Contents & AI Artifacts</span>
      </div>

      <div style="background: var(--bg-surface); border: 1px solid var(--border-default); border-radius: var(--radius-md); overflow: hidden;">
        <table class="dir-table">
          <thead>
            <tr>
              <th>Name</th>
              <th>Type</th>
              <th>Size</th>
              <th>FastCDC Chunks</th>
              <th>Content-Addressed Hash</th>
            </tr>
          </thead>
          <tbody>
            ${items.map(item => `
              <tr style="cursor: pointer;" onclick="document.querySelector('[data-name=\\'${item.name}\\']')?.click()">
                <td>
                  <div style="display: flex; align-items: center; gap: 8px; font-weight: 600; color: ${item.type === 'directory' ? 'var(--accent-purple)' : 'var(--text-primary)'};">
                    ${item.type === 'directory' ? '📁' : '📄'} ${item.name}
                  </div>
                </td>
                <td style="color: var(--text-muted);">${item.type}</td>
                <td style="font-family: var(--font-mono);">${item.size || 'N/A'}</td>
                <td>
                  ${item.chunks ? `<span class="pill-badge badge-pink">${item.chunks} chunks (${item.deduped || 0} shared)</span>` : '<span style="color: var(--text-muted);">&mdash;</span>'}
                </td>
                <td style="font-family: var(--font-mono); font-size: 11.5px; color: var(--accent-blue);">
                  ${item.hash ? item.hash.substring(0, 16) + '...' : '&mdash;'}
                </td>
              </tr>
            `).join("")}
          </tbody>
        </table>
      </div>
    `;
  }

  /* =========================================================
     INSPECTOR PANE: FILE & FASTCDC CHUNK MAP VISUALIZER
  ========================================================= */
  function inspectFile(model, path, file) {
    const pane = document.getElementById("treeInspectorPane");
    if (!pane || !file) return;

    const isModelWeights = file.name.endsWith(".safetensors") || file.name.endsWith(".bin") || file.name.endsWith(".gguf") || file.name.endsWith(".pt");

    // Generate mock visual chunks for this file
    const totalChunks = file.chunks || 8;
    const dedupedChunks = file.deduped || Math.floor(totalChunks * 0.75);

    let chunksHTML = "";
    for (let i = 0; i < Math.min(totalChunks, 24); i++) {
      const isDedup = i < dedupedChunks;
      const pseudoHash = (file.hash || "0742431d").substring(0, 8) + i.toString(16).padStart(4, "0");
      chunksHTML += `
        <div class="chunk-brick ${isDedup ? 'deduped' : 'new-chunk'}" title="Chunk #${i+1} | Hash: ${pseudoHash}... | Size: ~1.05 MB">
          <div style="font-weight: 700; color: ${isDedup ? '#4ade80' : 'var(--accent-purple)'};">
            ${isDedup ? '🟢 Deduped' : '🟣 Modified'}
          </div>
          <div style="color: var(--text-muted); margin-top: 3px;">#${i+1} &middot; 1.0 MB</div>
          <div style="font-size: 10px; color: var(--accent-blue); overflow: hidden; text-overflow: ellipsis;">${pseudoHash}</div>
        </div>
      `;
    }

    pane.innerHTML = `
      <div class="inspector-breadcrumb">
        <span>📦 AI-GIT Registry</span> &rsaquo; <span>${path}</span>
      </div>

      <div class="inspector-card">
        <div class="inspector-title-row">
          <div class="inspector-title">
            <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="color: var(--accent-blue);"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/></svg>
            <span>${file.name}</span>
          </div>
          <span class="pill-badge badge-pink">${file.size}</span>
        </div>

        <div style="display: flex; gap: 24px; margin-top: 14px; font-size: 13px;">
          <div><span style="color: var(--text-muted);">Repository:</span> <strong>${model.name}</strong></div>
          <div><span style="color: var(--text-muted);">Content-Addressed SHA-256:</span> <code style="color: var(--accent-purple); font-family: var(--font-mono);">${file.hash || 'N/A'}</code></div>
        </div>
      </div>

      ${isModelWeights ? `
        <!-- FastCDC Chunk Map -->
        <div class="inspector-card" style="border-color: rgba(168, 85, 247, 0.35);">
          <div class="inspector-title-row">
            <div class="section-title" style="font-size: 15px;">
              <span>⚡ FastCDC Content-Defined Chunk Map</span>
            </div>
            <span class="pill-badge badge-green">${dedupedChunks} of ${totalChunks} Chunks Deduplicated (${((dedupedChunks/totalChunks)*100).toFixed(0)}% Zero-Disk)</span>
          </div>
          <p style="font-size: 12.5px; color: var(--text-secondary); margin-bottom: 14px;">
            AI-GIT split this ${file.size} model checkpoint dynamically into content-addressed chunks using the FastCDC Gear Matrix. Green chunks are reused from earlier training epochs without duplicating disk storage!
          </p>
          <div class="chunk-map-grid">
            ${chunksHTML}
          </div>
        </div>
      ` : `
        <!-- Code / Config Viewer -->
        <div class="section-title" style="margin-bottom: 12px;">File Content Preview</div>
        <div class="code-box">
{
  "model_type": "llama",
  "architectures": ["LlamaForCausalLM"],
  "hidden_size": 4096,
  "intermediate_size": 14336,
  "num_attention_heads": 32,
  "num_hidden_layers": 32,
  "num_key_value_heads": 8,
  "max_position_embeddings": 8192,
  "rms_norm_eps": 1e-05,
  "torch_dtype": "bfloat16",
  "fastcdc_chunk_storage": {
    "enabled": true,
    "chunk_manifest_sha256": "${file.hash || 'c7dd71e5dcee2bf5e76314f9efdf9f9cc6599a554797baa3f141ab558fb87c24'}"
  }
}
        </div>
      `}
    `;
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();