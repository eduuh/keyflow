"use client";

import { driver } from "driver.js";
// driver.css imported via app/globals.css (@import) — Turbopack's
// node_modules CSS resolution doesn't always read the exports field.

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
      description: "Choose Shared or a platform before remapping. The keyboard always shows the environment you are editing. Presets and inheritance controls live on the Settings page.",
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
    element: '[data-tour="save-config"]',
    popover: {
      title: "Save your config",
      description: "Save config downloads your layout as JSON, keeping Shared and every platform override together. More contains JSON editing, images, sharing and community layouts.",
    },
  },
  {
    element: '[data-tour="settings-link"]',
    popover: {
      title: "Settings, when you need them",
      description: "Change the layout name, Caps Lock, platform presets, inheritance, custom modifiers and combos on the Settings page. Return to Remapping to edit the keyboard.",
    },
  },
];

const SETTINGS_STEPS = [
  { element: '[data-tour="environment-options"]', popover: {
    title: "Layout settings", description: "Edit your layout name and Caps Lock behavior. Choose Shared or a platform to keep settings in the correct environment.",
  } },
  { element: '[data-tour="advanced"]', popover: {
    title: "Modifiers and combos", description: "Set up layer triggers and no-modifier combos here. Changes apply to the selected environment.",
  } },
  { element: '[data-tour="save-config"]', popover: {
    title: "Save your config", description: "Download the complete portable document. Your settings also stay saved in this browser.",
  } },
  { element: '[data-tour="settings-link"]', popover: {
    title: "Back to remapping", description: "Return to the keyboard to edit key outputs and layers. The selected environment and your edits stay with you.",
  } },
];

export function startTour(settings = false) {
  const tour = driver({
    showProgress: true,
    showButtons: ["next", "previous", "close"],
    steps: (settings ? SETTINGS_STEPS : STEPS).map((s) => ({
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
