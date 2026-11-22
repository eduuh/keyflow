import { test, expect, Page } from "@playwright/test";
import JSZip from "jszip";
import { readFile } from "node:fs/promises";
import path from "node:path";

const config = {
  version: "1.0", name: "Workflow layout", disableCapsLock: false,
  remapping: { CapsLock: "LeftCtrl", A: "B" },
  noModCombos: [{ key: "1", output: "1", shift: true, description: "Exclamation" }],
  customModifiers: [{ key: "Space", modifierName: "SPACE_MOD", blockOutput: true }],
  layers: [
    { name: "Navigation", triggers: ["LCTRL", "SPACE_MOD"], mappings: { H: "Left" } },
    { name: "Numbers", triggers: ["RALT"], mappings: { H: "1" } },
    { name: "Symbols", triggers: ["LALT"], mappings: {}, shiftMappings: [{ key: "H", output: "3", shift: false }] },
  ],
  platformOverrides: { linux: { remapping: { A: "C" }, layers: [] }, windows: { disableCapsLock: true }, mac: {} },
};

test.beforeEach(async ({ page }) => {
  await page.addInitScript(() => localStorage.setItem("keyflow-designer-tour-seen", "true"));
  await page.goto("/");
});

async function importConfig(page: Page, value: unknown = config) {
  const chooser = page.waitForEvent("filechooser");
  await page.getByRole("button", { name: "Import", exact: true }).click();
  await (await chooser).setFiles({ name: "config.json", mimeType: "application/json", buffer: Buffer.from(JSON.stringify(value)) });
  await expect(page.getByLabel("Layout name", { exact: true })).toHaveValue((value as typeof config).name);
}

async function documentConfig(page: Page) {
  return page.evaluate(() => JSON.parse(localStorage.getItem("keyflow-config-storage")!).state.config);
}

async function selectKey(page: Page, key: string) {
  await page.locator(`[data-key="${key}"]`).click();
}

test("platform previews, key overrides, presets, inheritance and persistence", async ({ page }) => {
  await importConfig(page);
  await page.getByRole("button", { name: "Linux", exact: true }).click();
  await expect(page.locator('[data-key="A"]')).toHaveAccessibleName("A → C");
  await expect(page.getByRole("button", { name: "Edit layer Navigation" })).toHaveCount(0);
  await selectKey(page, "A");
  await page.getByRole("button", { name: "Map to D", exact: true }).click();
  expect((await documentConfig(page)).remapping.A).toBe("B");
  expect((await documentConfig(page)).platformOverrides.linux.remapping).toEqual({ A: "D" });
  page.once("dialog", (dialog) => dialog.accept());
  await page.getByRole("button", { name: "Use Shared / clear combo" }).click();
  await expect(page.locator('[data-key="A"]')).toHaveAccessibleName("A → B");
  for (const [platform, name, target] of [["linux", "Linux", "LeftWin"], ["windows", "Windows", "RightCtrl"], ["mac", "macOS", "LeftWin"]]) {
    await page.getByRole("button", { name, exact: true }).click();
    await page.getByRole("button", { name: `Apply ${name} preset`, exact: true }).click();
    expect((await documentConfig(page)).platformOverrides[platform].remapping.PrintScreen).toBe(target);
  }
  await page.getByRole("button", { name: "Windows", exact: true }).click();
  await expect(page.getByRole("switch", { name: "Disable Caps Lock" })).toBeChecked();
  await page.getByRole("button", { name: "Inherit Caps Lock" }).click();
  await expect(page.getByRole("switch", { name: "Disable Caps Lock" })).not.toBeChecked();
  await page.getByRole("button", { name: "Linux", exact: true }).click();
  await page.getByRole("button", { name: "Inherit Layers" }).click();
  await expect(page.getByRole("button", { name: "Edit layer Navigation" })).toBeVisible();
  await page.getByLabel("Layout name", { exact: true }).fill("Saved workflow");
  const saved = await documentConfig(page);
  await page.reload();
  await expect(page.getByLabel("Layout name", { exact: true })).toHaveValue("Saved workflow");
  expect(await documentConfig(page)).toEqual(saved);
  await expect(page.getByRole("button", { name: "Shared", exact: true })).toHaveAttribute("aria-pressed", "true");
});

