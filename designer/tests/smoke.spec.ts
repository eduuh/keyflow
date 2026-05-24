import { test, expect } from "@playwright/test";

// Smoke checks — page renders, key click works, layer add gated on modifier.

test("page loads with header and keyboard visible", async ({ page }) => {
  await page.goto("/");
  await expect(page.getByText(/Keyflow Designer/i)).toBeVisible();
  // The keyboard has 60+ key buttons (one per ANSI key).
  const keyButtons = page.locator("button").filter({ hasText: /^[A-Z]$/ });
  await expect(keyButtons.first()).toBeVisible();
});

test("clicking a key opens the picker with the right context", async ({ page }) => {
  await page.goto("/");
  // Click the A key specifically — scoped to the keyboard view so we don't
  // accidentally match BASE / Add layer / any other button containing "A".
  await page
    .locator('[data-tour="keyboard"] button')
    .filter({ hasText: /^A$/ })
    .first()
    .click();
  // The picker card should appear with "Editing".
  await expect(page.locator('[data-tour="picker"]').getByText(/editing/i)).toBeVisible();
});

test("add-layer is disabled when no BASE modifier remap exists", async ({ page }) => {
  // Reset state by clearing localStorage before navigating.
  await page.context().clearCookies();
  await page.addInitScript(() => {
    window.localStorage.removeItem("keyflow-config-storage");
  });
  await page.goto("/");
  const addLayer = page.getByRole("button", { name: /add layer/i });
  await expect(addLayer).toBeDisabled();
});
