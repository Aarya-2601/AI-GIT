/* =========================================================
   ORIGINAL CUTESY PROFILE STUDIO & README GENERATOR
   - Exactly matches the original cutesy template
   - Green hero heading ("Hi there! I'm [DisplayName]")
   - Pink Georgia italic headlines ("code, coffee, college clubs & curio")
   - Cute About Me emoji bullets (🔭 🌱 👯 🤔 💬 😄 ⚡)
   - Tech stack badges & Social links
   - 🌸 From [username] footer
========================================================= */

(function () {
  const $ = (id) => document.getElementById(id);
  const $$ = (sel) => Array.from(document.querySelectorAll(sel));

  const STACK_PRESETS = [
    { n: "JavaScript", c: "F7DF1E", t: "000", l: "javascript" },
    { n: "TypeScript", c: "3178C6", t: "fff", l: "typescript" },
    { n: "Python", c: "3776AB", t: "fff", l: "python" },
    { n: "React", c: "61DAFB", t: "000", l: "react" },
    { n: "Next.js", c: "000000", t: "fff", l: "nextdotjs" },
    { n: "Node.js", c: "5FA04E", t: "fff", l: "nodedotjs" },
    { n: "HTML5", c: "E34F26", t: "fff", l: "html5" },
    { n: "CSS3", c: "1572B6", t: "fff", l: "css3" },
    { n: "TailwindCSS", c: "06B6D4", t: "fff", l: "tailwindcss" },
    { n: "C++", c: "00599C", t: "fff", l: "cplusplus" },
    { n: "PyTorch", c: "EE4C2C", t: "fff", l: "pytorch" },
    { n: "Git", c: "F05032", t: "fff", l: "git" },
    { n: "GitHub", c: "181717", t: "fff", l: "github" },
    { n: "Docker", c: "2496ED", t: "fff", l: "docker" },
    { n: "MongoDB", c: "47A248", t: "fff", l: "mongodb" },
    { n: "PostgreSQL", c: "4169E1", t: "fff", l: "postgresql" },
    { n: "Figma", c: "F24E1E", t: "fff", l: "figma" },
    { n: "Linux", c: "FCC624", t: "000", l: "linux" }
  ];

  let state = {
    displayName: "Aarya Doshi",
    username: "Aarya-2601",
    avatar: "https://images.unsplash.com/photo-1534528741775-53994a69daeb?w=300&auto=format&fit=crop&q=80",
    location: "Mumbai, India",
    headline1: "code, coffee, college clubs & curio",
    headline2: "Building things that matter",
    bio: "Passionate software builder and creative developer. Love turning ideas into interactive digital experiences, playing with modern tools, and connecting with creators across the globe!",
    workingOn: "AI-GIT - Next-generation AI Model VCS",
    learning: "Creative Frontend & Distributed Systems",
    collaborate: "Open-source creative tools & cool web apps",
    help: "UI animations & interactive web visualizers",
    askMe: "JavaScript, Python, CSS aesthetics & git",
    pronouns: "she/her",
    funFact: "I can spot a 1px misalignment from across the room 🌸",
    portfolio: "https://aaryadoshi.dev",
    linkedin: "https://linkedin.com/in/aarya-doshi",
    instagram: "https://instagram.com/aarya_ai",
    github: "https://github.com/Aarya-2601",
    email: "aaryadoshi7@gmail.com",
    techStack: ["JavaScript", "Python", "React", "Node.js", "C++", "PyTorch", "HTML5", "CSS3", "Git"]
  };

  let currentStep = 1;
  const TOTAL_STEPS = 4;

  function loadState() {
    try {
      const saved = localStorage.getItem("aigit_profile_data_cutesy");
      if (saved) {
        state = { ...state, ...JSON.parse(saved) };
      }
    } catch (e) {
      console.warn("Could not load local profile state:", e);
    }
  }

  function saveState() {
    try {
      localStorage.setItem("aigit_profile_data_cutesy", JSON.stringify(state));
    } catch (e) {
      console.warn("Could not save profile state:", e);
    }
  }

  function esc(s) {
    return String(s || "")
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;")
      .replace(/'/g, "&#039;");
  }

  function showToast(msg) {
    const toast = $("toast");
    if (!toast) return;
    toast.textContent = msg;
    toast.hidden = false;
    toast.classList.add("show");
    setTimeout(() => {
      toast.hidden = true;
      toast.classList.remove("show");
    }, 2800);
  }

  /* ---------- Generate Original Cutesy Markdown ---------- */
  function generateMarkdown() {
    const p = state;
    const lines = [];

    lines.push('<p align="center">');
    lines.push(`<h1 align="center">Hi there! I'm ${p.displayName || p.username}</h1>`);
    if (p.headline1) lines.push(`<p align="center"><i>${p.headline1}</i></p>`);
    if (p.headline2) lines.push(`<p align="center"><i>${p.headline2}</i></p>`);
    lines.push('</p>');
    lines.push('');

    lines.push('### 🚀 About Me');
    lines.push('');
    if (p.bio) {
      lines.push(p.bio);
      lines.push('');
    }

    const addLine = (emoji, label, val) => {
      if (val) lines.push(`- ${emoji} **${label}:** ${val}`);
    };

    addLine("🔭", "Currently working on", p.workingOn);
    addLine("🌱", "Currently learning", p.learning);
    addLine("👯", "Looking to collaborate on", p.collaborate);
    addLine("🤔", "Looking for help with", p.help);
    addLine("💬", "Ask me about", p.askMe);
    addLine("😄", "Pronouns", p.pronouns);
    addLine("⚡", "Fun fact", p.funFact);
    lines.push('');

    if (p.techStack && p.techStack.length) {
      lines.push('### 🛠️ Tech Stack');
      lines.push('');
      lines.push(p.techStack.map(t => ``${t}``).join(' '));
      lines.push('');
    }

    const links = [];
    if (p.portfolio) links.push(`[Portfolio](${p.portfolio})`);
    if (p.linkedin) links.push(`[LinkedIn](${p.linkedin})`);
    if (p.instagram) links.push(`[Instagram](${p.instagram})`);
    if (p.github) links.push(`[GitHub](${p.github})`);
    if (p.email) links.push(`[Email](mailto:${p.email})`);

    if (links.length) {
      lines.push('### 🔗 Connect');
      lines.push('');
      lines.push(links.join(' · '));
      lines.push('');
    }

    lines.push('---');
    lines.push('<p align="center">');
    lines.push(`🌸 From <a href="https://github.com/${encodeURIComponent(p.username)}">${p.username}</a> 🌸`);
    lines.push('</p>');

    return lines.join('\n');
  }

  /* ---------- Generate Original Cutesy HTML ---------- */
  function generateHTML() {
    const p = state;
    let h = '<div class="readme-body">';

    // Hero
    h += '<div class="hero">';
    h += `<h1>Hi there! I'm ${esc(p.displayName || p.username)}</h1>`;
    if (p.headline1) h += `<div class="headline">${esc(p.headline1)}</div>`;
    if (p.headline2) h += `<div class="headline">${esc(p.headline2)}</div>`;
    h += '</div>';

    // About Me
    h += '<section class="section">';
    h += '<div class="section-title">🚀 About Me</div>';
    if (p.bio) h += `<div class="bio">${esc(p.bio)}</div>`;

    h += '<div class="profile-lines">';
    const addHtmlLine = (emoji, label, val) => {
      if (val) {
        h += `<div class="profile-line">${emoji} <b>${label}:</b> <span>${esc(val)}</span></div>`;
      }
    };
    addHtmlLine("🔭", "Currently working on", p.workingOn);
    addHtmlLine("🌱", "Currently learning", p.learning);
    addHtmlLine("👯", "Looking to collaborate on", p.collaborate);
    addHtmlLine("🤔", "Looking for help with", p.help);
    addHtmlLine("💬", "Ask me about", p.askMe);
    addHtmlLine("😄", "Pronouns", p.pronouns);
    addHtmlLine("⚡", "Fun fact", p.funFact);
    h += '</div>';
    h += '</section>';

    // Tech Stack
    if (p.techStack && p.techStack.length) {
      h += '<section class="section">';
      h += '<div class="section-title">🛠️ Tech Stack</div>';
      h += '<div class="badges">';
      h += p.techStack.map(s => {
        const found = STACK_PRESETS.find(x => x.n === s) || { c: "21262d", t: "fff", l: "git" };
        const fg = found.t === "fff" ? "ffffff" : "000000";
        return `<span class="badge" style="background:#${found.c};color:#${fg}"><img alt="" src="https://cdn.simpleicons.org/${found.l}/${fg}" width="14" height="14" onerror="this.remove()">${esc(s)}</span>`;
      }).join('');
      h += '</div>';
      h += '</section>';
    }

    // Connect
    const links = [];
    if (p.portfolio) links.push(`<a href="${esc(p.portfolio)}" class="social" target="_blank">🌐 Portfolio</a>`);
    if (p.linkedin) links.push(`<a href="${esc(p.linkedin)}" class="social" target="_blank">💼 LinkedIn</a>`);
    if (p.instagram) links.push(`<a href="${esc(p.instagram)}" class="social" target="_blank">📸 Instagram</a>`);
    if (p.github) links.push(`<a href="${esc(p.github)}" class="social" target="_blank">🐙 GitHub</a>`);
    if (p.email) links.push(`<a href="mailto:${esc(p.email)}" class="social">✉️ Email</a>`);

    if (links.length) {
      h += '<section class="section">';
      h += '<div class="section-title">🔗 Connect</div>';
      h += '<div class="socials">' + links.join('') + '</div>';
      h += '</section>';
    }

    // GitHub Summary
    h += '<section class="section">';
    h += '<div class="section-title">📊 GitHub</div>';
    h += '<div class="github-summary">';
    h += '<div class="summary-box"><div class="summary-number">8</div><div class="summary-label">Repositories</div></div>';
    h += '<div class="summary-box"><div class="summary-number">342</div><div class="summary-label">Stars</div></div>';
    h += '<div class="summary-box"><div class="summary-number">128</div><div class="summary-label">Followers</div></div>';
    h += '<div class="summary-box"><div class="summary-number">User</div><div class="summary-label">Account</div></div>';
    h += '</div>';
    h += '</section>';

    // Footer signature
    h += '<div style="margin-top: 36px; padding-top: 20px; border-top: 1px solid var(--border); text-align: center; color: var(--pink); font-family: Georgia, serif; font-style: italic; font-size: 15px;">';
    h += `🌸 From <a href="https://github.com/${encodeURIComponent(p.username)}" style="color:var(--pink);font-weight:bold;">${esc(p.username)}</a> 🌸`;
    h += '</div>';

    h += '</div>';
    return h;
  }

  /* ---------- Render Overview (View 1) ---------- */
  
  function renderProfileGitCalendarGrid() {
    const grid = document.getElementById("profileGitCalendarGrid");
    if (!grid || grid.children.length > 0) return;
    const totalWeeks = 48;
    for (let day = 0; day < 7; day++) {
      for (let week = 0; week < totalWeeks; week++) {
        const cell = document.createElement('div');
        let lvl = 'lvl-0';
        if (week >= 36) {
          const r = Math.random();
          if (r > 0.70) lvl = 'lvl-4';
          else if (r > 0.45) lvl = 'lvl-3';
          else if (r > 0.25) lvl = 'lvl-2';
          else lvl = 'lvl-1';
        } else if (week >= 28) {
          const r = Math.random();
          if (r > 0.75) lvl = 'lvl-3';
          else if (r > 0.55) lvl = 'lvl-2';
          else if (r > 0.35) lvl = 'lvl-1';
          else lvl = 'lvl-0';
        } else if (week >= 20) {
          const r = Math.random();
          if (r > 0.85) lvl = 'lvl-2';
          else if (r > 0.70) lvl = 'lvl-1';
          else lvl = 'lvl-0';
        } else {
          if (Math.random() > 0.94) lvl = 'lvl-1';
          else lvl = 'lvl-0';
        }
        cell.className = 'cal-cell ' + lvl;
        grid.appendChild(cell);
      }
    }
  }
  
  function renderOverview() {
    const dName = $("dName");
    const dUser = $("dUser");
    const dRole = $("dRole");
    const dBio = $("dBio");
    const avatarLetter = $("avatarLetter");
    const homeReadme = $("homeReadme");

    if (dName) dName.textContent = state.displayName || state.username;
    if (dUser) dUser.textContent = "@" + (state.username || "Aarya-2601");
    if (dRole) dRole.textContent = state.headline1 || "Building things that matter";
    if (dBio) dBio.textContent = state.bio || "";
    if (avatarLetter) {
      if (state.avatar) {
        avatarLetter.innerHTML = `<img src="${state.avatar}" alt="${state.displayName}" style="width:100%;height:100%;border-radius:50%;object-fit:cover;">`;
      } else {
        avatarLetter.textContent = (state.displayName || "A")[0].toUpperCase();
      }
    }

    if (homeReadme) {
      homeReadme.innerHTML = generateHTML();
    }
  }

  /* ---------- Render Studio Preview (View 2) ---------- */
  function renderStudioPreview() {
    const visual = $("visualPreview");
    const raw = $("rawMarkdown");
    if (visual) visual.innerHTML = generateHTML();
    if (raw) raw.value = generateMarkdown();
  }

  /* ---------- Bind Studio Form Inputs ---------- */
  function bindInputs() {
    $$("input[data-k], textarea[data-k]").forEach(input => {
      const k = input.getAttribute("data-k");
      if (state[k] !== undefined) {
        input.value = state[k];
      }
      input.addEventListener("input", (e) => {
        state[k] = e.target.value;
        saveState();
        renderStudioPreview();
      });
    });

    // Tech Stack Badges Picker
    const stackGrid = $("stackGrid");
    if (stackGrid) {
      stackGrid.innerHTML = STACK_PRESETS.map(p => {
        const active = state.techStack.includes(p.n);
        return `<button type="button" class="stack-badge-btn ${active ? 'active' : ''}" data-name="${p.n}">
          ${p.n}
        </button>`;
      }).join("");

      $$(".stack-badge-btn").forEach(btn => {
        btn.addEventListener("click", () => {
          const name = btn.getAttribute("data-name");
          if (state.techStack.includes(name)) {
            state.techStack = state.techStack.filter(x => x !== name);
            btn.classList.remove("active");
          } else {
            state.techStack.push(name);
            btn.classList.add("active");
          }
          saveState();
          renderStudioPreview();
        });
      });
    }
  }

  /* ---------- Stepper Navigation ---------- */
  function showStep(step) {
    currentStep = step;
    $$(".step").forEach(el => {
      const s = parseInt(el.getAttribute("data-s"), 10);
      el.hidden = s !== step;
    });

    const stepper = $("stepper");
    if (stepper) {
      stepper.innerHTML = [
        "1. Identity & Headlines",
        "2. About Me",
        "3. Tech Stack",
        "4. Connect & Socials"
      ].map((title, idx) => {
        const num = idx + 1;
        const cls = num === step ? "active" : num < step ? "done" : "";
        return `<button type="button" class="step-tab ${cls}" data-s="${num}">${title}</button>`;
      }).join("");

      $$(".step-tab").forEach(tab => {
        tab.addEventListener("click", () => showStep(parseInt(tab.getAttribute("data-s"), 10)));
      });
    }

    const backBtn = $("btnBackStep");
    const nextBtn = $("btnNextStep");
    if (backBtn) backBtn.disabled = step === 1;
    if (nextBtn) {
      nextBtn.textContent = step === TOTAL_STEPS ? "Finish & Save" : "Continue →";
    }
  }

  /* ---------- View Switcher (Tabs) ---------- */
  function setupTabs() {
    const tabHome = $("tabBtnHome");
    const tabStudio = $("tabBtnStudio");
    const homeView = $("homeView");
    const studioView = $("studioView");

    function switchTo(view) {
      if (view === "studio") {
        if (homeView) homeView.hidden = true;
        if (studioView) studioView.hidden = false;
        if (tabHome) tabHome.classList.remove("active");
        if (tabStudio) tabStudio.classList.add("active");
        renderStudioPreview();
      } else {
        if (homeView) homeView.hidden = false;
        if (studioView) studioView.hidden = true;
        if (tabHome) tabHome.classList.add("active");
        if (tabStudio) tabStudio.classList.remove("active");
        renderOverview();
    renderProfileGitCalendarGrid();
      }
    }

    if (tabHome) tabHome.addEventListener("click", () => switchTo("home"));
    if (tabStudio) tabStudio.addEventListener("click", () => switchTo("studio"));

    const editBtn = $("editProfileBtn");
    if (editBtn) editBtn.addEventListener("click", () => switchTo("studio"));

    // Check URL hash
    if (window.location.hash.includes("studio")) {
      switchTo("studio");
    } else {
      switchTo("home");
    }
  }

  /* ---------- Actions (Copy, Download, Save) ---------- */
  function setupActions() {
    const btnBack = $("btnBackStep");
    const btnNext = $("btnNextStep");

    if (btnBack) {
      btnBack.addEventListener("click", () => {
        if (currentStep > 1) showStep(currentStep - 1);
      });
    }

    if (btnNext) {
      btnNext.addEventListener("click", () => {
        if (currentStep < TOTAL_STEPS) {
          showStep(currentStep + 1);
        } else {
          saveState();
          showToast("Profile template saved!");
          const tabHome = $("tabBtnHome");
          if (tabHome) tabHome.click();
        }
      });
    }

    // Toggle Preview
    const tgVisual = $("tgVisual");
    const tgRaw = $("tgRaw");
    const visual = $("visualPreview");
    const raw = $("rawMarkdown");

    if (tgVisual && tgRaw) {
      tgVisual.addEventListener("click", () => {
        tgVisual.classList.add("active");
        tgRaw.classList.remove("active");
        if (visual) visual.hidden = false;
        if (raw) raw.hidden = true;
      });
      tgRaw.addEventListener("click", () => {
        tgRaw.classList.add("active");
        tgVisual.classList.remove("active");
        if (visual) visual.hidden = true;
        if (raw) raw.hidden = false;
      });
    }

    // Copy Markdown
    const btnCopy = $("btnCopyReadme");
    if (btnCopy) {
      btnCopy.addEventListener("click", () => {
        const md = generateMarkdown();
        navigator.clipboard.writeText(md).then(() => {
          showToast("README.md copied to clipboard! 📋");
        }).catch(() => {
          showToast("Copied to clipboard!");
        });
      });
    }

    // Download README
    const btnDownload = $("btnDownloadReadme");
    if (btnDownload) {
      btnDownload.addEventListener("click", () => {
        const md = generateMarkdown();
        const blob = new Blob([md], { type: "text/markdown;charset=utf-8" });
        const url = URL.createObjectURL(blob);
        const a = document.createElement("a");
        a.href = url;
        a.download = "README.md";
        a.click();
        URL.revokeObjectURL(url);
        showToast("Downloaded README.md! ✨");
      });
    }

    // Save Profile
    const btnSave = $("btnSaveProfile");
    if (btnSave) {
      btnSave.addEventListener("click", () => {
        saveState();
        showToast("Profile saved successfully! 🌸");
        const tabHome = $("tabBtnHome");
        if (tabHome) tabHome.click();
      });
    }
  }

  function init() {
    loadState();
    bindInputs();
    showStep(1);
    setupTabs();
    setupActions();
    renderOverview();
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
