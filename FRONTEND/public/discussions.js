/* =========================================================
   AI-GIT DISCUSSIONS & CHAT BOARD JAVASCRIPT
========================================================= */

(function () {
  let allDiscussions = [];
  let currentCategory = "all";
  let searchQuery = "";
  let currentDetailDisc = null;

  async function init() {
    setupEventListeners();
    await loadDiscussions();
  }

  function setupEventListeners() {
    // Category sidebar filter
    const catBtns = document.querySelectorAll(".category-btn");
    catBtns.forEach(btn => {
      btn.addEventListener("click", () => {
        catBtns.forEach(b => b.classList.remove("active"));
        btn.classList.add("active");
        currentCategory = btn.dataset.category;
        renderDiscussions();
      });
    });

    // Search input
    const searchInput = document.getElementById("discSearchInput");
    if (searchInput) {
      searchInput.addEventListener("input", (e) => {
        searchQuery = e.target.value.toLowerCase().trim();
        renderDiscussions();
      });
    }

    // Create Discussion Modal Open / Close
    const btnOpenCreate = document.getElementById("btnOpenNewDiscModal");
    const createModal = document.getElementById("createDiscModal");
    const btnCloseCreate = document.getElementById("btnCloseCreateDiscModal");
    const btnCancelCreate = document.getElementById("btnCancelCreateDisc");
    const createForm = document.getElementById("createDiscForm");

    if (btnOpenCreate && createModal) {
      btnOpenCreate.addEventListener("click", () => {
        createModal.style.display = "flex";
      });
    }

    const closeCreateModal = () => {
      if (createModal) createModal.style.display = "none";
    };

    if (btnCloseCreate) btnCloseCreate.addEventListener("click", closeCreateModal);
    if (btnCancelCreate) btnCancelCreate.addEventListener("click", closeCreateModal);

    // Detail Modal Close
    const detailModal = document.getElementById("discDetailModal");
    const btnCloseDetail = document.getElementById("btnCloseDiscDetailModal");
    if (btnCloseDetail && detailModal) {
      btnCloseDetail.addEventListener("click", () => {
        detailModal.style.display = "none";
      });
    }

    // Modal Upvote Button
    const btnModalUpvote = document.getElementById("btnModalUpvote");
    if (btnModalUpvote) {
      btnModalUpvote.addEventListener("click", async () => {
        if (!currentDetailDisc) return;
        await handleUpvote(currentDetailDisc.id);
        const updated = allDiscussions.find(d => d.id === currentDetailDisc.id);
        if (updated) {
          currentDetailDisc = updated;
          document.getElementById("detailDiscUpvotesCount").textContent = updated.upvotes;
        }
      });
    }

    // Create Discussion Form Submit
    if (createForm) {
      createForm.addEventListener("submit", async (e) => {
        e.preventDefault();
        const category = document.getElementById("discCategorySelect").value;
        const title = document.getElementById("discTitleInput").value.trim();
        const rawTags = document.getElementById("discTagsInput").value;
        const content = document.getElementById("discContentInput").value.trim();

        if (!title || !content) return;

        const tags = rawTags.split(",").map(t => t.trim()).filter(Boolean);

        try {
          const res = await fetch("/api/discussions", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ category, title, tags, content })
          });

          if (res.ok) {
            closeCreateModal();
            createForm.reset();
            await loadDiscussions();
          } else {
            const err = await res.json();
            alert("Error creating discussion: " + (err.error || "Unknown error"));
          }
        } catch (err) {
          console.error("Failed to post discussion:", err);
        }
      });
    }

    // Reply Form Submit
    const btnSubmitReply = document.getElementById("btnSubmitReply");
    const replyInput = document.getElementById("newReplyInput");
    if (btnSubmitReply && replyInput) {
      btnSubmitReply.addEventListener("click", async () => {
        const text = replyInput.value.trim();
        if (!text || !currentDetailDisc) return;

        try {
          const res = await fetch(`/api/discussions/${encodeURIComponent(currentDetailDisc.id)}/reply`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ text })
          });

          if (res.ok) {
            const updatedDisc = await res.json();
            currentDetailDisc = updatedDisc;

            const idx = allDiscussions.findIndex(d => d.id === updatedDisc.id);
            if (idx !== -1) allDiscussions[idx] = updatedDisc;

            replyInput.value = "";
            renderDetailModal(updatedDisc);
            renderDiscussions();
          }
        } catch (err) {
          console.error("Failed to post reply:", err);
        }
      });
    }
  }

  /* =========================================================
     LOAD DATA
  ========================================================= */
  async function loadDiscussions() {
    try {
      const res = await fetch("/api/discussions");
      if (res.ok) {
        allDiscussions = await res.json();
        renderDiscussions();
      }
    } catch (e) {
      console.error("Failed to load discussions:", e);
    }
  }

  /* =========================================================
     RENDER THREADS LIST
  ========================================================= */
  function renderDiscussions() {
    const container = document.getElementById("discussionsListContainer");
    if (!container) return;

    let filtered = allDiscussions;

    if (currentCategory !== "all") {
      filtered = filtered.filter(d => d.category.toLowerCase().includes(currentCategory.toLowerCase()));
    }

    if (searchQuery) {
      filtered = filtered.filter(d =>
        d.title.toLowerCase().includes(searchQuery) ||
        d.content.toLowerCase().includes(searchQuery) ||
        d.author.toLowerCase().includes(searchQuery) ||
        (d.tags && d.tags.some(t => t.toLowerCase().includes(searchQuery)))
      );
    }

    if (filtered.length === 0) {
      container.innerHTML = `
        <div style="padding: 60px 20px; text-align: center; color: var(--text-muted);">
          <div style="font-size: 16px; font-weight: 600; color: var(--text-primary); margin-bottom: 6px;">No discussions found</div>
          <div style="font-size: 13px;">Try selecting another category or start a new thread.</div>
        </div>
      `;
      return;
    }

    container.innerHTML = filtered.map(disc => {
      const tagsHtml = (disc.tags || []).map(t =>
        `<span class="pill-badge badge-blue" style="font-size: 10.5px;">${t}</span>`
      ).join(" ");

      return `
        <div class="thread-item-row" onclick="window.viewDiscDetail('${disc.id}')">
          <!-- Upvote Button -->
          <div class="upvote-box" onclick="event.stopPropagation(); window.handleThreadUpvote('${disc.id}')">
            <span class="upvote-arrow">▲</span>
            <span class="upvote-count" id="upvoteCount_${disc.id}">${disc.upvotes || 0}</span>
          </div>

          <!-- Main Thread Content -->
          <div class="thread-body">
            <div class="thread-title-line">
              <span class="pill-badge badge-purple" style="font-size: 11px;">${disc.category}</span>
              <span class="thread-title-text">${disc.title}</span>
            </div>

            <div class="thread-preview-snippet">${disc.content}</div>

            <div class="thread-meta-line">
              <span>started ${disc.createdAt} by <strong style="color: var(--text-secondary);">${disc.author}</strong></span>
              <span>&middot;</span>
              <div style="display: inline-flex; gap: 6px;">${tagsHtml}</div>
            </div>
          </div>

          <!-- Replies Count Badge -->
          <div class="thread-replies-badge">
            <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"/></svg>
            <span>${disc.repliesCount || 0}</span>
          </div>
        </div>
      `;
    }).join("");
  }

  /* =========================================================
     UPVOTE HANDLER
  ========================================================= */
  async function handleUpvote(id) {
    try {
      const res = await fetch(`/api/discussions/${encodeURIComponent(id)}/upvote`, {
        method: "POST"
      });
      if (res.ok) {
        const data = await res.json();
        const disc = allDiscussions.find(d => d.id === id);
        if (disc) {
          disc.upvotes = data.upvotes;
          const countEl = document.getElementById(`upvoteCount_${id}`);
          if (countEl) countEl.textContent = data.upvotes;
        }
      }
    } catch (e) {
      console.error("Upvote failed:", e);
    }
  }
  window.handleThreadUpvote = handleUpvote;

  /* =========================================================
     VIEW DISCUSSION DETAIL
  ========================================================= */
  window.viewDiscDetail = function (id) {
    const disc = allDiscussions.find(d => d.id === id);
    if (!disc) return;

    currentDetailDisc = disc;
    renderDetailModal(disc);

    const modal = document.getElementById("discDetailModal");
    if (modal) modal.style.display = "flex";
  };

  function renderDetailModal(disc) {
    const categoryEl = document.getElementById("detailDiscCategory");
    const idEl = document.getElementById("detailDiscId");
    const titleEl = document.getElementById("detailDiscTitle");
    const metaEl = document.getElementById("detailDiscMeta");
    const tagsEl = document.getElementById("detailDiscTags");
    const contentEl = document.getElementById("detailDiscContent");
    const upvotesCountEl = document.getElementById("detailDiscUpvotesCount");
    const timelineEl = document.getElementById("detailRepliesTimeline");

    if (categoryEl) categoryEl.textContent = disc.category;
    if (idEl) idEl.textContent = disc.id;
    if (titleEl) titleEl.textContent = disc.title;
    if (metaEl) {
      metaEl.innerHTML = `Started ${disc.createdAt} by <strong style="color: var(--text-primary);">${disc.author}</strong>`;
    }
    if (tagsEl) {
      tagsEl.innerHTML = (disc.tags || []).map(t =>
        `<span class="pill-badge badge-blue" style="font-size: 11px;">${t}</span>`
      ).join(" ");
    }
    if (contentEl) contentEl.textContent = disc.content;
    if (upvotesCountEl) upvotesCountEl.textContent = disc.upvotes || 0;

    if (timelineEl) {
      const replies = disc.replies || [];
      if (replies.length === 0) {
        timelineEl.innerHTML = `
          <div style="color: var(--text-muted); font-size: 13px; text-align: center; padding: 16px; background: var(--bg-surface); border-radius: var(--radius-sm);">
            No replies in this thread yet. Be the first to share your thoughts!
          </div>
        `;
      } else {
        timelineEl.innerHTML = replies.map(r => `
          <div class="reply-card">
            <div class="reply-card-header">
              <div style="display: flex; align-items: center; gap: 8px;">
                <span class="user-avatar-sm" style="width: 22px; height: 22px; font-size: 10px; background: var(--accent-purple); color: #fff; border-radius: 50%; display: inline-flex; align-items: center; justify-content: center; font-weight: 700;">
                  ${(r.author || 'AI')[0].toUpperCase()}
                </span>
                <strong style="color: var(--text-primary);">${r.author}</strong>
              </div>
              <span>${r.time}</span>
            </div>
            <div style="font-size: 13.5px; line-height: 1.6; color: var(--text-primary);">${r.text}</div>
          </div>
        `).join("");
      }
    }
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
