/* ==========================================================================
   AI-GIT API client
   Thin fetch wrappers around the BACKEND Express API.
   Base URL defaults to same-origin /api — override via window.AIGIT_API_BASE
   if the frontend is ever served separately from the backend.
   ========================================================================== */
(function () {
  "use strict";

  var API_BASE = window.AIGIT_API_BASE || "";
  var HEALTH_TIMEOUT_MS = 4000;

  function withTimeout(promise, ms) {
    var controller = new AbortController();
    var timer = setTimeout(function () { controller.abort(); }, ms);
    return { controller: controller, timer: timer };
  }

  /**
   * GET /health
   * Returns { ok: boolean, status: number|null, latencyMs: number|null }
   */
  async function getHealth() {
    var t = withTimeout(null, HEALTH_TIMEOUT_MS);
    var started = performance.now();
    try {
      var res = await fetch(API_BASE + "/health", {
        method: "GET",
        signal: t.controller.signal,
        cache: "no-store",
      });
      clearTimeout(t.timer);
      return {
        ok: res.ok,
        status: res.status,
        latencyMs: Math.round(performance.now() - started),
      };
    } catch (err) {
      clearTimeout(t.timer);
      return { ok: false, status: null, latencyMs: null, error: err };
    }
  }

  // TODO(integration): POST /api/v1/push/negotiate
  // Kicks off a push — negotiates which CAS objects the server is missing
  // before the CLI/browser uploads chunk data. Expected body: repo name,
  // branch, local commit graph / object hash list. Wire this in once the
  // dashboard supports initiating a push from the browser.
  async function negotiatePush(payload) {
    throw new Error("negotiatePush() not implemented — see TODO in js/api.js");
  }

  // TODO(integration): POST /api/v1/pull/urls
  // Requests signed MinIO URLs for the CAS objects needed to sync a branch
  // locally. Expected body: repo name, branch, local commit hash (for diff).
  async function requestPullUrls(payload) {
    throw new Error("requestPullUrls() not implemented — see TODO in js/api.js");
  }

  // TODO(integration): GET /clone/:repoName
  // Fetches everything needed to clone a repo fresh. Wire this in once the
  // dashboard supports "Clone" as a browser action (currently CLI-only).
  async function cloneRepo(repoName) {
    throw new Error("cloneRepo() not implemented — see TODO in js/api.js");
  }

  window.AIGIT_API = {
    getHealth: getHealth,
    negotiatePush: negotiatePush,
    requestPullUrls: requestPullUrls,
    cloneRepo: cloneRepo,
  };
})();
