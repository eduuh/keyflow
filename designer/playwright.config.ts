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
    baseURL: "http://localhost:3000",
    trace: "on-first-retry",
  },
  projects: [
    {
      name: "chromium",
      use: { ...devices["Desktop Chrome"] },
    },
  ],
  webServer: {
    command: "npm run dev",
    url: "http://localhost:3000",
    reuseExistingServer: !process.env.CI,
    timeout: 120_000,
  },
});
