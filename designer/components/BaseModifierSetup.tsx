"use client";

import { useState, useEffect } from "react";
import { Zap, X } from "lucide-react";
import { useConfigStore } from "@/lib/store";
import { Button } from "@/components/ui/button";

type Preset = {
  label: string;
  description: string;
  remapping: Record<string, string>;
};

const PRESETS: Preset[] = [
  {
    label: "CapsLock → Ctrl",
    description: "Classic ergonomic swap",
    remapping: { CapsLock: "LeftCtrl" },
  },
  {
    label: "CapsLock → Alt",
    description: "Use CapsLock for layers via LAlt",
    remapping: { CapsLock: "LeftAlt" },
  },
  {
    label: "CapsLock → Shift",
    description: "Frees real Shift for other uses",
    remapping: { CapsLock: "LeftShift" },
  },
];

// Slim onboarding strip — only shown when no modifier remap exists yet.
// Once the user picks a preset (or maps a modifier manually), this disappears
// for good. Dismissible via the × button.
export function BaseModifierSetup() {
  const { config, setConfig, hasBaseModifiers } = useConfigStore();
  const [mounted, setMounted] = useState(false);
  const [dismissed, setDismissed] = useState(false);

  useEffect(() => setMounted(true), []);
  if (!mounted) return null;
  if (hasBaseModifiers() || dismissed) return null;

  const apply = (preset: Preset) => {
    setConfig({
      ...config,
      remapping: { ...(config.remapping ?? {}), ...preset.remapping },
    });
  };

  return (
    <div className="flex items-center gap-3 flex-wrap px-4 py-2.5 rounded-md border border-primary/30 bg-primary/5">
      <Zap className="h-4 w-4 text-primary shrink-0" />
      <span className="text-sm font-medium">Map a modifier to unlock layers</span>
      <div className="flex flex-wrap gap-1.5 ml-auto">
        {PRESETS.map((p) => (
          <Button
            key={p.label}
            variant="outline"
            size="sm"
            onClick={() => apply(p)}
            title={p.description}
            className="h-7 text-xs"
          >
            {p.label}
          </Button>
        ))}
        <Button
          variant="ghost"
          size="sm"
          onClick={() => setDismissed(true)}
          className="h-7 w-7 p-0"
          aria-label="Dismiss"
        >
          <X className="h-3.5 w-3.5" />
        </Button>
      </div>
    </div>
  );
}
