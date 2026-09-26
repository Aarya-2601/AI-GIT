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

          <!-- 2. ANIMATED CHAMELEON CHARACTER -->
          <div class="chameleon-perch-container" id="chameleonPerch">
            <a href="dashboard.html" class="chameleon-anchor" title="AI-GIT Model Hub (Click home)">
              <img src="chameleon-logo.png" alt="AI-GIT Chameleon" class="topbar-chameleon-character" id="topbarChameleonImg">
            </a>
            <!-- Animated chameleon tongue that extends & snaps periodically -->
            <div class="chameleon-tongue-track" id="chameleonTongueTrack" aria-hidden="true">
              <div class="chameleon-tongue-stalk" id="chameleonTongueStalk">
                <span class="chameleon-tongue-bulb"></span>
              </div>
            </div>
          </div>

          <!-- 3. TOPBAR HEADING: MODEL HUB (Glowing Blue, Big Heading, Same for all users) -->
          <div class="topbar-repo-breadcrumb">
            <a href="dashboard.html" class="topbar-heading-modelhub" id="topbarHeadingModelHub" title="AI-GIT Model Hub">Model Hub</a>
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

          <!-- 4. TRIANGLE OF NEURAL NETWORKS (LINK TO ALL REPOS) BESIDE PFP -->
          <div class="btn-neural-triangle-wrap" style="position: relative;">
            <a href="repositories.html" class="topbar-nav-icon-btn btn-neural-triangle-pfp" id="btnNeuralTriangleRepos" title="All Nodes &amp; Repositories (View All Repos)" aria-label="All Nodes">
              <svg class="neural-triangle-svg" width="18" height="18" viewBox="0 0 20 20" fill="none" xmlns="http://www.w3.org/2000/svg">
                <polygon points="10,3.5 3.5,16 16.5,16" stroke="currentColor" stroke-width="1.6" stroke-linejoin="round"/>
                <line x1="10" y1="3.5" x2="10" y2="10.5" stroke="#38bdf8" stroke-width="1" stroke-dasharray="1.5,1.5"/>
                <line x1="3.5" y1="16" x2="10" y2="10.5" stroke="#38bdf8" stroke-width="1" stroke-dasharray="1.5,1.5"/>
                <line x1="16.5" y1="16" x2="10" y2="10.5" stroke="#38bdf8" stroke-width="1" stroke-dasharray="1.5,1.5"/>
                <circle cx="10" cy="10.5" r="1.4" fill="#ffffff"/>
                <circle cx="10" cy="3.5" r="2.2" fill="#c084fc"/>
                <circle cx="3.5" cy="16" r="2.2" fill="#38bdf8"/>
                <circle cx="16.5" cy="16" r="2.2" fill="#f472b6"/>
              </svg>
            </a>

            <!-- ALL USER REPOSITORIES DROPDOWN (Right-aligned next to PFP) -->
            <div class="user-repos-dropdown" id="userReposDropdown" style="right: 0; left: auto;">
              <div class="repos-drop-header">
                <span>All Repositories &amp; Nodes</span>
                <a href="repositories.html" style="font-size: 11px; color: #38bdf8; text-decoration: none; font-weight: 600;">View All →</a>
              </div>
              <input type="text" class="repos-filter-input" id="repoFilterInput" placeholder="Filter repositories..." autocomplete="off">
              <div class="repos-drop-list" id="reposDropList">
                <a href="repository.html?repo=AI-GIT" class="repo-drop-item">
                  <span style="display: flex; align-items: center; gap: 8px;">
                    <span>🧠</span>
                    <strong>AI-GIT</strong>
                  </span>
                  <span class="repo-drop-meta">Public Node</span>
                </a>
                <a href="repository.html?repo=llama-3-8b-instruct" class="repo-drop-item">
                  <span style="display: flex; align-items: center; gap: 8px;">
                    <span>📦</span>
                    <span>llama-3-8b-instruct</span>
                  </span>
                  <span class="repo-drop-meta">8B · FastCDC</span>
                </a>
                <a href="repository.html?repo=mistral-7b-v0.3" class="repo-drop-item">
                  <span style="display: flex; align-items: center; gap: 8px;">
                    <span>📦</span>
                    <span>mistral-7b-v0.3</span>
                  </span>
                  <span class="repo-drop-meta">7.3B Safetensors</span>
                </a>
                <a href="repository.html?repo=stable-diffusion-3-medium" class="repo-drop-item">
                  <span style="display: flex; align-items: center; gap: 8px;">
                    <span>🎨</span>
                    <span>stable-diffusion-3-medium</span>
                  </span>
                  <span class="repo-drop-meta">2B Diffusion</span>
                </a>
                <a href="repository.html?repo=phi-3-mini-4k" class="repo-drop-item">
                  <span style="display: flex; align-items: center; gap: 8px;">
                    <span>⚡</span>
                    <span>phi-3-mini-4k</span>
                  </span>
                  <span class="repo-drop-meta">3.8B Checkpoint</span>
                </a>
                <a href="repository.html?repo=deepseek-coder-v2" class="repo-drop-item">
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

          <!-- 5. RIGHTMOST: USER PROFILE PICTURE / AVATAR -->
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
    if (typeof initChameleonWorld === "function") {
      initChameleonWorld();
    } else {
      const s = document.createElement("script");
      s.src = "chameleon_world.js";
      document.body.appendChild(s);
    }
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

    // 2. Neural Triangle Button beside PFP (link to all repos + optional dropdown)
    const btnNeuralTriangle = document.getElementById("btnNeuralTriangleRepos");
    const userReposDropdown = document.getElementById("userReposDropdown");
    const repoFilterInput = document.getElementById("repoFilterInput");
    if (btnNeuralTriangle && userReposDropdown) {
      // Toggle dropdown on hover or right-click, direct navigation on primary click
      const wrap = btnNeuralTriangle.closest(".btn-neural-triangle-wrap");
      if (wrap) {
        wrap.addEventListener("mouseenter", () => {
          userReposDropdown.classList.add("show");
          btnNeuralTriangle.classList.add("open");
        });
        wrap.addEventListener("mouseleave", () => {
          userReposDropdown.classList.remove("show");
          btnNeuralTriangle.classList.remove("open");
        });
      }

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

    // 4. User Profile Picture click -> Toggle MySpace & Contract Feed
    const userToggle = document.getElementById("userMenuToggle");
    const userDropdown = document.getElementById("userMenuDropdown");
    const modelhubShell = document.getElementById("modelhubShell");
    const btnCloseMySpace = document.getElementById("btnCloseMySpace");

    function renderMySpaceHeatGrid() {
      const grid = document.getElementById("gitCalendarGrid");
      if (!grid || grid.children.length > 0) return;

      // 48 columns (weeks) x 7 rows (days) = 336 cells
      // Replicate image: Sep-May mostly empty, sparse commits in Feb-May,
      // and heavy green activity concentrated in Jul, Aug, Sep
      const totalWeeks = 48;
      for (let day = 0; day < 7; day++) {
        for (let week = 0; week < totalWeeks; week++) {
          const cell = document.createElement('div');
          let lvl = 'lvl-0';

          // Dense summer/autumn activity (weeks 36 to 48 -> Jul, Aug, Sep)
          if (week >= 36) {
            const r = Math.random();
            if (r > 0.70) lvl = 'lvl-4';
            else if (r > 0.45) lvl = 'lvl-3';
            else if (r > 0.25) lvl = 'lvl-2';
            else lvl = 'lvl-1';
          } 
          // Late spring activity (weeks 28 to 35 -> May, Jun)
          else if (week >= 28) {
            const r = Math.random();
            if (r > 0.75) lvl = 'lvl-3';
            else if (r > 0.55) lvl = 'lvl-2';
            else if (r > 0.35) lvl = 'lvl-1';
            else lvl = 'lvl-0';
          }
          // Sparse spring commits (weeks 20 to 27 -> Feb, Mar, Apr)
          else if (week >= 20) {
            const r = Math.random();
            if (r > 0.85) lvl = 'lvl-2';
            else if (r > 0.70) lvl = 'lvl-1';
            else lvl = 'lvl-0';
          }
          // Earlier autumn/winter (Sep - Jan) mostly empty with rare dots
          else {
            if (Math.random() > 0.94) lvl = 'lvl-1';
            else lvl = 'lvl-0';
          }

          cell.className = 'cal-cell ' + lvl;
          grid.appendChild(cell);
        }
      }
    }

    if (userToggle) {
      userToggle.addEventListener("click", (e) => {
        e.stopPropagation();
        if (modelhubShell) {
          modelhubShell.classList.toggle("myspace-open");
          const isOpen = modelhubShell.classList.contains("myspace-open");
          localStorage.setItem("aigit_myspace_open", isOpen ? "true" : "false");
          if (isOpen) renderMySpaceHeatGrid();
        } else {
          // If on a page other than dashboard, navigate to dashboard with ?myspace=1
          window.location.href = "dashboard.html?myspace=1";
        }
      });
    }

    if (btnCloseMySpace && modelhubShell) {
      btnCloseMySpace.addEventListener("click", () => {
        modelhubShell.classList.remove("myspace-open");
        localStorage.setItem("aigit_myspace_open", "false");
      });
    }

    // Auto-open MySpace if navigated with ?myspace=1 or saved in session
    if (modelhubShell) {
      const urlParams = new URLSearchParams(window.location.search);
      if (urlParams.get("myspace") === "1" || localStorage.getItem("aigit_myspace_open") === "true") {
        modelhubShell.classList.add("myspace-open");
        renderMySpaceHeatGrid();
      }
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
