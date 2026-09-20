/* Theme toggle: light / dark / system, persisted in localStorage.
   Runs inline+early (see <head> snippet in each page) to avoid a flash of
   the wrong theme; this file wires up the toggle button after load. */
(function () {
  "use strict";

  function getStored() {
    try {
      return localStorage.getItem("aigit-theme");
    } catch (e) {
      return null;
    }
  }

  function setStored(value) {
    try {
      localStorage.setItem("aigit-theme", value);
    } catch (e) {
      /* storage unavailable (private mode, blocked); theme just won't persist */
    }
  }

  function apply(theme) {
    if (theme === "light" || theme === "dark") {
      document.documentElement.setAttribute("data-theme", theme);
    } else {
      document.documentElement.removeAttribute("data-theme");
    }
  }

  function initToggle() {
    var btn = document.querySelector("[data-theme-toggle]");
    if (!btn) return;

    btn.addEventListener("click", function () {
      var current = document.documentElement.getAttribute("data-theme");
      var prefersDark = window.matchMedia && window.matchMedia("(prefers-color-scheme: dark)").matches;
      var effectiveCurrent = current || (prefersDark ? "dark" : "light");
      var next = effectiveCurrent === "dark" ? "light" : "dark";
      apply(next);
      setStored(next);
    });
  }

  document.addEventListener("DOMContentLoaded", initToggle);

  window.AIGIT_THEME = { apply: apply, getStored: getStored };
})();
