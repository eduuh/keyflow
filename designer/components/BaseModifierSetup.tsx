"use client";

import { useEffect, useState } from "react";
import { useConfigStore } from "@/lib/store";
import { cn } from "@/lib/utils";
import { Zap } from "lucide-react";

export function BaseModifierSetup() {
  const { config, setConfig, hasBaseModifiers } = useConfigStore();
  const [mounted, setMounted] = useState(false);

  useEffect(() => {
    setMounted(true);
  }, []);

  // Prevent hydration mismatch by only rendering after mount
  if (!mounted) return null;

  if (hasBaseModifiers()) return null;

  const quickSetups = [
    {
      name: "CapsLock → Ctrl",
      description: "Most popular for programmers",
      remapping: { CapsLock: "LCTRL" },
    },
    {
      name: "CapsLock → Alt",
      description: "Good for symbol layers",
      remapping: { CapsLock: "RALT" },
    },
    {
      name: "Both",
      description: "CapsLock → Ctrl, RightAlt → Alt",
      remapping: { CapsLock: "LCTRL", RightAlt: "RALT" },
    },
  ];

  const handleQuickSetup = (remapping: Record<string, string>) => {
    setConfig({
      ...config,
      remapping: {
        ...config.remapping,
        ...remapping,
      },
    });
  };

  return (
    <div className="rounded-lg border-2 border-primary/30 bg-primary/5 p-4 shadow-sm">
      <div className="flex items-start gap-3">
        <div className="flex-shrink-0 mt-0.5">
          <Zap className="h-5 w-5 text-primary" />
        </div>
        <div className="flex-1">
          <h3 className="text-sm font-semibold text-foreground mb-1">
            Quick Start: Set Up Modifiers
          </h3>
          <p className="text-xs text-muted-foreground mb-3">
            Map at least one modifier in BASE layer to enable layers. Choose a quick setup or manually map keys.
          </p>
          <div className="flex flex-wrap gap-2">
            {quickSetups.map((setup) => (
              <button
                key={setup.name}
                onClick={() => handleQuickSetup(setup.remapping)}
                className={cn(
                  "inline-flex flex-col items-start rounded-md text-left transition-colors",
                  "border border-input bg-background hover:bg-accent px-3 py-2 text-xs"
                )}
              >
                <div className="font-semibold">{setup.name}</div>
                <div className="text-muted-foreground text-[10px]">
                  {setup.description}
                </div>
              </button>
            ))}
          </div>
          <p className="text-[10px] text-muted-foreground mt-2">
            Or click any modifier key on the keyboard to map it manually
          </p>
        </div>
      </div>
    </div>
  );
}
