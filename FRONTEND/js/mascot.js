/* ==========================================================================
   Mascot hook point — Phase 2 (not implemented yet)
   ==========================================================================
   This file documents the state-change contract that a future animated
   character/guide will implement. It intentionally does nothing today.

   Expected wiring, once built:
   - A #mascot container exists in the DOM on every page (see partials in
     each HTML file, and .mascot rule in css/components.css, currently
     `display: none`).
   - Other scripts (nav.js, dashboard.js, api.js) will call
     `window.AIGIT_MASCOT.setState(stateName, detail)` at key moments:
       - "idle"          default resting state
       - "greet"         on first page load
       - "loading"       while a dashboard/API fetch is in flight
       - "success"       after an API call succeeds (e.g. push/pull complete)
       - "error"         after an API call fails (e.g. /health offline)
       - "celebrate"     on notable milestones (e.g. first repo created)
   - setState() will be responsible for swapping the mascot's sprite/animation
     and any accompanying speech-bubble copy; it should degrade silently
     (no-op) if the mascot has not been built yet, which is what the stub
     below does.
   ========================================================================== */
(function () {
  "use strict";

  function setState(stateName, detail) {
    // TODO(phase-2): drive mascot animation/copy from stateName + detail.
    // No-op until the mascot is implemented.
  }

  window.AIGIT_MASCOT = { setState: setState };
})();
