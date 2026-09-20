/* Subtle scroll-entrance animation using IntersectionObserver.
   Elements marked data-reveal fade/slide in once when they enter the
   viewport. Falls back to instantly-visible if IO is unsupported. */
(function () {
  "use strict";

  function init() {
    var targets = document.querySelectorAll("[data-reveal]");
    if (!targets.length) return;

    if (!("IntersectionObserver" in window)) {
      targets.forEach(function (el) { el.classList.add("is-visible"); });
      return;
    }

    var observer = new IntersectionObserver(
      function (entries) {
        entries.forEach(function (entry) {
          if (entry.isIntersecting) {
            var delay = entry.target.getAttribute("data-reveal-delay");
            if (delay) {
              entry.target.style.transitionDelay = delay + "ms";
            }
            entry.target.classList.add("is-visible");
            observer.unobserve(entry.target);
          }
        });
      },
      { threshold: 0.15, rootMargin: "0px 0px -40px 0px" }
    );

    targets.forEach(function (el) { observer.observe(el); });
  }

  document.addEventListener("DOMContentLoaded", init);
})();
