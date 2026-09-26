/* =========================================================
   AI-GIT UNIVERSAL NAVIGATION JAVASCRIPT
   Exact layout:
   - Top-left: Expandable circle showing 'Navbar' on hover
   - To its right: Chameleon logo + Username 'Aarya-2601' + '/' + current repo name glowing in purple
   - Beside it: Triangle of neural networks (opens user's all repos on click)
   - Center: Global search input (Type / to search)
   - Extreme Right (from left to right in order):
       1. Divergences (Issues) - SVG of straight line + curve diverging
       2. Convergences (Pull Requests) - SVG of straight line + curve converging
       3. Pings (Notifications) - SVG of telephone
       4. User profile picture / avatar
   Shared uniformly between Model Hub (dashboard) and Node (repository) pages.
========================================================= */

(function () {
  async function initNavigation() {
    const navPlaceholder = document.getElementById("aigitTopNav");
    if (!navPlaceholder) return;

    let user = {
      name: "Aarya Doshi",
      username: "Aarya-2601",
      role: "Lead AI Researcher",
      avatar: "https://images.unsplash.com/photo-1534528741775-53994a69daeb?w=150&auto=format&fit=crop&q=80"
    };

    try {
      const res = await fetch("/api/auth/me");
      if (res.ok) {
        const data = await res.json();
        if (data.user) {
          user = { ...user, ...data.user };
          if (data.user.username) user.username = data.user.username;
        }
      }
    } catch (e) {
      console.warn("Using local session user fallback:", e);
    }

    const currentPath = window.location.pathname.toLowerCase();
    const isDashboard = currentPath.includes("dashboard") || currentPath === "/";
    const isRepos = currentPath.includes("repositor");
    const isIssues = currentPath.includes("issues");
    const isDiscussions = currentPath.includes("discussions");

    const urlParams = new URLSearchParams(window.location.search);
    const activeRepoName = urlParams.get("repo") || urlParams.get("id") || "AI-GIT";
    const avatarInitial = (user.name || user.username || "A").charAt(0).toUpperCase();

    navPlaceholder.innerHTML = `
      <header class="aigit-topbar">
        <div class="topbar-left" style="display: flex; align-items: center; gap: 8px;">
          
          <!-- 1. TOP-LEFT EXPANDABLE CIRCULAR NAVBAR BUTTON -->
          <div class="node-nav-circle-wrapper" id="nodeNavCircleWrapper">
            <button type="button" class="node-nav-circle-btn" id="nodeNavCircleBtn" aria-label="Expand Navigation Menu" title="AI-GIT Platform Navigation">
              <div class="node-nav-icon-node">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <circle cx="12" cy="12" r="9" stroke="url(#nodeGlowGrad)" stroke-width="2"/>
                  <circle cx="12" cy="12" r="3" fill="#c084fc"/>
                  <line x1="12" y1="3" x2="12" y2="9" stroke="#38bdf8" stroke-width="1.5"/>
                  <line x1="12" y1="15" x2="12" y2="21" stroke="#38bdf8" stroke-width="1.5"/>
                  <defs>
                    <linearGradient id="nodeGlowGrad" x1="0%" y1="0%" x2="100%" y2="100%">
                      <stop offset="0%" stop-color="#38bdf8"/>
                      <stop offset="50%" stop-color="#c084fc"/>
                      <stop offset="100%" stop-color="#f472b6"/>
                    </linearGradient>
                  </defs>
                </svg>
              </div>
              <span class="node-nav-text">Navbar</span>
            </button>

            <!-- DROPDOWN MENU -->
            <div class="node-nav-dropdown" id="nodeNavDropdown">
              <div class="node-nav-menu-header">CORE NODES &amp; PAGES</div>

              <!-- Model Hub (Home) -->
              <a href="dashboard.html" class="node-menu-item ${isDashboard ? 'active' : ''}">
                <span class="node-circle-icon circle-emerald">
                  <span class="node-inner-dot"></span>
                </span>
                <div class="node-item-info">
                  <span class="node-item-title">Model Hub</span>
                  <span class="node-item-desc">Home overview &amp; workspace</span>
                </div>
              </a>

              <!-- All Divergences (Issues) -->
              <a href="issues.html" class="node-menu-item ${isIssues ? 'active' : ''}">
                <span class="node-circle-icon circle-amber">
                  <span class="node-inner-dot"></span>
                </span>
                <div class="node-item-info">
                  <span class="node-item-title">All Divergences</span>
                  <span class="node-item-desc">Issues, bug reports &amp; diffs</span>
                </div>
              </a>

              <!-- All Convergences (Pull Requests) -->
              <a href="issues.html#convergences" class="node-menu-item">
                <span class="node-circle-icon circle-purple">
                  <span class="node-inner-dot"></span>
                </span>
                <div class="node-item-info">
                  <span class="node-item-title">All Convergences</span>
                  <span class="node-item-desc">Pull requests &amp; weight merges</span>
                </div>
              </a>

              <!-- All Nodes (Repositories) -->
              <a href="repositories.html" class="node-menu-item ${isRepos ? 'active' : ''}">
                <span class="node-circle-icon circle-blue">
                  <span class="node-inner-dot"></span>
                </span>
                <div class="node-item-info">
                  <span class="node-item-title">All Nodes</span>
                  <span class="node-item-desc">Public &amp; private models</span>
                </div>
              </a>

              <!-- Exchanges or Chats (Discussions) -->
              <a href="discussions.html" class="node-menu-item ${isDiscussions ? 'active' : ''}">
                <span class="node-circle-icon circle-pink">
                  <span class="node-inner-dot"></span>
                </span>
                <div class="node-item-info">
                  <span class="node-item-title">Exchanges &amp; Chats</span>
                  <span class="node-item-desc">Community discussions</span>
                </div>
              </a>

              <div class="node-dropdown-divider"></div>

              <!-- TREE OF TOP REPOSITORIES -->
              <div class="node-tree-header">
                <span>TOP NODES</span>
              </div>

              <div class="dropdown-tree-list">
                <a href="repository.html?repo=llama-3-8b-instruct" class="dropdown-tree-node">
                  <span class="tree-line">├──►</span>
                  <span class="tree-repo-name">llama-3-8b-instruct</span>
                </a>
                <a href="repository.html?repo=mistral-7b-v0.3" class="dropdown-tree-node">
                  <span class="tree-line">├──►</span>
                  <span class="tree-repo-name">mistral-7b-v0.3</span>
                </a>
                <a href="repository.html?repo=stable-diffusion-3-medium" class="dropdown-tree-node">
                  <span class="tree-line">├──►</span>
                  <span class="tree-repo-name">stable-diffusion-3-medium</span>
                </a>
                <a href="repository.html?repo=phi-3-mini-4k" class="dropdown-tree-node">
                  <span class="tree-line">├──►</span>
                  <span class="tree-repo-name">phi-3-mini-4k</span>
                </a>
                <a href="repository.html?repo=deepseek-coder-v2" class="dropdown-tree-node">
                  <span class="tree-line">└──►</span>
                  <span class="tree-repo-name">deepseek-coder-v2</span>
                </a>
              </div>
            </div>
          </div>

          <!-- 2. CHAMELEON LOGO -->
          <a href="dashboard.html" title="AI-GIT Model Hub">
            <img src="chameleon-logo.png" alt="AI-GIT Chameleon" class="topbar-chameleon-logo">
          </a>

          <!-- 3. REPO BREADCRUMB: USERNAME / REPO NAME (GLOWING PURPLE) + NEURAL TRIANGLE -->
          <div class="topbar-repo-breadcrumb">
            <a href="profile.html" class="repo-owner-name">${user.username}</a>
            <span class="repo-breadcrumb-slash">/</span>
            <a href="repository.html?repo=${encodeURIComponent(activeRepoName)}" class="repo-current-name glowing-purple" id="topbarCurrentRepoName">${activeRepoName}</a>

            <!-- 4. TRIANGLE OF NEURAL NETWORKS (Click to see all user repos) -->
            <div class="btn-neural-triangle-wrap">
              <button type="button" class="btn-neural-triangle" id="btnNeuralTriangle" title="View all repositories of ${user.username}">
                <!-- Triangle with 3 connected neural network nodes -->
                <svg class="neural-triangle-svg" width="16" height="16" viewBox="0 0 20 20" fill="none" xmlns="http://www.w3.org/2000/svg">
                  <!-- Outer triangle connections -->
                  <polygon points="10,3.5 3.5,16 16.5,16" stroke="currentColor" stroke-width="1.6" stroke-linejoin="round"/>
                  <!-- Synapse to center node -->
                  <line x1="10" y1="3.5" x2="10" y2="10.5" stroke="#38bdf8" stroke-width="1" stroke-dasharray="1.5,1.5"/>
                  <line x1="3.5" y1="16" x2="10" y2="10.5" stroke="#38bdf8" stroke-width="1" stroke-dasharray="1.5,1.5"/>
                  <line x1="16.5" y1="16" x2="10" y2="10.5" stroke="#38bdf8" stroke-width="1" stroke-dasharray="1.5,1.5"/>
                  <!-- Center synapse vertex -->
                  <circle cx="10" cy="10.5" r="1.4" fill="#ffffff"/>
                  <!-- Three neural network vertices -->
                  <circle cx="10" cy="3.5" r="2.2" fill="#c084fc"/>
                  <circle cx="3.5" cy="16" r="2.2" fill="#38bdf8"/>
                  <circle cx="16.5" cy="16" r="2.2" fill="#f472b6"/>
                </svg>
                <!-- Mini Caret -->
                <svg width="8" height="8" viewBox="0 0 24 24" fill="currentColor">
                  <polygon points="6,9 18,9 12,16"/>
                </svg>
              </button>

              <!-- ALL USER REPOSITORIES DROPDOWN -->
              <div class="user-repos-dropdown" id="userReposDropdown">
                <div class="repos-drop-header">
                  <span>${user.username}'s Nodes</span>
                  <span style="font-size: 10px; color: #c084fc;">8 Repositories</span>
                </div>
                <input type="text" class="repos-filter-input" id="repoFilterInput" placeholder="Find a repository..." autocomplete="off">
                <div class="repos-drop-list" id="reposDropList">
                  <a href="repository.html?repo=AI-GIT" class="repo-drop-item ${activeRepoName === 'AI-GIT' ? 'current' : ''}">
                    <span style="display: flex; align-items: center; gap: 8px;">
                      <span>🧠</span>
                      <strong>AI-GIT</strong>
                    </span>
                    <span class="repo-drop-meta">Public Node</span>
                  </a>
                  <a href="repository.html?repo=llama-3-8b-instruct" class="repo-drop-item ${activeRepoName === 'llama-3-8b-instruct' ? 'current' : ''}">
                    <span style="display: flex; align-items: center; gap: 8px;">
                      <span>📦</span>
                      <span>llama-3-8b-instruct</span>
                    </span>
                    <span class="repo-drop-meta">8B · FastCDC</span>
                  </a>
                  <a href="repository.html?repo=mistral-7b-v0.3" class="repo-drop-item ${activeRepoName === 'mistral-7b-v0.3' ? 'current' : ''}">
                    <span style="display: flex; align-items: center; gap: 8px;">
                      <span>📦</span>
                      <span>mistral-7b-v0.3</span>
                    </span>
                    <span class="repo-drop-meta">7.3B Safetensors</span>
                  </a>
                  <a href="repository.html?repo=stable-diffusion-3-medium" class="repo-drop-item ${activeRepoName === 'stable-diffusion-3-medium' ? 'current' : ''}">
                    <span style="display: flex; align-items: center; gap: 8px;">
                      <span>🎨</span>
                      <span>stable-diffusion-3-medium</span>
                    </span>
                    <span class="repo-drop-meta">2B Diffusion</span>
                  </a>
                  <a href="repository.html?repo=phi-3-mini-4k" class="repo-drop-item ${activeRepoName === 'phi-3-mini-4k' ? 'current' : ''}">
                    <span style="display: flex; align-items: center; gap: 8px;">
                      <span>⚡</span>
                      <span>phi-3-mini-4k</span>
                    </span>
                    <span class="repo-drop-meta">3.8B Checkpoint</span>
                  </a>
                  <a href="repository.html?repo=deepseek-coder-v2" class="repo-drop-item ${activeRepoName === 'deepseek-coder-v2' ? 'current' : ''}">
                    <span style="display: flex; align-items: center; gap: 8px;">
                      <span>💻</span>
                      <span>deepseek-coder-v2</span>
                    </span>
                    <span class="repo-drop-meta">16B MoE</span>
                  </a>
                  <a href="repository.html?repo=whisper-large-v3" class="repo-drop-item">
                    <span style="display: flex; align-items: center; gap: 8px;">
                      <span>🎙️</span>
                      <span>whisper-large-v3</span>
                    </span>
                    <span class="repo-drop-meta">Audio Encoder</span>
                  </a>
                  <a href="repository.html?repo=gemma-2-9b" class="repo-drop-item">
                    <span style="display: flex; align-items: center; gap: 8px;">
                      <span>💎</span>
                      <span>gemma-2-9b</span>
                    </span>
                    <span class="repo-drop-meta">Google DeepMind</span>
                  </a>
                </div>
                <div class="repos-drop-footer">
                  <a href="repositories.html">View all 14 nodes in Model Hub →</a>
                </div>
              </div>
            </div>

          </div>

        </div>

        <!-- CENTER: SEARCH BAR (IMAGE REFERENCE: Type / to search) -->
        <div class="topbar-center">
          <div class="search-container">
            <svg class="search-icon-left" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/></svg>
            <input type="text" id="aigitGlobalSearch" class="search-input" placeholder="Type / to search" autocomplete="off">
            <span class="search-kbd">/</span>
          </div>
        </div>

        <!-- EXTREME RIGHT GROUP (Order from right to left as requested):
             Rightmost: User profile picture / avatar
             To its left: Telephone SVG for 'Pings' (Notifications)
             To its left: Convergences (Pull Requests) - Straight line + curve converging
             To its left: Divergences (Issues) - Straight line + curve diverging
        -->
        <div class="topbar-right topbar-right-group">

          <!-- 1. DIVERGENCES (ISSUES): Straight line and curve diverging from it -->
          <a href="issues.html" class="topbar-nav-icon-btn btn-divergences" title="All Divergences (Issues)">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <!-- Straight line -->
              <line x1="6" y1="3" x2="6" y2="21"/>
              <!-- Curve diverging from straight line -->
              <path d="M6 17 C6 11, 18 13, 18 6"/>
              <!-- Nodes -->
              <circle cx="6" cy="18" r="2.5" fill="currentColor"/>
              <circle cx="18" cy="6" r="2.5" fill="currentColor"/>
              <circle cx="6" cy="5" r="2" fill="currentColor"/>
            </svg>
            <span class="nav-icon-badge badge-amber">2</span>
          </a>

          <!-- 2. CONVERGENCES (PULL REQUESTS): Straight line and curve converging into it -->
          <a href="issues.html#convergences" class="topbar-nav-icon-btn btn-convergences" title="All Convergences (Pull Requests)">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <!-- Straight line on right -->
              <line x1="18" y1="3" x2="18" y2="21"/>
              <!-- Curve converging into straight line -->
              <path d="M6 6 C6 13, 18 11, 18 18"/>
              <!-- Nodes -->
              <circle cx="6" cy="6" r="2.5" fill="currentColor"/>
              <circle cx="18" cy="18" r="2.5" fill="currentColor"/>
              <circle cx="18" cy="5" r="2" fill="currentColor"/>
            </svg>
            <span class="nav-icon-badge badge-purple">1</span>
          </a>

          <!-- 3. PINGS (NOTIFICATIONS): Telephone SVG -->
          <div style="position: relative;">
            <button type="button" class="topbar-nav-icon-btn btn-pings" id="btnPingsToggle" title="Pings (Notifications)">
              <svg width="17" height="17" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <!-- Telephone handset -->
                <path d="M22 16.92v3a2 2 0 0 1-2.18 2 19.79 19.79 0 0 1-8.63-3.07 19.5 19.5 0 0 1-6-6 19.79 19.79 0 0 1-3.07-8.67A2 2 0 0 1 4.11 2h3a2 2 0 0 1 2 1.72 12.84 12.84 0 0 0 .7 2.81 2 2 0 0 1-.45 2.11L8.09 9.91a16 16 0 0 0 6 6l1.27-1.27a2 2 0 0 1 2.11-.45 12.84 12.84 0 0 0 2.81.7A2 2 0 0 1 22 16.92z"/>
                <!-- Notification Ping soundwaves -->
                <path d="M14.5 2a9 9 0 0 1 7.5 7.5"/>
                <path d="M14.5 5.5a5 5 0 0 1 4 4"/>
              </svg>
              <span class="nav-icon-badge badge-blue">3</span>
            </button>

            <!-- PINGS FLYOUT DROPDOWN -->
            <div class="pings-flyout-dropdown" id="pingsFlyoutDropdown">
              <div class="pings-header">
                <span>📞 PINGS &amp; NOTIFICATIONS</span>
                <span style="font-size: 11px; color: #8b949e;">3 unread</span>
              </div>
              <div class="pings-list">
                <div class="ping-item">
                  <span class="ping-icon-phone">📞</span>
                  <div>
                    <div class="ping-text"><strong>sanvinaik</strong> pushed commit <code>ebb6a18</code> to <strong>AI-GIT:main</strong></div>
                    <div class="ping-time">19 hours ago</div>
                  </div>
                </div>
                <div class="ping-item">
                  <span class="ping-icon-phone">⚡</span>
                  <div>
                    <div class="ping-text"><strong>FastCDC Deduplication</strong>: 78.4% storage reduction on <strong>llama-3-8b-instruct</strong></div>
                    <div class="ping-time">1 day ago</div>
                  </div>
                </div>
                <div class="ping-item">
                  <span class="ping-icon-phone">🔀</span>
                  <div>
                    <div class="ping-text"><strong>keya-ai</strong> opened Divergence #4: <em>"LoRA rank mismatch in attention weights"</em></div>
                    <div class="ping-time">2 days ago</div>
                  </div>
                </div>
              </div>
            </div>
          </div>

          <!-- 4. RIGHTMOST: USER PROFILE PICTURE / AVATAR -->
          <div style="position: relative;">
            <button type="button" class="user-profile-btn" id="userMenuToggle" title="Account: ${user.name}">
              ${user.avatar ? `<img src="${user.avatar}" alt="${user.name}" class="user-profile-img" onerror="this.style.display='none'">` : ''}
              <div class="user-profile-avatar-fallback" ${user.avatar ? 'style="display:none;"' : ''}>${avatarInitial}</div>
            </button>

            <!-- USER ACCOUNT DROPDOWN -->
            <div class="dropdown-menu" id="userMenuDropdown" style="right: 0; left: auto;">
              <div class="dropdown-header">
                Signed in as <strong>${user.name}</strong><br>
                <span style="font-size: 11px; color: var(--accent-purple);">@${user.username} · ${user.role || 'AI Researcher'}</span>
              </div>
              <a href="dashboard.html" class="dropdown-item">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="7" height="7"/><rect x="14" y="3" width="7" height="7"/><rect x="14" y="14" width="7" height="7"/><rect x="3" y="14" width="7" height="7"/></svg>
                <span>Model Hub (Dashboard)</span>
              </a>
              <a href="profile.html" class="dropdown-item">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>
                <span>Profile Studio</span>
              </a>
              <a href="repositories.html" class="dropdown-item">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>
                <span>All Nodes</span>
              </a>
              <div class="dropdown-divider"></div>
              <button type="button" class="dropdown-item" id="btnLogout" style="color: var(--accent-pink); width: 100%; border: none; background: transparent; cursor: pointer;">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"/><polyline points="16 17 21 12 16 7"/><line x1="21" y1="12" x2="9" y2="12"/></svg>
                <span>Log out</span>
              </button>
            </div>
          </div>

        </div>
      </header>
    `;

    setupEventListeners();
  }

  function setupEventListeners() {
    // 1. Top-left Circular Navbar Button click toggle
    const navBtn = document.getElementById("nodeNavCircleBtn");
    const navDropdown = document.getElementById("nodeNavDropdown");
    if (navBtn && navDropdown) {
      navBtn.addEventListener("click", (e) => {
        e.stopPropagation();
        navBtn.classList.toggle("open");
        navDropdown.classList.toggle("show");
      });
    }

    // 2. Neural Triangle Button (all user repos)
    const btnNeuralTriangle = document.getElementById("btnNeuralTriangle");
    const userReposDropdown = document.getElementById("userReposDropdown");
    const repoFilterInput = document.getElementById("repoFilterInput");
    if (btnNeuralTriangle && userReposDropdown) {
      btnNeuralTriangle.addEventListener("click", (e) => {
        e.stopPropagation();
        btnNeuralTriangle.classList.toggle("open");
        userReposDropdown.classList.toggle("show");
        if (userReposDropdown.classList.contains("show")) {
          setTimeout(() => repoFilterInput?.focus(), 100);
        }
      });

      // Filter repos inside dropdown
      if (repoFilterInput) {
        repoFilterInput.addEventListener("input", (e) => {
          const q = e.target.value.toLowerCase();
          const items = document.querySelectorAll(".repo-drop-item");
          items.forEach(it => {
            const txt = it.textContent.toLowerCase();
            it.style.display = txt.includes(q) ? "flex" : "none";
          });
        });
      }
    }

    // 3. Pings (Telephone) dropdown
    const btnPingsToggle = document.getElementById("btnPingsToggle");
    const pingsFlyoutDropdown = document.getElementById("pingsFlyoutDropdown");
    if (btnPingsToggle && pingsFlyoutDropdown) {
      btnPingsToggle.addEventListener("click", (e) => {
        e.stopPropagation();
        pingsFlyoutDropdown.classList.toggle("show");
      });
    }

    // 4. User profile menu toggle
    const userToggle = document.getElementById("userMenuToggle");
    const userDropdown = document.getElementById("userMenuDropdown");
    if (userToggle && userDropdown) {
      userToggle.addEventListener("click", (e) => {
        e.stopPropagation();
        userDropdown.classList.toggle("show");
      });
    }

    // Close any dropdown when clicking outside
    document.addEventListener("click", (e) => {
      if (userDropdown && !userDropdown.contains(e.target) && !userToggle?.contains(e.target)) {
        userDropdown.classList.remove("show");
      }
      if (navDropdown && !navDropdown.contains(e.target) && !navBtn?.contains(e.target)) {
        navDropdown.classList.remove("show");
        if (navBtn) navBtn.classList.remove("open");
      }
      if (userReposDropdown && !userReposDropdown.contains(e.target) && !btnNeuralTriangle?.contains(e.target)) {
        userReposDropdown.classList.remove("show");
        if (btnNeuralTriangle) btnNeuralTriangle.classList.remove("open");
      }
      if (pingsFlyoutDropdown && !pingsFlyoutDropdown.contains(e.target) && !btnPingsToggle?.contains(e.target)) {
        pingsFlyoutDropdown.classList.remove("show");
      }
    });

    // Global search shortcut '/'
    const searchInput = document.getElementById("aigitGlobalSearch");
    document.addEventListener("keydown", (e) => {
      if (e.key === "/" && document.activeElement !== searchInput && !['INPUT', 'TEXTAREA'].includes(document.activeElement.tagName)) {
        e.preventDefault();
        searchInput?.focus();
      }
    });

    // Logout
    const btnLogout = document.getElementById("btnLogout");
    if (btnLogout) {
      btnLogout.addEventListener("click", async () => {
        try {
          await fetch("/api/auth/logout", { method: "POST" });
        } catch (e) {
          console.error(e);
        }
        localStorage.removeItem("aigit_user");
        window.location.href = "login.html";
      });
    }
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", initNavigation);
  } else {
    initNavigation();
  }
})();
