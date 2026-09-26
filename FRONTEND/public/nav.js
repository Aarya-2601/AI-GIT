/* =========================================================
   AI-GIT UNIVERSAL NAVIGATION JAVASCRIPT
========================================================= */

(function () {
  async function initNavigation() {
    const navPlaceholder = document.getElementById("aigitTopNav");
    if (!navPlaceholder) return;

    // Fetch active session user
    let user = {
      name: "Aarya Doshi",
      username: "aarya-ml",
      role: "Lead AI Researcher",
      avatar: "https://images.unsplash.com/photo-1534528741775-53994a69daeb?w=150&auto=format&fit=crop&q=80"
    };

    try {
      const res = await fetch("/api/auth/me");
      if (res.ok) {
        const data = await res.json();
        if (data.user) user = { ...user, ...data.user };
      }
    } catch (e) {
      console.warn("Could not fetch user session; using local default:", e);
    }

    const currentPath = window.location.pathname.toLowerCase();
    const isDashboard = currentPath.includes("dashboard") || currentPath === "/";
    const isRepos = currentPath.includes("repositor");
    const isIssues = currentPath.includes("issues");
    const isDiscussions = currentPath.includes("discussions");
    const isMetrics = currentPath.includes("metrics");
    const isProfile = currentPath.includes("profile");

    const avatarInitial = (user.name || user.username || "A").charAt(0).toUpperCase();

    navPlaceholder.innerHTML = `
      <header class="aigit-topbar">
        <div class="topbar-left">
          <a href="dashboard.html" class="brand-link" title="AI-GIT Dashboard">
            <div class="brand-icon">
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">
                <path d="M12 2a4 4 0 0 1 4 4c0 1.1-.5 2.1-1.3 2.8L17 12h3a2 2 0 0 1 2 2v2a2 2 0 0 1-2 2h-3l-2.3 3.2c.8.7 1.3 1.7 1.3 2.8a4 4 0 1 1-8 0c0-1.1.5-2.1 1.3-2.8L7 18H4a2 2 0 0 1-2-2v-2a2 2 0 0 1 2-2h3l2.3-3.2C8.5 8.1 8 7.1 8 6a4 4 0 0 1 4-4z"/>
                <circle cx="12" cy="6" r="1.5" fill="currentColor"/>
                <circle cx="12" cy="18" r="1.5" fill="currentColor"/>
              </svg>
            </div>
            <span>AI-GIT</span>
            <span class="brand-badge">VCS for AI/ML</span>
          </a>

          <nav class="nav-links" aria-label="Main Navigation">
            <a href="dashboard.html" class="${isDashboard ? 'active' : ''}">
              <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="7" height="7"/><rect x="14" y="3" width="7" height="7"/><rect x="14" y="14" width="7" height="7"/><rect x="3" y="14" width="7" height="7"/></svg>
              <span>Dashboard</span>
            </a>
            <a href="repositories.html" class="${isRepos ? 'active' : ''}">
              <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>
              <span>Model Repos</span>
            </a>
            <a href="issues.html" class="${isIssues ? 'active' : ''}">
              <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><circle cx="12" cy="12" r="1"/><line x1="12" y1="8" x2="12" y2="12"/></svg>
              <span>Issues</span>
            </a>
            <a href="discussions.html" class="${isDiscussions ? 'active' : ''}">
              <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"/></svg>
              <span>Chats & Discussions</span>
            </a>
            <a href="profile.html" class="${isProfile ? 'active' : ''}">
              <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>
              <span>Profile Studio</span>
            </a>
          </nav>
        </div>

        <div class="topbar-center">
          <div class="search-container">
            <svg class="search-icon-left" width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/></svg>
            <input type="text" id="aigitGlobalSearch" class="search-input" placeholder="Search AI models, chunks, weights, issues... (Press /)" autocomplete="off">
            <span class="search-kbd">/</span>
          </div>
        </div>

        <div class="topbar-right">
          <button type="button" class="btn-create" id="btnOpenNewRepoModal">
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/></svg>
            <span>New Model</span>
          </button>

          <div style="position: relative;">
            <div class="user-chip" id="userMenuToggle" title="Account: ${user.name}">
              <div class="user-avatar">
                ${user.avatar ? `<img src="${user.avatar}" alt="${user.name}" onerror="this.style.display='none'">` : ''}
                <span>${avatarInitial}</span>
              </div>
              <span class="user-name">${user.username}</span>
              <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="6 9 12 15 18 9"/></svg>
            </div>

            <div class="dropdown-menu" id="userMenuDropdown">
              <div class="dropdown-header">
                Signed in as <strong>${user.name}</strong><br>
                <span style="font-size: 11px; color: var(--accent-purple);">${user.role || 'AI Researcher'}</span>
              </div>
              <a href="profile.html" class="dropdown-item">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>
                <span>Customize Profile Studio</span>
              </a>
              <a href="repositories.html" class="dropdown-item">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>
                <span>Your Model Repositories</span>
              </a>
              <a href="issues.html" class="dropdown-item">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><circle cx="12" cy="12" r="1"/><line x1="12" y1="8" x2="12" y2="12"/></svg>
                <span>Assigned Issues</span>
              </a>
              <div class="dropdown-divider"></div>
              <button type="button" class="dropdown-item" id="btnLogout" style="color: var(--accent-pink);">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"/><polyline points="16 17 21 12 16 7"/><line x1="21" y1="12" x2="9" y2="12"/></svg>
                <span>Sign Out</span>
              </button>
            </div>
          </div>
        </div>
      </header>

      <!-- New Repository Modal -->
      <div class="modal-overlay" id="newRepoModal">
        <div class="modal-card">
          <div class="modal-header">
            <h3 class="modal-title">
              <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="color: var(--accent-purple);"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>
              <span>Create New AI Model Repository</span>
            </h3>
            <button class="modal-close" id="btnCloseNewRepoModal">&times;</button>
          </div>
          <form id="newRepoForm">
            <div class="form-group">
              <label class="form-label">Repository Name (e.g. llama-3-8b, sdxl-lightning)</label>
              <input type="text" class="form-input" id="newRepoName" required placeholder="my-awesome-foundation-model">
            </div>
            <div class="form-group">
              <label class="form-label">Model Family</label>
              <select class="form-select" id="newRepoFamily">
                <option value="Large Language Model">Large Language Model (LLM)</option>
                <option value="Computer Vision / Diffusion">Computer Vision / Diffusion</option>
                <option value="Audio / Speech Recognition">Audio / Speech Recognition</option>
                <option value="Multimodal VLM">Multimodal Vision-Language Model</option>
                <option value="Code Intelligence">Code Intelligence & Reasoning</option>
              </select>
            </div>
            <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 12px;">
              <div class="form-group">
                <label class="form-label">Weight Format</label>
                <select class="form-select" id="newRepoFramework">
                  <option value="SafeTensors">SafeTensors (Recommended)</option>
                  <option value="GGUF">GGUF (Quantized Edge)</option>
                  <option value="PyTorch">PyTorch (.pt / .bin)</option>
                  <option value="ONNX">ONNX Runtime</option>
                </select>
              </div>
              <div class="form-group">
                <label class="form-label">Parameter Count</label>
                <input type="text" class="form-input" id="newRepoParams" placeholder="e.g. 7B, 14B, 70B">
              </div>
            </div>
            <div class="form-group">
              <label class="form-label">Description</label>
              <textarea class="form-textarea" id="newRepoDesc" placeholder="Describe the model architecture, training dataset, and evaluation metrics..."></textarea>
            </div>
            <div style="display: flex; justify-content: flex-end; gap: 10px; margin-top: 24px;">
              <button type="button" class="btn-secondary" id="btnCancelNewRepo">Cancel</button>
              <button type="submit" class="btn-primary">Create AI-GIT Repository</button>
            </div>
          </form>
        </div>
      </div>
    `;

    // Dropdown toggles
    const userToggle = document.getElementById("userMenuToggle");
    const userDropdown = document.getElementById("userMenuDropdown");
    if (userToggle && userDropdown) {
      userToggle.addEventListener("click", (e) => {
        e.stopPropagation();
        userDropdown.classList.toggle("show");
      });
      document.addEventListener("click", () => userDropdown.classList.remove("show"));
    }

    // Modal toggles
    const btnOpenNewRepo = document.getElementById("btnOpenNewRepoModal");
    const newRepoModal = document.getElementById("newRepoModal");
    const btnCloseNewRepo = document.getElementById("btnCloseNewRepoModal");
    const btnCancelNewRepo = document.getElementById("btnCancelNewRepo");
    const newRepoForm = document.getElementById("newRepoForm");

    if (btnOpenNewRepo && newRepoModal) {
      btnOpenNewRepo.addEventListener("click", () => newRepoModal.classList.add("show"));
      btnCloseNewRepo?.addEventListener("click", () => newRepoModal.classList.remove("show"));
      btnCancelNewRepo?.addEventListener("click", () => newRepoModal.classList.remove("show"));
      newRepoModal.addEventListener("click", (e) => {
        if (e.target === newRepoModal) newRepoModal.classList.remove("show");
      });

      newRepoForm?.addEventListener("submit", async (e) => {
        e.preventDefault();
        const payload = {
          name: document.getElementById("newRepoName").value.trim(),
          family: document.getElementById("newRepoFamily").value,
          framework: document.getElementById("newRepoFramework").value,
          parameters: document.getElementById("newRepoParams").value.trim() || "1B",
          description: document.getElementById("newRepoDesc").value.trim()
        };

        try {
          const res = await fetch("/api/models", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(payload)
          });
          if (res.ok) {
            const created = await res.json();
            newRepoModal.classList.remove("show");
            window.location.href = `repository.html?id=${created.id}`;
          } else {
            const err = await res.json();
            alert("Error: " + (err.error || "Failed to create model repository"));
          }
        } catch (err) {
          alert("Network error creating repository: " + err.message);
        }
      });
    }

    // Global Search shortcut
    const searchInput = document.getElementById("aigitGlobalSearch");
    if (searchInput) {
      window.addEventListener("keydown", (e) => {
        if (e.key === "/" && document.activeElement !== searchInput && !["INPUT", "TEXTAREA"].includes(document.activeElement.tagName)) {
          e.preventDefault();
          searchInput.focus();
        }
      });

      searchInput.addEventListener("keydown", (e) => {
        if (e.key === "Enter" && searchInput.value.trim()) {
          window.location.href = `repositories.html?q=${encodeURIComponent(searchInput.value.trim())}`;
        }
      });
    }

    // Logout
    const btnLogout = document.getElementById("btnLogout");
    if (btnLogout) {
      btnLogout.addEventListener("click", async () => {
        await fetch("/api/auth/logout", { method: "POST" });
        window.location.href = "index.html";
      });
    }
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", initNavigation);
  } else {
    initNavigation();
  }
})();