test("mapping categories, extra keys, shifted output, layer creation, editing and deletion", async ({ page }) => {
  await importConfig(page);
  await selectKey(page, "A");
  await page.getByRole("tab", { name: "0-9", exact: true }).click();
  await page.getByRole("button", { name: "Map to 2", exact: true }).click();
  await page.getByRole("tab", { name: "Sym", exact: true }).click();
  await page.getByRole("button", { name: "Map to Slash", exact: true }).click();
  await page.getByRole("tab", { name: "Mod", exact: true }).click();
  await page.getByRole("button", { name: "Map to RightShift", exact: true }).click();
  await page.getByRole("tab", { name: "Nav", exact: true }).click();
  await page.getByRole("button", { name: "Map to Up", exact: true }).click();
  expect((await documentConfig(page)).remapping.A).toBe("Up");
  await page.getByLabel("Other physical keys").selectOption("PrintScreen");
  await page.getByLabel("All output keys").selectOption("MediaPlay");
  expect((await documentConfig(page)).remapping.PrintScreen).toBe("MediaPlay");

  await page.getByRole("button", { name: "Add layer" }).click();
  await selectKey(page, "A");
  await page.getByRole("tab", { name: "Shift+", exact: true }).click();
  await page.getByRole("button", { name: "Map to Shift+1", exact: true }).click();
  await expect(page.locator('[data-key="A"]')).toHaveAccessibleName("A → !");
  const layers = (await documentConfig(page)).layers;
  expect(layers[3].triggers).toEqual(["LCTRL"]);
  expect(layers[3].shiftMappings).toEqual([{ key: "A", output: "1", shift: true }]);
  await page.getByRole("tab", { name: "A-Z", exact: true }).click();
  await page.getByRole("button", { name: "Map to Z", exact: true }).click();
  expect((await documentConfig(page)).layers[3].shiftMappings).toEqual([]);
  page.once("dialog", (dialog) => dialog.accept());
  await page.getByRole("button", { name: "Clear", exact: true }).click();
  expect((await documentConfig(page)).layers[3].mappings).toEqual({});
  await page.getByRole("button", { name: "Edit layer Layer 4", exact: true }).click();
  await page.getByLabel("Name", { exact: true }).fill("New layer");
  await page.getByRole("button", { name: "SPACE_MOD", exact: true }).click();
  await page.getByRole("button", { name: "Save", exact: true }).click();
  expect((await documentConfig(page)).layers[3].triggers).toEqual(["LCTRL", "SPACE_MOD"]);
  await page.getByRole("button", { name: "Edit layer New layer", exact: true }).click();
  page.once("dialog", (dialog) => dialog.accept());
  await page.getByRole("button", { name: "Delete", exact: true }).click();
  expect((await documentConfig(page)).layers).toHaveLength(3);
});

