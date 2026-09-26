/* =========================================================
   AI-GIT MODEL ISSUES & CHECKPOINT BUG TRACKER JAVASCRIPT
========================================================= */

(function () {
  let allIssues = [];
  let allModels = [];
  let currentStatus = "open";
  let currentRepo = "all";
  let currentLabel = "all";
  let searchQuery = "";
  let currentDetailIssue = null;

  async function init() {
    setupEventListeners();
    await Promise.all([loadModels(), loadIssues()]);
  }

  function setupEventListeners() {
    // Status tab buttons (Open / Closed / All)
    const statusBtns = document.querySelectorAll(".status-tab-btn");
    statusBtns.forEach(btn => {
      btn.addEventListener("click", () => {
        statusBtns.forEach(b => b.classList.remove("active"));
        btn.classList.add("active");
        currentStatus = btn.dataset.status;
        renderIssues();
      });
    });

    // Repo filter select
    const repoSelect = document.getElementById("filterRepoSelect");
    if (repoSelect) {
      repoSelect.addEventListener("change", (e) => {
        currentRepo = e.target.value;
        renderIssues();
      });
    }

    // Search input
    const searchInput = document.getElementById("issuesSearchInput");
    if (searchInput) {
      searchInput.addEventListener("input", (e) => {
        searchQuery = e.target.value.toLowerCase().trim();
        renderIssues();
      });
    }

    // Label pills
    const labelPills = document.querySelectorAll(".label-pill-btn");
    labelPills.forEach(pill => {
      pill.addEventListener("click", () => {
        labelPills.forEach(p => p.classList.remove("active"));
        pill.classList.add("active");
        currentLabel = pill.dataset.label;
        renderIssues();
      });
    });

    // Create Issue Modal Open / Close
    const btnOpenCreate = document.getElementById("btnOpenNewIssueModal");
    const createModal = document.getElementById("createIssueModal");
    const btnCloseCreate = document.getElementById("btnCloseCreateIssueModal");
    const btnCancelCreate = document.getElementById("btnCancelCreateIssue");
    const createForm = document.getElementById("createIssueForm");

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
    const detailModal = document.getElementById("issueDetailModal");
    const btnCloseDetail = document.getElementById("btnCloseDetailModal");
    if (btnCloseDetail && detailModal) {
      btnCloseDetail.addEventListener("click", () => {
        detailModal.style.display = "none";
      });
    }

    // Create Issue Form Submission
    if (createForm) {
      createForm.addEventListener("submit", async (e) => {
        e.preventDefault();
        const repoId = document.getElementById("issueTargetRepo").value;
        const title = document.getElementById("issueTitleInput").value.trim();
        const rawLabels = document.getElementById("issueLabelsInput").value;
        const description = document.getElementById("issueDescInput").value.trim();

        if (!title || !repoId) return;

        const labels = rawLabels.split(",").map(l => l.trim()).filter(Boolean);

        try {
          const res = await fetch("/api/issues", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ repoId, title, labels, description })
          });

          if (res.ok) {
            closeCreateModal();
            createForm.reset();
            await loadIssues();
          } else {
            const err = await res.json();
            alert("Error creating issue: " + (err.error || "Unknown error"));
          }
        } catch (err) {
          console.error("Failed to post issue:", err);
        }
      });
    }

    // Comment Submission
    const btnSubmitComment = document.getElementById("btnSubmitComment");
    const commentInput = document.getElementById("newCommentInput");
    if (btnSubmitComment && commentInput) {
      btnSubmitComment.addEventListener("click", async () => {
        const text = commentInput.value.trim();
        if (!text || !currentDetailIssue) return;

        try {
          const res = await fetch(`/api/issues/${encodeURIComponent(currentDetailIssue.id)}/comments`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ text })
          });

          if (res.ok) {
            const updatedIssue = await res.json();
            currentDetailIssue = updatedIssue;
            // Update in local array
            const idx = allIssues.findIndex(i => i.id === updatedIssue.id);
            if (idx !== -1) allIssues[idx] = updatedIssue;

            commentInput.value = "";
            renderDetailModal(updatedIssue);
            renderIssues();
          }
        } catch (err) {
          console.error("Failed to post comment:", err);
        }
      });
    }
  }

  /* =========================================================
     LOAD DATA
  ========================================================= */
  async function loadModels() {
    try {
      const res = await fetch("/api/models");
      if (res.ok) {
        allModels = await res.json();
        populateModelDropdowns();
      }
    } catch (e) {
      console.error("Failed to load models for issues:", e);
    }
  }

  function populateModelDropdowns() {
    const filterSelect = document.getElementById("filterRepoSelect");
    const formSelect = document.getElementById("issueTargetRepo");

    if (filterSelect) {
      filterSelect.innerHTML = `<option value="all">All AI-GIT Models (${allModels.length})</option>` +
        allModels.map(m => `<option value="${m.id}">${m.name} (${m.parameters})</option>`).join("");
    }

    if (formSelect) {
      formSelect.innerHTML = allModels.map(m => `<option value="${m.id}">${m.name} [${m.framework}]</option>`).join("");
    }
  }

  async function loadIssues() {
    try {
      const res = await fetch("/api/issues");
      if (res.ok) {
        allIssues = await res.json();
        updateCounts();
        renderIssues();
      }
    } catch (e) {
      console.error("Failed to load issues:", e);
    }
  }

  function updateCounts() {
    const openCount = allIssues.filter(i => i.status === "open").length;
    const closedCount = allIssues.filter(i => i.status === "closed").length;

    const openBadge = document.getElementById("openCountBadge");
    const closedBadge = document.getElementById("closedCountBadge");

    if (openBadge) openBadge.textContent = `${openCount} Open`;
    if (closedBadge) closedBadge.textContent = `${closedCount} Closed`;
  }

  /* =========================================================
     RENDER ISSUES LIST
  ========================================================= */
  function renderIssues() {
    const container = document.getElementById("issuesListContainer");
    if (!container) return;

    let filtered = allIssues;

    // Status filter
    if (currentStatus !== "all") {
      filtered = filtered.filter(i => i.status === currentStatus);
    }

    // Repo filter
    if (currentRepo !== "all") {
      filtered = filtered.filter(i => i.repoId === currentRepo);
    }

    // Label filter
    if (currentLabel !== "all") {
      filtered = filtered.filter(i => i.labels && i.labels.some(l => l.toLowerCase() === currentLabel.toLowerCase()));
    }

    // Search query
    if (searchQuery) {
      filtered = filtered.filter(i =>
        i.title.toLowerCase().includes(searchQuery) ||
        i.author.toLowerCase().includes(searchQuery) ||
        i.repoId.toLowerCase().includes(searchQuery) ||
        (i.description && i.description.toLowerCase().includes(searchQuery))
      );
    }

    if (filtered.length === 0) {
      container.innerHTML = `
        <div class="issues-empty-state">
          <svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" style="color: var(--text-muted); margin-bottom: 12px;"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>
          <div style="font-size: 16px; font-weight: 600; color: var(--text-primary); margin-bottom: 6px;">No issues match your filter criteria</div>
          <div style="font-size: 13px; color: var(--text-muted);">Try resetting search filters or report a new model checkpoint bug.</div>
        </div>
      `;
      return;
    }

    container.innerHTML = filtered.map(issue => {
      const isOpen = issue.status === "open";
      const iconSvg = isOpen
        ? `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#22c55e" stroke-width="2"><circle cx="12" cy="12" r="10"/><circle cx="12" cy="12" r="3"/></svg>`
        : `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#a855f7" stroke-width="2"><polyline points="20 6 9 17 4 12"/></svg>`;

      const labelsHtml = (issue.labels || []).map(label => {
        let badgeClass = "badge-blue";
        if (label.includes("divergence") || label.includes("overflow") || label.includes("fp16")) badgeClass = "badge-pink";
        else if (label.includes("fastcdc") || label.includes("manifest")) badgeClass = "badge-purple";
        return `<span class="pill-badge ${badgeClass}" style="font-size: 10.5px;">${label}</span>`;
      }).join(" ");

      return `
        <div class="issue-item-row" onclick="window.viewIssueDetail('${issue.id}')">
          <div class="issue-status-icon">${iconSvg}</div>
          <div class="issue-main-content">
            <div class="issue-title-line">
              <span class="issue-title-text">${issue.title}</span>
              <a href="repository.html?id=${encodeURIComponent(issue.repoId)}" class="pill-badge badge-blue" style="font-size: 11px;" onclick="event.stopPropagation()">
                ${issue.repoId}
              </a>
              ${labelsHtml}
            </div>
            <div class="issue-meta-line">
              <span>#${issue.id}</span>
              <span>&middot;</span>
              <span>opened ${issue.createdAt} by <strong style="color: var(--text-secondary);">${issue.author}</strong></span>
            </div>
          </div>
          <div class="issue-comments-badge">
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"/></svg>
            <span>${issue.commentsCount || 0}</span>
          </div>
        </div>
      `;
    }).join("");
  }

  /* =========================================================
     VIEW ISSUE DETAILS & TIMELINE
  ========================================================= */
  window.viewIssueDetail = function (id) {
    const issue = allIssues.find(i => i.id === id);
    if (!issue) return;

    currentDetailIssue = issue;
    renderDetailModal(issue);

    const modal = document.getElementById("issueDetailModal");
    if (modal) modal.style.display = "flex";
  };

  function renderDetailModal(issue) {
    const statusEl = document.getElementById("detailIssueStatus");
    const idEl = document.getElementById("detailIssueId");
    const titleEl = document.getElementById("detailIssueTitle");
    const metaEl = document.getElementById("detailIssueMeta");
    const labelsEl = document.getElementById("detailIssueLabels");
    const descEl = document.getElementById("detailIssueDesc");
    const timelineEl = document.getElementById("detailCommentsTimeline");

    const isOpen = issue.status === "open";
    if (statusEl) {
      statusEl.textContent = isOpen ? "Open" : "Closed";
      statusEl.className = `pill-badge ${isOpen ? 'badge-pink' : 'badge-purple'}`;
    }
    if (idEl) idEl.textContent = issue.id;
    if (titleEl) titleEl.textContent = issue.title;
    if (metaEl) {
      metaEl.innerHTML = `Model: <a href="repository.html?id=${encodeURIComponent(issue.repoId)}" style="color: var(--accent-blue); font-weight: 600;">${issue.repoId}</a> &middot; Reported by <strong>${issue.author}</strong> &middot; ${issue.createdAt}`;
    }

    if (labelsEl) {
      labelsEl.innerHTML = (issue.labels || []).map(l =>
        `<span class="pill-badge badge-purple" style="font-size: 11px;">${l}</span>`
      ).join(" ");
    }

    if (descEl) descEl.textContent = issue.description || "No description provided.";

    if (timelineEl) {
      const comments = issue.comments || [];
      if (comments.length === 0) {
        timelineEl.innerHTML = `
          <div style="color: var(--text-muted); font-size: 13px; text-align: center; padding: 14px; background: var(--bg-surface); border-radius: var(--radius-sm);">
            No researcher comments yet. Be the first to diagnose this checkpoint bug!
          </div>
        `;
      } else {
        timelineEl.innerHTML = comments.map(c => `
          <div class="comment-bubble">
            <div class="comment-bubble-header">
              <div style="display: flex; align-items: center; gap: 8px;">
                <span class="user-avatar-sm" style="width: 22px; height: 22px; font-size: 10px; background: var(--accent-blue); color: #000; border-radius: 50%; display: inline-flex; align-items: center; justify-content: center; font-weight: 700;">
                  ${(c.author || 'AI')[0].toUpperCase()}
                </span>
                <strong style="color: var(--text-primary);">${c.author}</strong>
              </div>
              <span>${c.time}</span>
            </div>
            <div class="comment-bubble-body">${c.text}</div>
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
