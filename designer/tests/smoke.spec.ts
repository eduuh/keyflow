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
  // Pick the first letter button — the A key on the keyboard.
  await page.locator('button:has-text("A")').first().click();
  // The picker card should show "Editing A".
  await expect(page.getByText(/Editing/i)).toBeVisible();
  await expect(page.locator(".font-mono", { hasText: /^A$/ }).first()).toBeVisible();
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