test("combo and custom modifier edits replace rows, update triggers and stay scoped", async ({ page }) => {
  await importConfig(page);
  await page.getByRole("button", { name: "Advanced settings" }).click();
  await page.getByRole("combobox", { name: "Combo 1 source" }).click();
  await page.getByRole("option", { name: "2", exact: true }).click();
  await page.getByRole("combobox", { name: "Combo 1 output" }).click();
  await page.getByRole("option", { name: "[ LeftBracket", exact: true }).click();
  await page.getByLabel("Combo 1 description").fill("Bracket");
  await page.getByRole("switch", { name: "Combo 1 shift" }).click();
  expect((await documentConfig(page)).noModCombos).toEqual([{ key: "2", output: "LeftBracket", shift: false, description: "Bracket" }]);
  await page.getByRole("button", { name: "Add combo", exact: true }).click();
  await expect(page.getByRole("button", { name: "Remove combo", exact: true })).toHaveCount(2);
  await page.getByRole("button", { name: "Remove combo", exact: true }).last().click();
  await page.getByLabel("Modifier 1 name").fill("SPACE_NEW");
  expect((await documentConfig(page)).customModifiers).toHaveLength(1);
  expect((await documentConfig(page)).layers[0].triggers).toEqual(["LCTRL", "SPACE_NEW"]);
  await page.getByRole("switch", { name: "Block output", exact: true }).click();
  expect((await documentConfig(page)).customModifiers[0].blockOutput).toBe(false);
  await page.getByRole("button", { name: "Add custom modifier", exact: true }).click();
  await expect(page.getByRole("button", { name: "Remove custom modifier" })).toHaveCount(2);
  await page.getByRole("button", { name: "Remove custom modifier" }).last().click();
  page.once("dialog", (dialog) => dialog.accept());
  await page.getByRole("button", { name: "Remove custom modifier" }).click();
  expect((await documentConfig(page)).layers[0].triggers).toEqual(["LCTRL"]);
});

test("JSON format, validation, reset, download and save retain all environments", async ({ page }) => {
  await importConfig(page);
  await page.getByRole("button", { name: "JSON", exact: true }).click();
  const editor = page.getByRole("textbox", { name: "Configuration JSON" });
  await editor.fill('{"version":"1.0","platformOverrides":{"linux":null}}');
  await page.getByRole("button", { name: "Save", exact: true }).click();
  await expect(page.getByRole("alert")).toContainText("platformOverrides.linux");
  page.once("dialog", (dialog) => dialog.accept());
  await page.getByRole("button", { name: "Reset", exact: true }).click();
  expect(JSON.parse(await editor.inputValue()).platformOverrides).toEqual(config.platformOverrides);
  await editor.fill(JSON.stringify({ ...config, name: "JSON edited" }));
  await page.getByRole("button", { name: "Format", exact: true }).click();
  expect(await editor.inputValue()).toContain('\n  "version"');
  const download = page.waitForEvent("download");
  await page.getByRole("button", { name: "Download", exact: true }).click();
  expect(JSON.parse(await readFile((await (await download).path())!, "utf8")).name).toBe("JSON edited");
  await page.getByRole("button", { name: "Save", exact: true }).click();
  await expect(page.getByLabel("Layout name", { exact: true })).toHaveValue("JSON edited");
  expect((await documentConfig(page)).platformOverrides).toEqual(config.platformOverrides);
});

test("image categories include physical defaults, all layers, selected platform and ZIP downloads", async ({ page }) => {
  test.setTimeout(120_000);
  await importConfig(page);
  await page.getByRole("button", { name: "Windows", exact: true }).click();
  await page.setViewportSize({ width: 390, height: 844 });
  await page.getByRole("button", { name: "Export image", exact: true }).click();
  const preview = page.getByLabel("Export preview");
  await expect(preview.locator('[data-export-layer="2"]')).toContainText("3: 3");
  for (const category of ["Letters", "Numbers", "Symbols", "Directional", "Functional", "Return", "Full layout"]) {
    await page.getByRole("tab", { name: category, exact: true }).click();
    await expect(page.getByText("Highlighted:")).toBeVisible();
  }
  const pngPromise = page.waitForEvent("download");
  await page.getByRole("button", { name: "Download this PNG" }).click();
  const png = await pngPromise;
  expect(png.suggestedFilename()).toContain("-windows-layout.png");
  const image = await readFile((await png.path())!);
  expect(image.subarray(0, 8)).toEqual(Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]));
  expect(image.readUInt32BE(16)).toBeGreaterThanOrEqual(15 * 64 * 2);
  const zipPromise = page.waitForEvent("download");
  await page.getByRole("button", { name: "Download all (.zip)" }).click();
  const zip = await JSZip.loadAsync(await readFile((await (await zipPromise).path())!));
  expect(Object.keys(zip.files)).toHaveLength(7);
  for (const file of Object.values(zip.files)) expect((await file.async("uint8array")).length).toBeGreaterThan(1000);
});

