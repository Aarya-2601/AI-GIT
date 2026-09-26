/* =========================================================
   AI-GIT DASHBOARD CLIENT JAVASCRIPT
========================================================= */

(function () {
  async function loadDashboardData() {
    try {
      // 1. Fetch system-wide metrics
      const metricsRes = await fetch("/api/metrics");
      if (metricsRes.ok) {
        const m = await metricsRes.json();
        renderMetrics(m);
      }

      // 2. Fetch AI models
      const modelsRes = await fetch("/api/models");
      if (modelsRes.ok) {
        const models = await modelsRes.json();
        renderModelRepositories(models);
        renderRecentActivity(models);
      }
    } catch (e) {
      console.error("Error loading dashboard data:", e);
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

    // Comparison bar update
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

    if (!models.length) {
      container.innerHTML = `<div style="padding: 30px; text-align: center; color: var(--text-muted); background: var(--bg-card); border-radius: var(--radius-md);">No model repositories tracked yet. Click "+ New Model" to create one.</div>`;
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
            <span class="meta-item" style="margin-left: auto;">Updated ${m.updatedAt}</span>
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

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", loadDashboardData);
  } else {
    loadDashboardData();
  }
})();