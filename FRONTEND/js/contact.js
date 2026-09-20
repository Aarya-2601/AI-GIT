/* Contact form — front-end only. No backend wiring yet; simulates a
   submission and shows inline status. */
(function () {
  "use strict";

  function init() {
    var form = document.getElementById("contact-form");
    if (!form) return;

    var status = document.getElementById("contact-status");

    form.addEventListener("submit", function (e) {
      e.preventDefault();

      if (!form.checkValidity()) {
        form.reportValidity();
        return;
      }

      var submitBtn = form.querySelector("button[type=submit]");
      submitBtn.disabled = true;
      submitBtn.textContent = "Sending…";

      // TODO(integration): POST to a real support/ticketing endpoint once
      // the BACKEND exposes one. For now this just simulates latency.
      setTimeout(function () {
        status.textContent = "Thanks — your message has been recorded. We'll get back to you shortly. (Demo form: no email was actually sent.)";
        status.className = "form-status visible success";
        form.reset();
        submitBtn.disabled = false;
        submitBtn.textContent = "Send message";
      }, 700);
    });
  }

  document.addEventListener("DOMContentLoaded", init);
})();
