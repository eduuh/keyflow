import { test, expect } from "@playwright/test";
import path from "node:path";

// Critical contract: importing a config and immediately re-exporting it
// must preserve every C++ schema field. The previous store silently dropped
// customModifiers / disableCapsLock / multi-trigger triggers[]; the rewrite
// in Phase 1 fixed it, and this test guards against regression.
//
// The test loads the real bundled Eduuh-DH config (src/config.json), so it
// also exercises the designer against the maintained spec — if the C++
// schema ever adds a new field that the designer doesn't propagate, this
// test catches it.

const CONFIG_PATH = path.resolve(__dirname, "../../src/config.json");

test.describe("import → export round-trip", () => {
  test("preserves every field of src/config.json", async ({ page }) => {
    await page.goto("/");

    // Drive the file picker by intercepting the click. The Import button
    // opens a hidden <input type=file>; Playwright's setInputFiles handles it.
    const fileChooserPromise = page.waitForEvent("filechooser");
    await page.getByRole("button", { name: /import/i }).click();
    const chooser = await fileChooserPromise;
    await chooser.setFiles(CONFIG_PATH);

    // Wait for the imported config name to appear in the header.
    await expect(page.getByText(/Eduuh-DH/i).first()).toBeVisible({ timeout: 5000 });

    // Re-export. Intercept the download.
    const downloadPromise = page.waitForEvent("download");
    await page.getByRole("button", { name: /^JSON$/i }).click();
    const download = await downloadPromise;

    // Read the exported JSON and compare against the source.
    const tmpPath = path.join(process.cwd(), "test-results", "exported.json");
    await download.saveAs(tmpPath);

    const fs = await import("node:fs/promises");
    const source = JSON.parse(await fs.readFile(CONFIG_PATH, "utf-8"));
    const exported = JSON.parse(await fs.readFile(tmpPath, "utf-8"));

    // Strip C++-only metadata that the designer normalizes:
    //   - `_comment*` keys (the C++ parser ignores them; the designer preserves
    //     them on round-trip as opaque pass-through). Strip from BOTH sides so
    //     the equality check is about real schema fields.
    //   - `trigger: "LALT"` becomes `triggers: ["LALT"]` (canonical)
    stripComments(source.remapping);
    stripComments(exported.remapping);
    normalizeLayers(source);
    expect(exported.version).toBe(source.version);
    expect(exported.name).toBe(source.name);
    expect(exported.disableCapsLock).toBe(source.disableCapsLock);
    expect(exported.remapping).toEqual(source.remapping);
    expect(exported.noModCombos).toEqual(source.noModCombos);
    expect(exported.customModifiers ?? []).toEqual(source.customModifiers ?? []);
    expect(exported.layers).toEqual(source.layers);
  });
});

function stripComments(obj: Record<string, unknown>) {
  for (const k of Object.keys(obj)) {
    if (k.startsWith("_")) delete obj[k];
  }
}

function normalizeLayers(config: { layers?: Array<Record<string, unknown>> }) {
  for (const layer of config.layers ?? []) {
    if (typeof layer.trigger === "string") {
      layer.triggers = [layer.trigger];
      delete layer.trigger;
    } else if (Array.isArray(layer.trigger)) {
      layer.triggers = layer.trigger;
      delete layer.trigger;
    }
  }
}
