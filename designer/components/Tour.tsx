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
        "Visual editor for your Keyflow config.json. The actual remapping happens in keyflow.exe — this is where you design what it does. Save your config as JSON, share it as a PNG, or grab the binary.",
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
        "Every key is colored by what it produces — mocha for modifiers, cream for letters & numbers, pink for navigation, burgundy for function keys. The legend below the keyboard explains the palette. Click any key to start mapping it.",
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
      title: "Get keyflow.exe",
      description:
        "Designing here is half the work — to actually use your config, install keyflow.exe (and the Interception driver). Latest stable release is one click away.",
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
