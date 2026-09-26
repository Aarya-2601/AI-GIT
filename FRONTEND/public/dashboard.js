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
      initAINewsFeed();
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
        
        btnToggle.classList.toggle("expanded", isShowingAllNodes);
        const textSpan = btnToggle.querySelector(".btn-show-more-text");
        if (textSpan) {
          textSpan.textContent = isShowingAllNodes ? "Collapse nodes" : "Show all nodes";
        }

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
            <!-- Stylized enlarged branch arrow -->
            <svg class="node-branch-svg" width="38" height="46" viewBox="0 0 38 46" fill="none">
              <!-- Vertical tree trunk -->
              <line class="branch-vline" x1="12" y1="0" x2="12" y2="46" stroke="#ffffff" stroke-width="2.4" stroke-linecap="round" />
              <!-- Horizontal branching connector -->
              <line x1="12" y1="23" x2="28" y2="23" stroke="#ffffff" stroke-width="2.4" stroke-linecap="round" />
              <!-- Stylized sharp arrowhead -->
              <polygon points="26,17 37,23 26,29" fill="#ffffff" />
            </svg>
          </div>
          <a href="repository.html?repo=${encodeURIComponent(repoName.split('/')[1] || repoName)}" class="node-repo-link" title="${repoName}">
            <span class="node-repo-text">${repoName}</span>
            <span class="node-repo-caret">➔</span>
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
        vline.setAttribute("y2", "23");
      } else {
        vline.setAttribute("y2", "46");
      }
    });
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", loadDashboardData);
  } else {
    loadDashboardData();
  }
})();



  /* =========================================================
     LIVE AI/ML NEWS & EXPANDING COMPOSER CONTROLLER
  ========================================================= */
  let aiNewsArticles = [];
  let userCommunityPosts = [];
  let activeFilter = 'all';
  let attachedMediaList = []; // Array of { type: 'image' | 'video' | 'link', url: string, name?: string }

  function initAINewsFeed() {
    loadSavedCommunityPosts();
    setupComposerInteractions();
    setupNewsFilterTabs();
    fetchLiveAINews();
  }

  function loadSavedCommunityPosts() {
    try {
      const saved = localStorage.getItem('aigit_community_feed');
      if (saved) {
        userCommunityPosts = JSON.parse(saved);
      } else {
        // Pre-seed with a relevant breakthrough post
        userCommunityPosts = [
          {
            id: 'post_seed_1',
            author: 'Aarya Doshi',
            username: 'Aarya-2601',
            avatar: 'https://images.unsplash.com/photo-1534528741775-53994a69daeb?w=100&auto=format&fit=crop&q=80',
            time: '3 hours ago',
            text: 'Just finished benchmarking FastCDC neural weight deduplication on our 70B parameter MoE checkpoint. Sliced physical disk footprint down by 74.6% with zero loss in FP16 precision. Check out the repository below!',
            images: ['https://images.unsplash.com/photo-1618005182384-a83a8bd57fbe?w=800&auto=format&fit=crop&q=80'],
            videos: [],
            links: ['https://github.com/Aarya-2601/AI-GIT'],
            likes: 38,
            liked: false,
            type: 'community'
          }
        ];
      }
    } catch (e) {
      userCommunityPosts = [];
    }
  }

  function saveCommunityPosts() {
    try {
      localStorage.setItem('aigit_community_feed', JSON.stringify(userCommunityPosts));
    } catch (e) {}
  }

  function setupComposerInteractions() {
    const textarea = document.getElementById('composerTextarea');
    const imageInput = document.getElementById('attachImageFileInput');
    const videoInput = document.getElementById('attachVideoFileInput');
    const btnAttachUrl = document.getElementById('btnAttachMediaUrl');
    const btnTagWeight = document.getElementById('btnTagModelWeight');
    const btnPublish = document.getElementById('btnPublishUpdate');
    const previewContainer = document.getElementById('composerAttachmentsPreview');

    if (!textarea) return;

    // 1. Auto-expanding input textbox
    textarea.addEventListener('input', function () {
      this.style.height = 'auto';
      this.style.height = Math.min(this.scrollHeight, 260) + 'px';
    });

    // 2. Attach Image File
    if (imageInput) {
      imageInput.addEventListener('change', function (e) {
        const file = e.target.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = function (evt) {
          attachedMediaList.push({ type: 'image', url: evt.target.result, name: file.name });
          renderComposerPreviews();
        };
        reader.readAsDataURL(file);
      });
    }

    // 3. Attach Video File
    if (videoInput) {
      videoInput.addEventListener('change', function (e) {
        const file = e.target.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = function (evt) {
          attachedMediaList.push({ type: 'video', url: evt.target.result, name: file.name });
          renderComposerPreviews();
        };
        reader.readAsDataURL(file);
      });
    }

    // 4. Attach Media URL or Paper Link
    if (btnAttachUrl) {
      btnAttachUrl.addEventListener('click', function () {
        const url = prompt('Enter image URL, video link (.mp4/embed), or paper link:');
        if (!url || !url.trim()) return;
        const cleanUrl = url.trim();
        if (cleanUrl.match(/\.(jpeg|jpg|gif|png|webp)/i)) {
          attachedMediaList.push({ type: 'image', url: cleanUrl });
        } else if (cleanUrl.match(/\.(mp4|webm|mov)/i)) {
          attachedMediaList.push({ type: 'video', url: cleanUrl });
        } else {
          attachedMediaList.push({ type: 'link', url: cleanUrl });
        }
        renderComposerPreviews();
      });
    }

    // 5. Tag Model Architecture
    if (btnTagWeight) {
      btnTagWeight.addEventListener('click', function () {
        const modelTag = prompt('Tag Model Checkpoint (e.g. Llama-3-8B, SDXL-Base, Mistral-7B):', 'Llama-3-8B');
        if (!modelTag || !modelTag.trim()) return;
        attachedMediaList.push({ type: 'tag', name: modelTag.trim() });
        renderComposerPreviews();
      });
    }

    // 6. Publish Update Button
    if (btnPublish) {
      btnPublish.addEventListener('click', function () {
        const text = textarea.value.trim();
        if (!text && attachedMediaList.length === 0) {
          alert('Please enter some text or attach media to share your update!');
          return;
        }

        const newPost = {
          id: 'post_' + Date.now(),
          author: 'Aarya Doshi',
          username: 'Aarya-2601',
          avatar: 'https://images.unsplash.com/photo-1534528741775-53994a69daeb?w=100&auto=format&fit=crop&q=80',
          time: 'Just now',
          text: text,
          images: attachedMediaList.filter(m => m.type === 'image').map(m => m.url),
          videos: attachedMediaList.filter(m => m.type === 'video').map(m => m.url),
          links: attachedMediaList.filter(m => m.type === 'link').map(m => m.url),
          tags: attachedMediaList.filter(m => m.type === 'tag').map(m => m.name),
          likes: 0,
          liked: false,
          type: 'community'
        };

        userCommunityPosts.unshift(newPost);
        saveCommunityPosts();

        // Reset composer
        textarea.value = '';
        textarea.style.height = 'auto';
        attachedMediaList = [];
        renderComposerPreviews();

        renderCombinedFeed();
      });
    }
  }

  function renderComposerPreviews() {
    const container = document.getElementById('composerAttachmentsPreview');
    if (!container) return;

    if (attachedMediaList.length === 0) {
      container.style.display = 'none';
      container.innerHTML = '';
      return;
    }

    container.style.display = 'flex';
    container.innerHTML = attachedMediaList.map((item, idx) => {
      if (item.type === 'image') {
        return `<div class="preview-media-item">
          <img src="${item.url}" alt="Attachment preview">
          <button type="button" class="btn-remove-attachment" data-idx="${idx}">&times;</button>
        </div>`;
      } else if (item.type === 'video') {
        return `<div class="preview-media-item">
          <video src="${item.url}" controls muted></video>
          <button type="button" class="btn-remove-attachment" data-idx="${idx}">&times;</button>
        </div>`;
      } else if (item.type === 'tag') {
        return `<div class="preview-tag-chip">
          <span>🧠 ${item.name}</span>
          <button type="button" class="btn-remove-attachment" style="position:static;width:16px;height:16px;" data-idx="${idx}">&times;</button>
        </div>`;
      } else {
        return `<div class="preview-tag-chip">
          <span>🔗 ${item.url}</span>
          <button type="button" class="btn-remove-attachment" style="position:static;width:16px;height:16px;" data-idx="${idx}">&times;</button>
        </div>`;
      }
    }).join('');

    container.querySelectorAll('.btn-remove-attachment').forEach(btn => {
      btn.addEventListener('click', function () {
        const i = parseInt(this.getAttribute('data-idx'), 10);
        attachedMediaList.splice(i, 1);
        renderComposerPreviews();
      });
    });
  }

  function setupNewsFilterTabs() {
    const tabs = document.querySelectorAll('.news-tab-btn[data-filter]');
    tabs.forEach(tab => {
      tab.addEventListener('click', function () {
        tabs.forEach(t => t.classList.remove('active'));
        this.classList.add('active');
        activeFilter = this.getAttribute('data-filter');
        renderCombinedFeed();
      });
    });

    const btnRefresh = document.getElementById('btnRefreshAINews');
    if (btnRefresh) {
      btnRefresh.addEventListener('click', function () {
        this.style.transform = 'rotate(180deg)';
        fetchLiveAINews().finally(() => {
          setTimeout(() => this.style.transform = 'none', 400);
        });
      });
    }
  }

  async function fetchLiveAINews() {
    const loading = document.getElementById('newsFeedLoading');
    if (loading) loading.style.display = 'flex';

    try {
      // 1. Try local backend proxy
      const res = await fetch('/api/news/ai');
      if (res.ok) {
        const data = await res.json();
        if (data.articles && data.articles.length > 0) {
          aiNewsArticles = data.articles;
          renderCombinedFeed();
          return;
        }
      }
    } catch (e) {
      console.warn('Backend news proxy unavailable, trying direct free API...');
    }

    try {
      // 2. Direct fallback to free Dev.to AI API
      const res = await fetch('https://dev.to/api/articles?tag=ai&per_page=8');
      if (res.ok) {
        const items = await res.json();
        aiNewsArticles = items.map(item => ({
          id: 'devto_' + item.id,
          title: item.title,
          description: item.description || 'Latest AI breakthrough and engineering discussion from the developer community.',
          url: item.url,
          cover_image: item.cover_image || item.social_image || 'https://images.unsplash.com/photo-1620712943543-bcc4688e7485?w=600&auto=format&fit=crop&q=80',
          source: 'Dev.to AI',
          author: item.user ? item.user.name : 'AI Researcher',
          author_avatar: item.user ? item.user.profile_image_90 : 'https://images.unsplash.com/photo-1534528741775-53994a69daeb?w=80&auto=format&fit=crop&q=80',
          published_at: item.published_at,
          tags: item.tag_list || ['ai', 'machinelearning'],
          reading_time: item.reading_time_minutes ? `${item.reading_time_minutes} min read` : '4 min read',
          reactions_count: item.positive_reactions_count || 32
        }));
        renderCombinedFeed();
        return;
      }
    } catch (e) {
      console.warn('Direct Dev.to API fallback error:', e);
    }

    renderCombinedFeed();
  }

  function renderCombinedFeed() {
    const container = document.getElementById('aiNewsFeedContainer');
    if (!container) return;

    let items = [];

    if (activeFilter === 'all') {
      // Interleave community posts and articles
      const maxLen = Math.max(userCommunityPosts.length, aiNewsArticles.length);
      for (let i = 0; i < maxLen; i++) {
        if (userCommunityPosts[i]) items.push(userCommunityPosts[i]);
        if (aiNewsArticles[i]) items.push(aiNewsArticles[i]);
      }
    } else if (activeFilter === 'community') {
      items = [...userCommunityPosts];
    } else if (activeFilter === 'articles') {
      items = [...aiNewsArticles];
    }

    if (items.length === 0) {
      container.innerHTML = `<div style="text-align: center; padding: 36px 0; color: #8b949e;">No updates found in this category. Be the first to publish an update above!</div>`;
      return;
    }

    container.innerHTML = items.map(item => {
      const isCommunity = item.type === 'community';

      if (isCommunity) {
        return `
          <article class="feed-item-card community-post" id="${item.id}">
            <div class="feed-card-header">
              <div class="feed-author-meta">
                <img src="${item.avatar}" alt="${item.author}" class="feed-author-avatar">
                <div>
                  <span class="feed-author-name">${item.author}</span>
                  <span class="feed-author-handle">@${item.username}</span>
                </div>
              </div>
              <div style="display:flex;align-items:center;gap:8px;">
                <span class="feed-item-time">${item.time}</span>
                <span class="feed-source-pill community">AI-GIT Community</span>
              </div>
            </div>

            <div class="feed-card-desc" style="color: #f8fafc; font-size: 14.5px; white-space: pre-wrap;">${item.text}</div>

            ${item.images && item.images.length > 0 ? `
              <div class="feed-card-media-banner">
                <img src="${item.images[0]}" alt="Post image attachment">
              </div>
            ` : ''}

            ${item.videos && item.videos.length > 0 ? `
              <div class="feed-card-media-banner">
                <video src="${item.videos[0]}" controls></video>
              </div>
            ` : ''}

            ${item.tags && item.tags.length > 0 ? `
              <div class="feed-card-tags">
                ${item.tags.map(t => `<span class="feed-tag">🧠 ${t}</span>`).join('')}
              </div>
            ` : ''}

            <div class="feed-card-actions-bar">
              <div style="display:flex;gap:12px;">
                <button type="button" class="feed-action-btn ${item.liked ? 'liked' : ''}" onclick="window.__togglePostLike('${item.id}')">
                  <svg width="15" height="15" viewBox="0 0 24 24" fill="${item.liked ? 'currentColor' : 'none'}" stroke="currentColor" stroke-width="2"><path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z"/></svg>
                  <span>${item.likes || 0}</span>
                </button>
                <button type="button" class="feed-action-btn">
                  <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"/></svg>
                  <span>Discuss</span>
                </button>
                <button type="button" class="feed-action-btn">
                  <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="18" cy="5" r="3"/><circle cx="6" cy="12" r="3"/><circle cx="18" cy="19" r="3"/><line x1="8.59" y1="13.51" x2="15.42" y2="17.49"/><line x1="15.41" y1="6.51" x2="8.59" y2="10.49"/></svg>
                  <span>Share</span>
                </button>
              </div>
            </div>
          </article>
        `;
      } else {
        // News Article
        return `
          <article class="feed-item-card news-article" id="${item.id}">
            <div class="feed-card-header">
              <div class="feed-author-meta">
                <img src="${item.author_avatar}" alt="${item.author}" class="feed-author-avatar" onerror="this.src='https://images.unsplash.com/photo-1534528741775-53994a69daeb?w=80&auto=format&fit=crop&q=80'">
                <div>
                  <span class="feed-author-name">${item.author}</span>
                  <span class="feed-item-time" style="margin-left:6px;">· ${item.reading_time || '3 min'}</span>
                </div>
              </div>
              <span class="feed-source-pill">${item.source}</span>
            </div>

            <h3 class="feed-card-title">
              <a href="${item.url}" target="_blank" rel="noopener">${item.title}</a>
            </h3>

            <p class="feed-card-desc">${item.description}</p>

            ${item.cover_image ? `
              <div class="feed-card-media-banner">
                <a href="${item.url}" target="_blank" rel="noopener">
                  <img src="${item.cover_image}" alt="${item.title}" loading="lazy" onerror="this.parentElement.style.display='none'">
                </a>
              </div>
            ` : ''}

            <div class="feed-card-tags">
              ${(item.tags || []).map(t => `<span class="feed-tag">#${t}</span>`).join('')}
            </div>

            <div class="feed-card-actions-bar">
              <div style="display:flex;gap:12px;">
                <button type="button" class="feed-action-btn">
                  <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 9V5a3 3 0 0 0-3-3l-4 9v11h11.28a2 2 0 0 0 2-1.7l1.38-9a2 2 0 0 0-2-2.3zM7 22H4a2 2 0 0 1-2-2v-7a2 2 0 0 1 2-2h3"/></svg>
                  <span>${item.reactions_count || 24}</span>
                </button>
                <button type="button" class="feed-action-btn">
                  <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M19 21l-7-5-7 5V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2z"/></svg>
                  <span>Save</span>
                </button>
              </div>

              <a href="${item.url}" target="_blank" rel="noopener" class="feed-open-link">
                <span>Read Full Article</span>
                <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"/><polyline points="15 3 21 3 21 9"/><line x1="10" y1="14" x2="21" y2="3"/></svg>
              </a>
            </div>
          </article>
        `;
      }
    }).join('');
  }

  window.__togglePostLike = function(postId) {
    const post = userCommunityPosts.find(p => p.id === postId);
    if (!post) return;
    post.liked = !post.liked;
    post.likes = (post.likes || 0) + (post.liked ? 1 : -1);
    saveCommunityPosts();
    renderCombinedFeed();
  };