test("many-layer image preview keeps the final layer badge inside the keycap", async ({ page }) => {
  await importConfig(page, { ...config, layers: Array.from({ length: 20 }, (_, index) => ({
    name: `Layer ${index + 1}`, triggers: ["LCTRL"], mappings: { H: "A" },
  })) });
  await page.getByRole("button", { name: "Export image", exact: true }).click();
  const keycap = page.getByLabel("Export preview").locator('[data-export-key="H"]');
  await page.getByRole("dialog").evaluate((element) =>
    Promise.all(element.getAnimations({ subtree: true }).map((animation) => animation.finished)));
  const keyRect = await keycap.boundingBox();
  const lastRect = await keycap.locator('[data-export-layer="19"]').boundingBox();
  expect(keyRect).not.toBeNull();
  expect(lastRect).not.toBeNull();
  expect(lastRect!.y + lastRect!.height).toBeLessThanOrEqual(keyRect!.y + keyRect!.height);
});

test("coupled section reset asks before dropping platform modifier and layer edits", async ({ page }) => {
  await importConfig(page, { ...config, platformOverrides: { linux: {}, windows: {}, mac: {} } });
  await page.getByRole("button", { name: "Linux", exact: true }).click();
  await page.getByRole("button", { name: "Advanced settings" }).click();
  await page.getByLabel("Modifier 1 name").fill("SPACE_NEW");
  const before = await documentConfig(page);
  page.once("dialog", (dialog) => dialog.dismiss());
  await page.getByRole("button", { name: "Inherit Custom modifiers" }).click();
  expect(await documentConfig(page)).toEqual(before);
  page.once("dialog", (dialog) => dialog.accept());
  await page.getByRole("button", { name: "Inherit Custom modifiers" }).click();
  expect((await documentConfig(page)).platformOverrides.linux).toEqual({});
  expect((await documentConfig(page)).customModifiers[0].modifierName).toBe("SPACE_MOD");
});

test("share URL loading, Gist upload and publishing UI use the whole document", async ({ page }) => {
  await page.context().grantPermissions(["clipboard-read", "clipboard-write"]);
  await importConfig(page);
  await page.route("https://api.github.com/gists", async (route) => {
    const body = route.request().postDataJSON();
    expect(JSON.parse(body.files["config.json"].content).platformOverrides).toEqual(config.platformOverrides);
    await route.fulfill({ json: { files: { "config.json": { raw_url: "https://gist.githubusercontent.com/example/raw/config.json" } },
      html_url: "https://gist.github.com/example" } });
  });
  await page.getByRole("button", { name: "macOS", exact: true }).click();
  await page.getByRole("button", { name: "Share", exact: true }).click();
  await page.getByRole("button", { name: "Upload to Gist" }).click();
  await expect(page.getByText("Paste a GitHub personal access token")).toBeVisible();
  await page.getByLabel("GitHub token (gist scope)").fill("test-token-not-a-credential");
  await page.getByRole("button", { name: "Upload to Gist" }).click();
  await expect(page.getByRole("link", { name: "View on Gist" })).toBeVisible();
  await page.getByRole("button", { name: "Copy raw URL" }).click();
  expect(await page.evaluate(() => navigator.clipboard.readText())).toBe("https://gist.githubusercontent.com/example/raw/config.json");
  await page.getByRole("button", { name: "Done", exact: true }).click();
  await page.getByRole("button", { name: "Share", exact: true }).click();
  await page.getByRole("tab", { name: "Load URL" }).click();
  await page.getByLabel("Raw URL", { exact: true }).fill("https://example.com/config.json");
  await page.getByRole("button", { name: "Load", exact: true }).click();
  await expect(page.getByText(/Only.*raw|Unsupported|must.*raw/i).first()).toBeVisible();
  const url = "https://raw.githubusercontent.com/example/layout/main/config.json";
  await page.route(url, (route) => route.fulfill({ json: { ...config, name: "URL layout" } }));
  await page.getByLabel("Raw URL", { exact: true }).fill(url);
  page.once("dialog", (dialog) => dialog.accept());
  await page.getByRole("button", { name: "Load", exact: true }).click();
  await expect(page.getByLabel("Layout name", { exact: true })).toHaveValue("URL layout");
  await page.getByRole("button", { name: "Share", exact: true }).click();
  await page.getByRole("tab", { name: "Load URL" }).click();
  await expect(page.getByRole("button", { name: url, exact: true })).toBeVisible();
  await page.getByRole("tab", { name: "Publish PR" }).click();
  await page.getByLabel("Description", { exact: true }).fill("A test layout");
  await page.getByLabel("Author (GitHub handle)").fill("example");
  await page.getByLabel("Author URL", { exact: true }).fill("https://github.com/example");
  await page.getByLabel("Raw URL (the layout's config.json)").fill(url);
  await page.getByRole("button", { name: "Build snippet" }).click();
  await expect(page.getByRole("link", { name: "Open editor" })).toHaveAttribute("href", /community-layouts.ts/);
  await expect(page.getByRole("textbox").last()).toHaveValue(/URL layout/);
  await page.getByRole("button", { name: "Copy", exact: true }).click();
  expect(await page.evaluate(() => navigator.clipboard.readText())).toContain("URL layout");
});

