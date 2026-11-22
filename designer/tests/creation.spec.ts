import { test, expect, type Page } from "@playwright/test";
import { readFile } from "node:fs/promises";
import { createLayoutTemplate, LAYOUT_TEMPLATES } from "../lib/layoutTemplates";
import { normalizeConfig, resolveConfig, serializeConfig } from "../lib/config";

async function saveConfig(page: Page) {
  const [download] = await Promise.all([
    page.waitForEvent("download"),
    page.getByRole("button", { name: "Save config", exact: true }).click(),
  ]);
  return JSON.parse(await readFile((await download.path())!, "utf8"));
}

test("keyboard templates produce independent, runtime-supported positional configs", () => {
  const examples = {
    colemak: { E: "F", S: "R", P: "Semicolon", Semicolon: "O", N: "K" },
    "colemak-dh": { T: "B", H: "M", Z: "X", C: "D", B: "Z", M: "H" },
    dvorak: { Q: "Apostrophe", W: "Comma", Z: "Semicolon", Apostrophe: "Minus", Minus: "LeftBracket", Equals: "RightBracket" },
    workman: { W: "D", E: "R", H: "Y", C: "M", M: "L", Semicolon: "I" },
  } as const;
  for (const template of LAYOUT_TEMPLATES) {
    const config = createLayoutTemplate(template.id);
    expect(JSON.parse(serializeConfig(config))).toEqual(config);
    for (const platform of ["shared", "linux", "windows", "mac"] as const) {
      expect(() => normalizeConfig(resolveConfig(config, platform))).not.toThrow();
    }
    if (template.id in examples) {
      expect(config.remapping).toMatchObject(examples[template.id as keyof typeof examples]);
      expect(config.remapping?.CapsLock).toBeUndefined();
      expect(config.customModifiers).toBeUndefined();
    }
    if (template.id === "eduuh-dh") {
      const original = [...config.layers![0].triggers];
      config.layers![0].triggers.push("RSHIFT");
      expect(createLayoutTemplate(template.id).layers![0].triggers).toEqual(original);
    }
  }
});

test("new sessions are blank; template previews and cancellation do not replace the layout", async ({ page }) => {
  await page.goto("/");
  await expect(page.locator("[data-designer-ready]")).toBeVisible();
  await expect(page.locator("[data-layout-name]")).toHaveText("My Layout");
  await expect(page.getByRole("button", { name: "New layer", exact: true })).toBeEnabled();
  await page.getByRole("button", { name: "New layout", exact: true }).click();
  const dialog = page.getByRole("dialog", { name: "New layout", exact: true });
  await dialog.getByRole("radio", { name: /^Eduuh-DH/ }).check();
  await expect(dialog.getByLabel("Layout name")).toHaveValue("Eduuh-DH");
  await dialog.getByRole("button", { name: "Cancel", exact: true }).click();
  expect(await saveConfig(page)).toEqual({ version: "1.0", remapping: {}, name: "My Layout" });
});

test("creation applies a named template and scopes optional settings without flattening it", async ({ page }) => {
  await page.goto("/");
  await page.getByRole("button", { name: "New layout", exact: true }).click();
  const dialog = page.getByRole("dialog", { name: "New layout", exact: true });
  await dialog.getByRole("radio", { name: /^Dvorak/ }).check();
  await dialog.getByLabel("Layout name").fill("My Dvorak");
  await dialog.getByText("Optional settings", { exact: true }).click();
  await dialog.getByLabel("Start editing in").selectOption("linux");
  await dialog.getByRole("switch", { name: "Disable Caps Lock" }).check();
  await dialog.getByRole("button", { name: "Create layout" }).click();
  await expect(dialog).not.toBeVisible();
  await expect(page.getByRole("group", { name: "Editing environment" }).getByRole("button", { name: "Linux", exact: true })).toHaveAttribute("aria-pressed", "true");
  const config = await saveConfig(page);
  expect(config).toEqual({
    ...createLayoutTemplate("dvorak", "My Dvorak"),
    platformOverrides: { linux: { disableCapsLock: true } },
  });
  await page.reload();
  await expect(page.locator("[data-layout-name]")).toHaveText("My Dvorak");
});

test("Eduuh-DH is loaded explicitly and replacing an existing layout requires confirmation", async ({ page }) => {
  await page.goto("/");
  await page.getByRole("button", { name: "New layout", exact: true }).click();
  let dialog = page.getByRole("dialog", { name: "New layout", exact: true });
  await dialog.getByRole("radio", { name: /^Eduuh-DH/ }).check();
  await dialog.getByRole("button", { name: "Create layout" }).click();
  expect(await saveConfig(page)).toEqual(createLayoutTemplate("eduuh-dh"));
  await page.getByRole("button", { name: "New layout", exact: true }).click();
  dialog = page.getByRole("dialog", { name: "New layout", exact: true });
  page.once("dialog", (confirmation) => confirmation.dismiss());
  await dialog.getByRole("button", { name: "Create layout" }).click();
  await expect(dialog).toBeVisible();
  await dialog.getByRole("button", { name: "Cancel" }).click();
  expect(await saveConfig(page)).toEqual(createLayoutTemplate("eduuh-dh"));
});

