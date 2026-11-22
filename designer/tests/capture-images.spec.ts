import { test, expect } from "@playwright/test";
import { mkdir, readFile } from "node:fs/promises";
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
  { layer: "Win Layer", file: "eduuh-dh-win.png", label: "Win layer (WIN_MOD)" },
  { layer: "Numpad Layer", file: "eduuh-dh-numpad.png", label: "Numpad layer (LAlt)" },
  { layer: "Arrows and Symbols Layer", file: "eduuh-dh-arrows.png", label: "Arrows & Symbols layer (RAlt)" },
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
    const config = JSON.parse(await readFile(resolve(process.cwd(), "../src/config.json"), "utf8"));
    const index = config.layers.findIndex((layer: { name: string }) => layer.name === shot.layer);
    const isNamedLayer = shot.layer !== "all" && shot.layer !== "base";
    if (isNamedLayer) expect(index, `${shot.layer} must exist in the maintained config`).toBeGreaterThanOrEqual(0);
    await page.goto(`/screenshot/?layer=${isNamedLayer ? index : shot.layer}`);
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
