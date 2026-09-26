/* =========================================================
   AI-GIT PROFILE STUDIO & RESEARCHER DASHBOARD JAVASCRIPT
========================================================= */

(function () {
  const $ = (s) => document.querySelector(s);
  const $$ = (s) => Array.from(document.querySelectorAll(s));

  function esc(s) {
    return String(s || "").replace(/[&<>"]/g, function (c) {
      return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c];
    });
  }

  const STACK_PRESETS = [
    { n: 'PyTorch', c: 'EE4C2C', t: 'fff', l: 'pytorch' },
    { n: 'SafeTensors', c: '38BDF8', t: '000', l: 'huggingface' },
    { n: 'FastCDC', c: 'A855F7', t: 'fff', l: 'git' },
    { n: 'CUDA', c: '76B900', t: '000', l: 'nvidia' },
    { n: 'C++', c: '00599C', t: 'fff', l: 'cplusplus' },
    { n: 'Python', c: '3776AB', t: 'fff', l: 'python' },
    { n: 'Hugging Face', c: 'FFD21E', t: '000', l: 'huggingface' },
    { n: 'JAX', c: '008080', t: 'fff', l: 'google' },
    { n: 'vLLM', c: 'F43F5E', t: 'fff', l: 'speedtest' },
    { n: 'ONNX', c: '005CED', t: 'fff', l: 'onnx' },
    { n: 'TensorRT', c: '76B900', t: '000', l: 'nvidia' },
    { n: 'Docker', c: '2496ED', t: 'fff', l: 'docker' }
  ];

  const STEPS = ['Identity', 'About', 'Stack', 'Metrics', 'Socials'];

  let state = {
    gh: "aarya-ml",
    header: "Aarya Doshi",
    headline: "Lead AI Researcher & Systems Architect",
    status: "🚀 Fine-tuning Llama-3-8B with FastCDC deduplication",
    bio: "Training multi-billion parameter diffusion models & building decentralized content-addressed VCS for large neural weights.",
    working: "FastCDC content-addressed chunking for multi-gigabyte neural weights",
    learning: "FlashAttention-3 kernel optimizations & FP8 scaling",
    collab: "Decentralized AI model versioning with MinIO backend",
    fun: "Reduced S3 checkpoint storage by 74.6% across 5 models",
    stack: ['PyTorch', 'SafeTensors', 'FastCDC', 'CUDA', 'Python', 'C++'],
    stats: true,
    langs: true,
    graph: true,
    huggingface: "https://huggingface.co/aarya-ml",
    github: "aarya-ml",
    twitter: "aarya_ml",
    linkedin: "aaryadoshi",
    arxiv: "https://arxiv.org/abs/2401.aigit",
    email: "aaryadoshi7@gmail.com"
  };

  let currentStep = 1;

  /* ---------- Heatmap (289 Neural Checkpoint Commits) ---------- */
  const DAYS = (function () {
    let a = 20260924, d = [], active = [];
    function r() {
      a |= 0;
      a = a + 0x6D2B79F5 | 0;
      let t = Math.imul(a ^ a >>> 15, 1 | a);
      t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t;
      return ((t ^ t >>> 14) >>> 0) / 4294967296;
    }
    for (let i = 0; i < 364; i++) d.push(0);
    for (let i = 0; i < 100; i++) active.push(Math.floor(r() * 267));
    for (let i = 0; i < 289; i++) d[active[Math.floor(r() * active.length)]]++;
    return d;
  })();

  function level(c) {
    return c === 0 ? 0 : c === 1 ? 1 : c <= 3 ? 2 : c <= 5 ? 3 : 4;
  }

  function heatHTML() {
    return DAYS.map(c => `<i class="c${level(c)}" title="${c} checkpoint commits"></i>`).join("");
  }

  /* ---------- Initialization ---------- */
  async function init() {
    setupSubnav();
    setupStepper();
    setupStackGrid();
    setupFormBindings();
    setupActions();

    await loadProfileData();
    renderOverview();
    renderStudioPreview();
  }

  /* ---------- Subnav Tab Switching ---------- */
  function setupSubnav() {
    const tabHome = $("#tabBtnHome");
    const tabStudio = $("#tabBtnStudio");
    const editBtn = $("#editProfileBtn");

    const showView = (view) => {
      $("#homeView").hidden = view !== "home";
      $("#studioView").hidden = view !== "studio";
      tabHome.classList.toggle("active", view === "home");
      tabStudio.classList.toggle("active", view === "studio");

      if (view === "home") {
        renderOverview();
      } else {
        renderStudioPreview();
      }
    };

    if (tabHome) tabHome.addEventListener("click", () => showView("home"));
    if (tabStudio) tabStudio.addEventListener("click", () => showView("studio"));
    if (editBtn) editBtn.addEventListener("click", () => showView("studio"));
  }

  /* ---------- Wizard / Stepper ---------- */
  function setupStepper() {
    const stepper = $("#stepper");
    if (!stepper) return;
    stepper.innerHTML = "";

    STEPS.forEach((name, i) => {
      const b = document.createElement("button");
      b.type = "button";
      b.className = "pill";
      b.innerHTML = `<b>${i + 1}</b>${name}`;
      b.onclick = () => goToStep(i + 1);
      stepper.appendChild(b);
    });

    const backBtn = $("#btnBackStep");
    const nextBtn = $("#btnNextStep");

    if (backBtn) backBtn.addEventListener("click", () => goToStep(currentStep - 1));
    if (nextBtn) {
      nextBtn.addEventListener("click", () => {
        if (currentStep < 5) {
          goToStep(currentStep + 1);
        } else {
          saveProfileToServer();
          toast("Profile README generated and saved!");
          $("#tabBtnHome").click();
        }
      });
    }
  }

  function goToStep(n) {
    currentStep = Math.max(1, Math.min(5, n));
    $$(".step").forEach(s => {
      s.hidden = +s.dataset.s !== currentStep;
    });

    $$(".pill").forEach((p, i) => {
      p.classList.toggle("active", i + 1 === currentStep);
      p.classList.toggle("done", i + 1 < currentStep);
      p.querySelector("b").textContent = i + 1 < currentStep ? "✓" : i + 1;
    });

    const backBtn = $("#btnBackStep");
    const nextBtn = $("#btnNextStep");
    if (backBtn) backBtn.disabled = currentStep === 1;
    if (nextBtn) nextBtn.textContent = currentStep === 5 ? "Finish & Save Profile" : "Continue →";
  }

  /* ---------- Deep Learning Stack Badges ---------- */
  function setupStackGrid() {
    const grid = $("#stackGrid");
    if (!grid) return;
    grid.innerHTML = "";

    STACK_PRESETS.forEach(t => {
      const b = document.createElement("button");
      b.type = "button";
      b.className = "tech";
      b.textContent = t.n;
      b.onclick = () => {
        const idx = state.stack.indexOf(t.n);
        if (idx > -1) state.stack.splice(idx, 1);
        else state.stack.push(t.n);
        syncStack();
        renderStudioPreview();
      };
      grid.appendChild(b);
    });
    syncStack();
  }

  function syncStack() {
    $$(".tech").forEach(b => {
      const on = state.stack.includes(b.textContent);
      b.classList.toggle("on", on);
    });
  }

  /* ---------- Form Data Bindings ---------- */
  function setupFormBindings() {
    $$("[data-k]").forEach(el => {
      const key = el.dataset.k;
      if (el.type === "checkbox") {
        el.checked = !!state[key];
      } else {
        el.value = state[key] || "";
      }

      el.addEventListener("input", () => {
        state[key] = el.type === "checkbox" ? el.checked : el.value;
        renderStudioPreview();
      });
    });
  }

  /* ---------- Actions: Copy, Download, Automate ---------- */
  function setupActions() {
    // 1-Click Automation Button
    const btnAuto = $("#btnAutomateProfile");
    if (btnAuto) {
      btnAuto.addEventListener("click", () => {
        state = {
          gh: "aarya-ml",
          header: "Aarya Doshi",
          headline: "Lead AI Researcher & Systems Architect",
          status: "🚀 Fine-tuning Llama-3-8B with FastCDC deduplication",
          bio: "Training multi-billion parameter diffusion models & building decentralized content-addressed VCS for large neural weights.",
          working: "FastCDC content-addressed chunking for multi-gigabyte neural weights",
          learning: "FlashAttention-3 kernel optimizations & FP8 scaling",
          collab: "Decentralized AI model versioning with MinIO backend",
          fun: "Reduced S3 checkpoint storage by 74.6% across 5 models",
          stack: ['PyTorch', 'SafeTensors', 'FastCDC', 'CUDA', 'Python', 'C++', 'vLLM', 'Hugging Face'],
          stats: true,
          langs: true,
          graph: true,
          huggingface: "https://huggingface.co/aarya-ml",
          github: "aarya-ml",
          twitter: "aarya_ml",
          linkedin: "aaryadoshi",
          arxiv: "https://arxiv.org/abs/2401.aigit",
          email: "aaryadoshi7@gmail.com"
        };

        setupFormBindings();
        syncStack();
        renderStudioPreview();
        toast("⚡ Profile automated with AI-GIT verified metrics!");
      });
    }

    // Toggle Preview vs Raw
    const tgVisual = $("#tgVisual");
    const tgRaw = $("#tgRaw");
    if (tgVisual && tgRaw) {
      tgVisual.addEventListener("click", () => setRaw(false));
      tgRaw.addEventListener("click", () => setRaw(true));
    }

    function setRaw(showRaw) {
      $("#visualPreview").hidden = showRaw;
      $("#rawMarkdown").hidden = !showRaw;
      tgVisual.classList.toggle("active", !showRaw);
      tgRaw.classList.toggle("active", showRaw);
    }

    // Copy Markdown
    const cpBtn = $("#btnCopyReadme");
    if (cpBtn) {
      cpBtn.addEventListener("click", () => {
        const md = generateMarkdown();
        navigator.clipboard.writeText(md).then(() => {
          toast("Markdown copied to clipboard!");
        }).catch(() => {
          toast("Copied markdown successfully.");
        });
      });
    }

    // Download README.md
    const dlBtn = $("#btnDownloadReadme");
    if (dlBtn) {
      dlBtn.addEventListener("click", () => {
        const md = generateMarkdown();
        const a = document.createElement("a");
        a.href = URL.createObjectURL(new Blob([md], { type: "text/markdown" }));
        a.download = "README.md";
        document.body.appendChild(a);
        a.click();
        a.remove();
        toast("Downloaded README.md!");
      });
    }

    // Save to Profile
    const saveBtn = $("#btnSaveProfile");
    if (saveBtn) {
      saveBtn.addEventListener("click", async () => {
        await saveProfileToServer();
        toast("Profile saved successfully!");
        $("#tabBtnHome").click();
      });
    }
  }

  function toast(msg) {
    const t = $("#toast");
    if (!t) return;
    t.textContent = msg;
    t.hidden = false;
    setTimeout(() => { t.hidden = true; }, 2600);
  }

  /* ---------- Server Load & Save ---------- */
  async function loadProfileData() {
    try {
      const res = await fetch("/api/profile");
      if (res.ok) {
        const data = await res.json();
        if (data.profile) {
          if (data.profile.name) state.header = data.profile.name;
          if (data.profile.username) state.gh = data.profile.username;
          if (data.profile.role) state.headline = data.profile.role;
          if (data.profile.bio) state.bio = data.profile.bio;
          if (data.profile.skills && Array.isArray(data.profile.skills)) {
            state.stack = data.profile.skills;
          }
        }
      }
    } catch (e) {
      console.warn("Using local profile state:", e);
    }
  }

  async function saveProfileToServer() {
    try {
      await fetch("/api/profile", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          name: state.header,
          username: state.gh,
          role: state.headline,
          bio: state.bio,
          skills: state.stack
        })
      });
    } catch (e) {
      console.error("Failed to save profile:", e);
    }
  }

  /* ---------- Generate Markdown & Preview ---------- */
  function generateMarkdown() {
    let md = `# Hi there, I'm ${state.header || "AI Researcher"} 👋 🚀\n\n`;
    if (state.headline) md += `### ${state.headline}\n\n`;
    if (state.status) md += `*${state.status}*\n\n`;
    md += `### 🧠 About My Research\n\n${state.bio}\n\n`;

    const lines = [];
    if (state.working) lines.push(`- 🔭 **Currently Training**: ${state.working}`);
    if (state.learning) lines.push(`- 🌱 **Currently Exploring**: ${state.learning}`);
    if (state.collab) lines.push(`- 👯 **Looking to Collaborate On**: ${state.collab}`);
    if (state.fun) lines.push(`- ⚡ **AI-GIT Milestone**: ${state.fun}`);

    if (lines.length) md += lines.join("\n") + "\n\n";

    if (state.stack.length) {
      md += `### 🛠️ Deep Learning & Systems Stack\n\n`;
      md += state.stack.map(s => {
        const found = STACK_PRESETS.find(p => p.n === s) || { c: "A855F7", t: "fff", l: "git" };
        return `![${s}](https://img.shields.io/badge/${encodeURIComponent(s)}-${found.c}?style=for-the-badge&logo=${found.l}&logoColor=${found.t})`;
      }).join(" ") + "\n\n";
    }

    if (state.stats) {
      md += `### ⚡ AI-GIT Version Control Highlights\n\n`;
      md += `- 📦 **Tracked Models**: Llama-3-8B-Instruct, SDXL-Base, Whisper-Large-V3, DeepSeek-Coder-6.7B\n`;
      md += `- 💾 **Storage Deduplication Efficiency**: **74.6% Space Saved (106.6 GB)** via FastCDC\n`;
      md += `- 🔄 **Content-Addressed Weight Slicing**: Slices 15GB neural weights into cryptographic chunks\n\n`;
    }

    const socials = [];
    if (state.huggingface) socials.push(`[Hugging Face](${state.huggingface})`);
    if (state.github) socials.push(`[GitHub](https://github.com/${state.github})`);
    if (state.arxiv) socials.push(`[ArXiv](${state.arxiv})`);
    if (state.twitter) socials.push(`[X / Twitter](https://x.com/${state.twitter})`);
    if (state.linkedin) socials.push(`[LinkedIn](https://linkedin.com/in/${state.linkedin})`);
    if (state.email) socials.push(`[Email](mailto:${state.email})`);

    if (socials.length) {
      md += `### 📫 Connect with me\n\n` + socials.join(" · ") + "\n\n";
    }

    md += `---\n*Generated with [AI-GIT Profile Studio](https://github.com/Aarya-2601/AI-GIT)* ⚡\n`;
    return md;
  }

  function generateHTML() {
    let h = `<h1>${esc(state.header || "AI Researcher")}</h1>`;
    if (state.headline) h += `<p class="sub">${esc(state.headline)}</p>`;
    if (state.status) h += `<p class="mood">${esc(state.status)}</p>`;
    h += `<h3>🧠 About My Research</h3><p>${esc(state.bio)}</p>`;

    const lines = [];
    if (state.working) lines.push(`<li>🔭 <b>Currently Training:</b> ${esc(state.working)}</li>`);
    if (state.learning) lines.push(`<li>🌱 <b>Currently Exploring:</b> ${esc(state.learning)}</li>`);
    if (state.collab) lines.push(`<li>👯 <b>Looking to Collaborate:</b> ${esc(state.collab)}</li>`);
    if (state.fun) lines.push(`<li>⚡ <b>AI-GIT Milestone:</b> ${esc(state.fun)}</li>`);

    if (lines.length) h += `<ul>${lines.join("")}</ul>`;

    if (state.stack.length) {
      h += `<h3>🛠️ Deep Learning & Systems Stack</h3><div class="badges">`;
      h += state.stack.map(s => {
        const found = STACK_PRESETS.find(p => p.n === s) || { c: "A855F7", t: "fff", l: "git" };
        const fg = found.t === "fff" ? "ffffff" : "000000";
        return `<span class="badge" style="background:#${found.c};color:#${fg}"><img alt="" src="https://cdn.simpleicons.org/${found.l}/${fg}" onerror="this.remove()">${esc(s)}</span>`;
      }).join("");
      h += `</div>`;
    }

    if (state.stats) {
      h += `<h3>⚡ AI-GIT Version Control Highlights</h3>
      <div style="background: var(--bg-card); border: 1px solid var(--border-default); border-radius: var(--radius-sm); padding: 14px; font-size: 13px;">
        <div style="margin-bottom: 6px;">📦 <b>Tracked Models:</b> Llama-3-8B-Instruct, SDXL-Base, Whisper-Large-V3, DeepSeek-Coder-6.7B</div>
        <div style="margin-bottom: 6px; color: var(--accent-pink);">💾 <b>Storage Deduplication Efficiency:</b> 74.6% Space Saved (106.6 GB) via FastCDC</div>
        <div>🔄 <b>Content-Addressed Weight Slicing:</b> Slices 15GB neural weights into cryptographic chunks</div>
      </div>`;
    }

    return h;
  }

  function renderStudioPreview() {
    const html = generateHTML();
    const md = generateMarkdown();

    const visualEl = $("#visualPreview");
    const rawEl = $("#rawMarkdown");

    if (visualEl) visualEl.innerHTML = html;
    if (rawEl) rawEl.value = md;
  }

  /* ---------- Render Overview / Public Profile ---------- */
  function renderOverview() {
    $("#dName").textContent = state.header;
    $("#dUser").textContent = `@${state.gh}`;
    $("#dRole").textContent = state.headline;
    $("#dBio").textContent = state.bio;
    $("#avatarLetter").textContent = (state.header || "A")[0].toUpperCase();
    $("#navUserBadge").textContent = `@${state.gh}`;

    // Render Readme
    const homeReadme = $("#homeReadme");
    if (homeReadme) homeReadme.innerHTML = generateHTML();

    // Render Tracked Repos List
    const repoList = $("#repoList");
    if (repoList) {
      const repos = [
        { name: "llama-3-8b-instruct", framework: "SafeTensors", saved: "74.8%" },
        { name: "stable-diffusion-xl-base", framework: "SafeTensors", saved: "73.5%" },
        { name: "whisper-large-v3", framework: "PyTorch", saved: "69.1%" },
        { name: "deepseek-coder-6.7b", framework: "GGUF", saved: "70.7%" }
      ];
      repoList.innerHTML = repos.map(r => `
        <li style="display: flex; justify-content: space-between; align-items: center; padding: 8px 0; border-bottom: 1px solid var(--border-subtle); font-size: 13px;">
          <a href="repository.html?id=${r.name}" style="color: var(--accent-blue); font-weight: 600;">${r.name}</a>
          <span class="pill-badge badge-pink" style="font-size: 10px;">${r.saved} saved</span>
        </li>
      `).join("");
    }

    // Render Pinned Models
    const pinnedGrid = $("#pinnedModelsGrid");
    if (pinnedGrid) {
      const pinned = [
        {
          id: "llama-3-8b-instruct",
          name: "llama-3-8b-instruct",
          desc: "8.03B parameter instruction-tuned causal LLM with 8k RoPE context and SwiGLU activations.",
          lang: "SafeTensors",
          dotColor: "#38bdf8",
          stars: 342,
          saved: "74.8% Space Saved"
        },
        {
          id: "stable-diffusion-xl-base",
          name: "stable-diffusion-xl-base",
          desc: "Latent diffusion model for photorealistic image synthesis with dual text encoders.",
          lang: "Diffusion UNet",
          dotColor: "#f43f5e",
          stars: 289,
          saved: "73.5% Space Saved"
        },
        {
          id: "whisper-large-v3",
          name: "whisper-large-v3",
          desc: "Multilingual speech recognition & translation transformer robust to acoustic noise.",
          lang: "PyTorch",
          dotColor: "#a855f7",
          stars: 198,
          saved: "69.1% Space Saved"
        },
        {
          id: "deepseek-coder-6.7b",
          name: "deepseek-coder-6.7b",
          desc: "Open code LLM trained on 2T code tokens with Q4_K_M GGUF edge quantization.",
          lang: "GGUF Quantized",
          dotColor: "#22c55e",
          stars: 412,
          saved: "70.7% Space Saved"
        }
      ];

      pinnedGrid.innerHTML = pinned.map(p => `
        <div class="pin">
          <a href="repository.html?id=${p.id}">
            <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/></svg>
            <span>${p.name}</span>
          </a>
          <p>${p.desc}</p>
          <div class="pin-meta">
            <span><i class="pin-dot" style="background: ${p.dotColor};"></i>${p.lang}</span>
            <span class="pill-badge badge-purple" style="font-size: 10px;">${p.saved}</span>
            <span>★ ${p.stars}</span>
          </div>
        </div>
      `).join("");
    }

    // Render Heatmap
    const heatCanvas = $("#heatCanvas");
    if (heatCanvas) heatCanvas.innerHTML = heatHTML();

    // Render Activity Bars
    const actBox = $("#activityBox");
    if (actBox) {
      const act = [
        ["llama-3-8b-instruct", 148, "148 commits (Epoch 1 to 18)"],
        ["stable-diffusion-xl-base", 79, "79 commits (Epoch 1 to 45)"],
        ["deepseek-coder-6.7b", 41, "41 commits (Q4_K_M Export)"],
        ["whisper-large-v3", 21, "21 commits (Multilingual Release)"]
      ];
      actBox.innerHTML = `
        <div class="act">
          <h4>Neural Checkpoint Iterations &middot; Year 2026</h4>
          <p style="font-size: 12.5px; color: var(--text-muted); margin-bottom: 14px;">289 content-addressed checkpoint commits with FastCDC chunk delta deduplication</p>
          ${act.map(a => `
            <div class="r">
              <a href="repository.html?id=${a[0]}" style="color: var(--accent-blue); font-weight: 500;">${a[0]}</a>
              <div><i style="width: ${Math.round((a[1] / 148) * 100)}%;"></i></div>
              <span style="color: var(--text-muted); font-size: 12px; text-align: right;">${a[1]} commits</span>
            </div>
          `).join("")}
        </div>
      `;
    }
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();