test("New layer works on a blank layout and stays visible when tabs overflow on mobile", async ({ page }) => {
  await page.setViewportSize({ width: 390, height: 844 });
  await page.goto("/");
  const create = page.getByRole("button", { name: "New layer", exact: true });
  await create.click();
  const dialog = page.getByRole("dialog", { name: "New layer", exact: true });
  await dialog.getByLabel("Name", { exact: true }).fill("Navigation");
  await dialog.getByRole("button", { name: "Create layer" }).click();
  expect((await saveConfig(page)).layers).toEqual([{ name: "Navigation", triggers: ["RALT"], mappings: {} }]);
  const manyLayers = {
    customModifiers: [{ key: "Space", modifierName: "constructor" }],
    version: "1.0", layers: Array.from({ length: 12 }, (_, i) => ({ name: `Layer ${i + 1}`, triggers: ["RALT"], mappings: {} })),
  };
  const [chooser] = await Promise.all([page.waitForEvent("filechooser"), page.getByRole("button", { name: "Import", exact: true }).click()]);
  await chooser.setFiles({ name: "layers.json", mimeType: "application/json", buffer: Buffer.from(JSON.stringify(manyLayers)) });
  await expect(page.getByRole("button", { name: "Edit layer Layer 12", exact: true })).toBeAttached();
  const box = (await create.boundingBox())!;
  expect(box.x).toBeGreaterThanOrEqual(0);
  expect(box.x + box.width).toBeLessThanOrEqual(390);
  await create.click();
  await expect(dialog).toBeVisible();
  await dialog.getByRole("button", { name: "Add constructor (constructor)", exact: true }).click();
  await dialog.getByRole("button", { name: "Create layer", exact: true }).click();
  expect((await saveConfig(page)).layers.at(-1).triggers).toEqual(["RALT", "constructor"]);
});

test("saved JSON offers actionable platform setup without pretending macOS has a runtime", async ({ page }) => {
  await page.goto("/");
  await saveConfig(page);
  await page.getByRole("button", { name: "How to use your JSON" }).click();
  const dialog = page.getByRole("dialog", { name: "Use your config" });
  await expect(dialog.locator("code").first()).toHaveText('.\\keyflow.exe --validate "C:\\path\\your-layout.json"');
  await page.context().grantPermissions(["clipboard-read", "clipboard-write"]);
  await dialog.getByRole("button", { name: "Copy command", exact: true }).first().click();
  await expect(dialog.getByRole("button", { name: "Copied", exact: true })).toHaveCount(1);
  await dialog.getByRole("button", { name: "Linux", exact: true }).click();
  await expect(dialog.getByRole("button", { name: "Copy command", exact: true })).toHaveCount(2);
  await expect(dialog.locator("code").first()).toHaveText('./keyflow --validate "/path/to/your-layout.json"');
  await expect(dialog.locator("code").nth(1)).toContainText("--device");
  await dialog.getByRole("button", { name: "macOS", exact: true }).click();
  await expect(dialog.getByRole("status")).toContainText("no macOS remapping runtime");
  await expect(dialog.locator("code")).toHaveCount(0);
});

test("key legends keep readable references without oversized primary characters", async ({ page }, testInfo) => {
  await page.goto("/");
  await page.evaluate(() => document.fonts.ready);
  const sizes = await page.locator('[data-key="Q"]').evaluate((key) => {
    const labels = key.querySelectorAll(":scope > span");
    return {
      reference: parseFloat(getComputedStyle(labels[0]).fontSize),
      primary: parseFloat(getComputedStyle(labels[1]).fontSize),
      referenceFont: getComputedStyle(labels[0]).fontFamily,
      primaryFont: getComputedStyle(labels[1]).fontFamily,
    };
  });
  expect(sizes.reference).toBeGreaterThanOrEqual(sizes.primary * 0.6);
  expect(sizes.primary).toBeGreaterThan(sizes.reference);
  expect(sizes.primaryFont).toBe(sizes.referenceFont);
  await page.screenshot({ path: testInfo.outputPath("designer-desktop.png"), fullPage: true });
  await page.setViewportSize({ width: 390, height: 844 });
  const modifierSizes = await page.locator('[data-key="LeftCtrl"]').evaluate((key) => {
    const labels = key.querySelectorAll(":scope > span");
    return [parseFloat(getComputedStyle(labels[0]).fontSize), parseFloat(getComputedStyle(labels[1]).fontSize)];
  });
  expect(modifierSizes[1]).toBeGreaterThanOrEqual(modifierSizes[0]);
  await page.screenshot({ path: testInfo.outputPath("designer-mobile.png"), fullPage: true });
});
