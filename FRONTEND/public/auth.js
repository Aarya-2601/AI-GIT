/* =========================================================
   AI-GIT AUTHENTICATION LOGIC (Client-side)
========================================================= */

(function () {
  const alertEl = document.getElementById("authAlert");
  const tabIn = document.getElementById("tabSignIn");
  const tabUp = document.getElementById("tabSignUp");
  const formIn = document.getElementById("formSignIn");
  const formUp = document.getElementById("formSignUp");

  const modePwBtn = document.getElementById("modePw");
  const modeOtpBtn = document.getElementById("modeOtp");
  const inPwFields = document.getElementById("inPwFields");
  const inOtpFields = document.getElementById("inOtpFields");

  const btnSendOtpIn = document.getElementById("btnSendOtpIn");
  const btnSendOtpUp = document.getElementById("btnSendOtpUp");
  const btnDemoLogin = document.getElementById("btnDemoLogin");

  let currentMode = "pw"; // 'pw' or 'otp'
  let otpTimer = null;

  function showAlert(message, type = "error") {
    if (!alertEl) return;
    alertEl.textContent = message;
    alertEl.className = `auth-alert ${type}`;
    alertEl.style.display = "flex";
  }

  function hideAlert() {
    if (!alertEl) return;
    alertEl.style.display = "none";
    alertEl.textContent = "";
  }

  // Tab switching
  if (tabIn && tabUp) {
    tabIn.addEventListener("click", () => {
      tabIn.classList.add("active");
      tabUp.classList.remove("active");
      formIn.style.display = "block";
      formUp.style.display = "none";
      hideAlert();
    });

    tabUp.addEventListener("click", () => {
      tabUp.classList.add("active");
      tabIn.classList.remove("active");
      formIn.style.display = "none";
      formUp.style.display = "block";
      hideAlert();
    });
  }

  // Sign In Mode switching (Password vs OTP)
  if (modePwBtn && modeOtpBtn) {
    modePwBtn.addEventListener("click", () => {
      currentMode = "pw";
      modePwBtn.classList.add("active");
      modeOtpBtn.classList.remove("active");
      inPwFields.style.display = "block";
      inOtpFields.style.display = "none";
      hideAlert();
    });

    modeOtpBtn.addEventListener("click", () => {
      currentMode = "otp";
      modeOtpBtn.classList.add("active");
      modePwBtn.classList.remove("active");
      inPwFields.style.display = "none";
      inOtpFields.style.display = "block";
      hideAlert();
    });
  }

  // Send OTP Helper
  async function triggerSendOtp(emailInput, btn) {
    const email = (emailInput.value || "").trim();
    if (!email || !/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email)) {
      showAlert("Please enter a valid email address.", "error");
      emailInput.focus();
      return;
    }

    btn.disabled = true;
    btn.textContent = "Sending...";
    hideAlert();

    try {
      const res = await fetch("/api/auth/send-otp", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ email })
      });
      const data = await res.json();

      if (res.ok) {
        let msg = "Verification code generated!";
        if (data.otp) {
          msg += ` Code: [ ${data.otp} ] (Auto-filled for testing)`;
          const otpInput = document.getElementById(emailInput.id === "inEmail" ? "inOtp" : "upOtp");
          if (otpInput) otpInput.value = data.otp;
        }
        showAlert(msg, "success");

        let countdown = 30;
        btn.textContent = `Resend (${countdown}s)`;
        otpTimer = setInterval(() => {
          countdown--;
          if (countdown <= 0) {
            clearInterval(otpTimer);
            btn.disabled = false;
            btn.textContent = "Send OTP";
          } else {
            btn.textContent = `Resend (${countdown}s)`;
          }
        }, 1000);
      } else {
        showAlert(data.error || "Failed to send code.", "error");
        btn.disabled = false;
        btn.textContent = "Send OTP";
      }
    } catch (e) {
      showAlert("Network error sending code: " + e.message, "error");
      btn.disabled = false;
      btn.textContent = "Send OTP";
    }
  }

  if (btnSendOtpIn) {
    btnSendOtpIn.addEventListener("click", () => {
      triggerSendOtp(document.getElementById("inEmail"), btnSendOtpIn);
    });
  }

  if (btnSendOtpUp) {
    btnSendOtpUp.addEventListener("click", () => {
      triggerSendOtp(document.getElementById("upEmail"), btnSendOtpUp);
    });
  }

  // Instant Demo Login
  if (btnDemoLogin) {
    btnDemoLogin.addEventListener("click", async () => {
      btnDemoLogin.disabled = true;
      btnDemoLogin.innerHTML = `<span style="display:inline-block;animation:spin 1s linear infinite;">⏳</span> Signing in as AI Researcher...`;
      try {
        const res = await fetch("/api/auth/demo-login", { method: "POST" });
        const data = await res.json();
        if (res.ok) {
          window.location.href = "dashboard.html";
        } else {
          showAlert(data.error || "Demo sign in failed", "error");
          btnDemoLogin.disabled = false;
          btnDemoLogin.textContent = "⚡ Instant 1-Click Researcher Demo Sign In";
        }
      } catch (e) {
        showAlert("Network error during demo login: " + e.message, "error");
        btnDemoLogin.disabled = false;
      }
    });
  }

  // Handle Sign In Submit
  if (formIn) {
    formIn.addEventListener("submit", async (e) => {
      e.preventDefault();
      hideAlert();

      let payload = { mode: currentMode };
      if (currentMode === "pw") {
        payload.username = document.getElementById("inUser").value.trim();
        payload.password = document.getElementById("inPass").value;
        if (!payload.username || !payload.password) {
          showAlert("Please fill in both username and password.", "error");
          return;
        }
      } else {
        payload.email = document.getElementById("inEmail").value.trim();
        payload.otp = document.getElementById("inOtp").value.trim();
        if (!payload.email || !payload.otp) {
          showAlert("Please fill in both email and 6-digit OTP code.", "error");
          return;
        }
      }

      const submitBtn = formIn.querySelector('button[type="submit"]');
      submitBtn.disabled = true;
      submitBtn.textContent = "Authenticating...";

      try {
        const res = await fetch("/api/auth/login", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(payload)
        });
        const data = await res.json();

        if (res.ok) {
          showAlert("Sign in successful! Redirecting to dashboard...", "success");
          setTimeout(() => {
            window.location.href = "dashboard.html";
          }, 400);
        } else {
          showAlert(data.error || "Sign in failed.", "error");
          submitBtn.disabled = false;
          submitBtn.textContent = "Sign In to AI-GIT";
        }
      } catch (err) {
        showAlert("Server connection failed: " + err.message, "error");
        submitBtn.disabled = false;
        submitBtn.textContent = "Sign In to AI-GIT";
      }
    });
  }

  // Handle Sign Up Submit
  if (formUp) {
    formUp.addEventListener("submit", async (e) => {
      e.preventDefault();
      hideAlert();

      const name = document.getElementById("upName").value.trim();
      const username = document.getElementById("upUser").value.trim();
      const email = document.getElementById("upEmail").value.trim();
      const password = document.getElementById("upPass").value;
      const confirmPassword = document.getElementById("upPass2").value;
      const otp = document.getElementById("upOtp").value.trim();

      if (!name || !username || !email || !password) {
        showAlert("All required fields must be filled in.", "error");
        return;
      }

      if (password !== confirmPassword) {
        showAlert("Passwords do not match.", "error");
        return;
      }

      if (password.length < 6) {
        showAlert("Password must be at least 6 characters.", "error");
        return;
      }

      const submitBtn = formUp.querySelector('button[type="submit"]');
      submitBtn.disabled = true;
      submitBtn.textContent = "Creating account...";

      try {
        const res = await fetch("/api/auth/signup", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ name, username, email, password, otp })
        });
        const data = await res.json();

        if (res.ok) {
          showAlert("Account created successfully! Redirecting...", "success");
          setTimeout(() => {
            window.location.href = "dashboard.html";
          }, 500);
        } else {
          showAlert(data.error || "Sign up failed.", "error");
          submitBtn.disabled = false;
          submitBtn.textContent = "Create AI-GIT Account";
        }
      } catch (err) {
        showAlert("Server connection failed: " + err.message, "error");
        submitBtn.disabled = false;
        submitBtn.textContent = "Create AI-GIT Account";
      }
    });
  }
})();