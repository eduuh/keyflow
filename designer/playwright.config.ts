import { defineConfig, devices } from "@playwright/test";

// Playwright test setup. Boots `next dev` on port 3000 before running tests,
// shuts it down after. Only Chromium is configured by default — adding
// Firefox/WebKit is one entry away if needed for a release matrix.

export default defineConfig({
  testDir: "./tests",
  fullyParallel: false, // Tests share the same dev server, run sequentially
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 2 : 0,
  workers: 1,
  reporter: process.env.CI ? "github" : "list",
  use: {
    // Port 3100 instead of the Next default 3000 — Windows/WSL2 routinely
    // reserves 3000 invisibly (port appears free in netstat but binding
    // fails), so we move out of the way.
    baseURL: "http://localhost:3100",
    trace: "on-first-retry",
  },
  projects: [
    {
      name: "chromium",
      // Capture spec is opt-in via the `capture` project (npm run capture-images).
      // Keeping it out of `npm test` avoids accidentally regenerating doc PNGs
      // on every CI run.
      testIgnore: /capture-images\.spec\.ts/,
      use: { ...devices["Desktop Chrome"] },
    },
    {
      name: "capture",
      testMatch: /capture-images\.spec\.ts/,
      // Higher device scale + wider viewport → sharper, complete keyboard PNGs.
      // ExportKeyboard at scale=80 renders 15 * 80 = 1200px wide; viewport 1400
      // gives enough room for the route's 24px padding.
      use: {
        ...devices["Desktop Chrome"],
        deviceScaleFactor: 2,
        viewport: { width: 1400, height: 600 },
      },
    },
  ],
  webServer: {
    command: "npm run dev -- --port 3100",
    url: "http://localhost:3100",
    reuseExistingServer: !process.env.CI,
    timeout: 120_000,
  },
});
