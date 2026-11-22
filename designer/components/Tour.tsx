"use client";

import { useEffect } from "react";
import { driver } from "driver.js";
// driver.css imported via app/globals.css (@import) — Turbopack's
// node_modules CSS resolution doesn't always read the exports field.

// First-visit onboarding tour. Walks through the main UI concepts in
// 8 steps. Driver.js handles overlay rendering, focus management, and
// keyboard navigation; we just define the steps and target selectors.
//
// Storage contract: localStorage["keyflow-designer-tour-seen"] === "true"
// after the user finishes or dismisses. Returning visitors don't see the
// tour unless they click "Take a tour" in the header (startTour()).

const STORAGE_KEY = "keyflow-designer-tour-seen";

const STEPS = [
  {
    element: '[data-tour="header"]',
    popover: {
      title: "Welcome to Keyflow Designer",
      description:
        "Visual editor for your Keyflow config.json. Design here, then download JSON to use with Keyflow. Shared settings and Linux, Windows and macOS overrides travel in one document.",
    },
  },
  {
    element: '[data-tour="environment"]',
    popover: {
      title: "One layout, every environment",
      description: "Choose Shared to edit the portable base, or a platform to preview and edit its overrides. Remaps merge; array edits replace that section. Use Shared restores inheritance.",
    },
  },
  {
    element: '[data-tour="layer-tabs"]',
    popover: {
      title: "BASE + Layers",
      description:
        "BASE is the remap that's always active (e.g. CapsLock → LeftShift). Layers activate when you hold a modifier key. Each layer can have its own mappings; press the modifier and you swap into that layer's behavior.",
    },
  },
  {
    element: '[data-tour="keyboard"]',
    popover: {
      title: "Color-coded keyboard",
      description:
        "The small legend identifies the physical key; the large legend shows its output. Colored edges show layer bindings. Select a key to edit it, or choose function, navigation, numpad and media keys below the canvas.",
    },
  },
  {
    element: '[data-tour="picker"]',
    popover: {
      title: "Mapping a key",
      description:
        "Click a key on the board, then pick a target from this panel. Tabs across the top filter by category (A-Z, numbers, symbols, etc.). The current binding is shown at the top — change it by clicking a different target, or clear it with the trash button.",
    },
  },
  {
    element: '[data-tour="picker-shift"]',
    popover: {
      title: "Shift mappings (layers only)",
      description:
        "Inside a layer, the 'Shift+' tab is enabled. Picking a target there creates a 'shift mapping' — pressing the key in the layer also injects Shift. Useful for symbols (RAlt+T → ^) without holding two modifiers manually.",
    },
  },
  {
    element: '[data-tour="advanced"]',
    popover: {
      title: "Advanced settings",
      description:
        "Expand to find two power-user features: 'no-modifier combos' (number row → symbol without Shift, like 2→[), and 'custom modifiers' (any key promoted to a layer trigger, like Space → SPACE_MOD). Hidden by default to keep the main view clean.",
    },
  },
  {
    element: '[data-tour="export-image"]',
    popover: {
      title: "Export as PNG",
      description:
        "Generate cheat-sheet images of your layout. Each category (Letters / Numbers / Symbols / Directional / Functional / Return / Layout) highlights different keys. Download one at a time, or all categories as a zip.",
    },
  },
  {
    element: '[data-tour="download-binary"]',
    popover: {
      title: "Get Keyflow",
      description:
        "Install the latest Keyflow release for your operating system, then load your exported config.json.",
    },
  },
];

export function startTour() {
  const tour = driver({
    showProgress: true,
    showButtons: ["next", "previous", "close"],
    steps: STEPS.map((s) => ({
      element: s.element,
      popover: {
        ...s.popover,
        side: "bottom",
        align: "start",
      },
    })),
    onDestroyed: () => {
      try {
        localStorage.setItem(STORAGE_KEY, "true");
      } catch {
        // localStorage can throw in private mode / quota
      }
    },
  });
  tour.drive();
}

export function TourAutoStart() {
  useEffect(() => {
    let seen = false;
    try {
      seen = localStorage.getItem(STORAGE_KEY) === "true";
    } catch {
      seen = false;
    }
    if (seen) return;
    // Small delay so the page settles before the tour overlays appear.
    const t = setTimeout(() => startTour(), 500);
    return () => clearTimeout(t);
  }, []);
  return null;
}
