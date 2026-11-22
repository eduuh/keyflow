import { test, expect } from "@playwright/test";

// Smoke checks — page renders, key click works, physical modifiers need no setup.

test("page loads with header and keyboard visible", async ({ page }) => {
  await page.goto("/");
  await expect(page.getByRole("heading", { name: "Keyflow Designer" })).toBeVisible();
  // The keyboard has 60+ key buttons (one per ANSI key).
  const keyButtons = page.locator('[data-tour="keyboard"] [data-key]');
  await expect(keyButtons.first()).toBeVisible();
  await expect(page.locator('[data-tour="keyboard"] [data-key="A"]')).toHaveAccessibleName("A");
});

test("clicking a key opens the picker with the right context", async ({ page }) => {
  await page.goto("/");
  // Click the A key specifically — scoped to the keyboard view so we don't
  // accidentally match BASE / New layer / any other button containing "A".
  await page
    .locator('[data-tour="keyboard"] [data-key="A"]')
    .click();
  // The picker card should appear with "Editing".
  await expect(page.locator('[data-tour="picker"]').getByText(/editing/i)).toBeVisible();
});

test("new layer is enabled without BASE modifier remaps", async ({ page }) => {
  await page.addInitScript(() => {
    localStorage.setItem("keyflow-config-storage", JSON.stringify({
      version: 2, state: { config: { version: "1.0", remapping: {}, layers: [] } },
    }));
  });
  await page.goto("/");
  await expect(page.getByRole("button", { name: "New layer", exact: true })).toBeEnabled();
});

for (const colorScheme of ["light", "dark"] as const) {
  test(`fresh ${colorScheme} session has no automatic tour and one click changes the rendered theme`, async ({ page }) => {
    await page.emulateMedia({ colorScheme });
    await page.goto("/");
    await expect(page.locator("[data-designer-ready]")).toBeVisible();
    await expect.poll(() => page.evaluate(() => localStorage.getItem("theme"))).toBe(colorScheme);
    await expect(page.locator(".driver-popover, .driver-overlay")).toHaveCount(0);
    expect(await page.evaluate(() => localStorage.getItem("keyflow-designer-tour-seen"))).toBeNull();
    const background = () => page.locator("body").evaluate((element) => getComputedStyle(element).backgroundColor);
    const before = await background();
    await page.getByRole("button", { name: "Toggle theme" }).click();
    await expect.poll(background).not.toBe(before);
    const after = await background();
    expect(after).not.toBe("rgba(0, 0, 0, 0)");
    const selectedTheme = colorScheme === "light" ? "dark" : "light";
    await expect.poll(() => page.evaluate(() => localStorage.getItem("theme"))).toBe(selectedTheme);
    await page.reload();
    await expect(page.locator("[data-designer-ready]")).toBeVisible();
    await expect.poll(background).toBe(after);
    await expect(page.locator(".driver-popover, .driver-overlay")).toHaveCount(0);
    await page.getByRole("link", { name: "Settings", exact: true }).click();
    await expect(page.getByLabel("Layout name", { exact: true })).toBeVisible({ timeout: 15_000 });
    await expect(page.locator(".driver-popover, .driver-overlay")).toHaveCount(0);
    await expect.poll(background).toBe(after);
  });
}
