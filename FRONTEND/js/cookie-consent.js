/* Cookie consent banner — vanilla JS, preference stored in localStorage. */
(function () {
  "use strict";

  var STORAGE_KEY = "aigit-cookie-consent";

  function getConsent() {
    try {
      return localStorage.getItem(STORAGE_KEY);
    } catch (e) {
      return null;
    }
  }

  function setConsent(value) {
    try {
      localStorage.setItem(STORAGE_KEY, value);
    } catch (e) {
      /* ignore — banner will just reappear next visit */
    }
  }

  function init() {
    var banner = document.querySelector("[data-cookie-banner]");
    if (!banner) return;

    if (getConsent()) return;

    requestAnimationFrame(function () {
      banner.classList.add("visible");
    });

    banner.addEventListener("click", function (e) {
      var action = e.target.closest("[data-cookie-action]");
      if (!action) return;
      var choice = action.getAttribute("data-cookie-action");
      setConsent(choice);
      banner.classList.remove("visible");
    });
  }

  document.addEventListener("DOMContentLoaded", init);
})();
