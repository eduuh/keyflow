import { test } from "@playwright/test";
import { mkdir } from "node:fs/promises";
import { resolve } from "node:path";

// Generates documentation screenshots for the Eduuh-DH layout.
//
// Not run as part of `npm test` — gated to its own Playwright project
// (`--project=capture` via playwright.config.ts), invoked via
// `npm run capture-images`. Output is committed alongside the code change
// that motivated regenerating the images.
//
// The route under test (designer/app/screenshot/page.tsx) imports
// src/config.json directly, so screenshots always reflect the bundled
// Eduuh-DH config.

// Playwright runs from the designer/ directory (that's where package.json
// lives). Use process.cwd() to dodge ESM/CJS ambiguity around import.meta.
const docsImagesDir = resolve(process.cwd(), "../docs/images");

const SHOTS = [
  { layer: "all", file: "eduuh-dh-all.png", label: "all layers composite" },
  { layer: "base", file: "eduuh-dh-base.png", label: "BASE remap only" },
  { layer: "0", file: "eduuh-dh-numpad.png", label: "Numpad layer (LAlt)" },
  { layer: "1", file: "eduuh-dh-arrows.png", label: "Arrows & Symbols layer (RAlt)" },
];

test.beforeAll(async () => {
  await mkdir(docsImagesDir, { recursive: true });
});

test.beforeEach(async ({ page }) => {
  // Clear the persisted theme so the screenshot route's dark-mode override
  // doesn't race against a stale "light" class hydrated from localStorage.
  await page.addInitScript(() => {
    window.localStorage.removeItem("theme");
  });
});

for (const shot of SHOTS) {
  test(`capture ${shot.label}`, async ({ page }) => {
    await page.goto(`/screenshot/?layer=${shot.layer}`);
    await page.locator("[data-screenshot-ready]").waitFor();
    // Fonts settling avoids glyph FOUT artifacts in the snapshot.
    await page.evaluate(() => document.fonts.ready);
    await page
      .locator("#screenshot-root")
      .screenshot({
        path: resolve(docsImagesDir, shot.file),
        animations: "disabled",
      });
  });
}
