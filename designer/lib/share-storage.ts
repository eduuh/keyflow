// Storage + validation helpers for the Share dialog.
//
// Token storage rule: gist PAT lives in sessionStorage (cleared on tab close)
//   — convenient enough to reuse for multiple uploads in one session, narrow
//   enough that walking away from the keyboard doesn't leak it. localStorage
//   was rejected because the designer is served from GitHub Pages, where any
//   XSS would have free read access.
//
// Recent URLs live in localStorage with a hard cap of 5, dedup-on-write.

const TOKEN_KEY = "keyflow-gist-token";
const URLS_KEY = "keyflow-recent-urls";
const MAX_RECENT = 5;

export function getGistToken(): string {
  if (typeof window === "undefined") return "";
  return window.sessionStorage.getItem(TOKEN_KEY) ?? "";
}

export function setGistToken(token: string): void {
  if (typeof window === "undefined") return;
  if (token) window.sessionStorage.setItem(TOKEN_KEY, token);
  else window.sessionStorage.removeItem(TOKEN_KEY);
}

export function getRecentUrls(): string[] {
  if (typeof window === "undefined") return [];
  try {
    const raw = window.localStorage.getItem(URLS_KEY);
    if (!raw) return [];
    const parsed = JSON.parse(raw);
    return Array.isArray(parsed)
      ? parsed.filter((u): u is string => typeof u === "string").slice(0, MAX_RECENT)
      : [];
  } catch {
    return [];
  }
}

export function addRecentUrl(url: string): void {
  if (typeof window === "undefined") return;
  const next = [url, ...getRecentUrls().filter((u) => u !== url)].slice(0, MAX_RECENT);
  try {
    window.localStorage.setItem(URLS_KEY, JSON.stringify(next));
  } catch {
    // localStorage may be unavailable (private browsing, quota); silent fail is fine.
  }
}

// ─── URL validation (USER CONTRIBUTION POINT) ────────────────────────────────
//
// Allowed forms (must be CORS-allowed raw JSON content):
//   • https://raw.githubusercontent.com/<user>/<repo>/<ref>/<path...>
//   • https://gist.githubusercontent.com/<user>/<id>/raw/<...>
//
// Common rejections that benefit from a specific hint:
//   • https://github.com/<user>/<repo>/blob/...
//       → tell the user to click "Raw" on GitHub
//   • https://gist.github.com/<user>/<id>
//       → tell the user to open the gist and click "Raw" on the file
//   • Any other host
//       → say only github.com raw URLs are accepted
//
// The error's `.message` is shown verbatim to the user in the Load dialog, so
// phrase rejections as actionable hints, not parser-internal jargon.

export function assertRawUrl(url: string): void {
  let parsed: URL;
  try {
    parsed = new URL(url);
  } catch {
    throw new Error("That doesn't look like a URL.");
  }
  if (parsed.protocol !== "https:") {
    throw new Error("URL must start with https://");
  }
  if (parsed.hostname === "github.com") {
    throw new Error(
      "That's a github.com page, not a raw file. Click the 'Raw' button on GitHub and paste that URL.",
    );
  }
  if (parsed.hostname === "gist.github.com") {
    throw new Error(
      "That's the gist page. Open the gist, click 'Raw' on the file, and paste that URL.",
    );
  }
  if (parsed.hostname === "raw.githubusercontent.com") return;
  if (parsed.hostname === "gist.githubusercontent.com" && parsed.pathname.includes("/raw/")) return;
  throw new Error(
    "Only raw.githubusercontent.com or gist.githubusercontent.com/.../raw/... URLs are accepted.",
  );
}