test("community layouts import the maintained bundled config", async ({ page }) => {
  const source = await readFile(path.resolve(__dirname, "../../src/config.json"), "utf8");
  await page.route("https://raw.githubusercontent.com/eduuh/keyflow/main/src/config.json",
    (route) => route.fulfill({ contentType: "application/json", body: source }));
  page.once("dialog", (dialog) => dialog.accept());
  await page.getByRole("button", { name: "Layouts", exact: true }).click();
  await page.getByRole("dialog").getByRole("button", { name: "Load", exact: true }).first().click();
  await expect(page.getByLabel("Layout name", { exact: true })).toHaveValue("Eduuh-DH");
  expect((await documentConfig(page)).platformOverrides).toBeDefined();
});

test("visual workspace stays usable on mobile and in both themes", async ({ page }, testInfo) => {
  await importConfig(page);
  await page.setViewportSize({ width: 390, height: 844 });
  const errors: string[] = [];
  page.on("pageerror", (error) => errors.push(error.message));
  for (const theme of ["light", "dark"]) {
    if (await page.locator("html").evaluate((element) => element.classList.contains("dark")) !== (theme === "dark")) {
      await page.getByRole("button", { name: "Toggle theme" }).click();
    }
    await expect(page.locator("html")).toHaveClass(theme === "dark" ? /dark/ : /^(?!.*dark)/);
    expect(await page.evaluate(() => document.documentElement.scrollWidth <= window.innerWidth)).toBe(true);
    await page.getByRole("button", { name: "Import", exact: true }).focus();
    await expect(page.getByRole("button", { name: "Import", exact: true })).toBeFocused();
    await page.screenshot({ path: testInfo.outputPath(`mobile-${theme}.png`), fullPage: true });
  }
  await page.setViewportSize({ width: 1440, height: 1000 });
  await page.screenshot({ path: testInfo.outputPath("desktop.png"), fullPage: true });
  expect(errors).toEqual([]);
});

test("onboarding tour covers the environment and can be completed", async ({ page }) => {
  await page.getByRole("button", { name: "Take a tour" }).click();
  await expect(page.locator(".driver-popover-title")).toContainText("Welcome");
  await page.getByRole("button", { name: /^Next/ }).click();
  await expect(page.locator(".driver-popover-title")).toContainText("environment");
  for (let index = 0; index < 7; index++) await page.getByRole("button", { name: /^Next/ }).click();
  await page.getByRole("button", { name: /^Done/ }).click();
  expect(await page.evaluate(() => localStorage.getItem("keyflow-designer-tour-seen"))).toBe("true");
});
