/* Site-wide small behaviors: copy-to-clipboard on code blocks, current year
   in footer. Page-specific logic lives in nav.js, theme.js, dashboard.js,
   contact.js, cookie-consent.js, reveal.js. */
(function () {
  "use strict";

  function initCopyButtons() {
    document.querySelectorAll("[data-copy-target]").forEach(function (btn) {
      btn.addEventListener("click", function () {
        var target = document.getElementById(btn.getAttribute("data-copy-target"));
        if (!target) return;
        var text = target.textContent;
        var restore = btn.innerHTML;

        function flash(label) {
          btn.textContent = label;
          setTimeout(function () { btn.innerHTML = restore; }, 1400);
        }

        if (navigator.clipboard && navigator.clipboard.writeText) {
          navigator.clipboard.writeText(text).then(function () {
            flash("Copied");
          }, function () {
            flash("Press Ctrl+C");
          });
        } else {
          flash("Press Ctrl+C");
        }
      });
    });
  }

  function initFooterYear() {
    document.querySelectorAll("[data-current-year]").forEach(function (el) {
      el.textContent = new Date().getFullYear();
    });
  }

  document.addEventListener("DOMContentLoaded", function () {
    initCopyButtons();
    initFooterYear();
  });
})();
