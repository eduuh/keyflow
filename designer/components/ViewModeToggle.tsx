"use client";

import { useConfigStore } from "@/lib/store";
import { cn } from "@/lib/utils";

export function ViewModeToggle() {
  const { config, setConfig } = useConfigStore();

  return (
    <div className="flex items-center justify-end">
      {/* Strict Mode Toggle */}
      <div className="flex items-center gap-2">
        <span className="text-sm font-medium text-muted-foreground">Strict Mode:</span>
        <button
          role="switch"
          aria-checked={config.strictMode || false}
          onClick={() => {
            setConfig({ ...config, strictMode: !config.strictMode });
          }}
          className={cn(
            "peer inline-flex h-5 w-9 shrink-0 cursor-pointer items-center rounded-full border-2 border-transparent shadow-sm transition-colors focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2 focus-visible:ring-offset-background disabled:cursor-not-allowed disabled:opacity-50",
            config.strictMode
              ? "bg-primary"
              : "bg-input"
          )}
        >
          <span
            className={cn(
              "pointer-events-none block h-4 w-4 rounded-full bg-background shadow-lg ring-0 transition-transform",
              config.strictMode ? "translate-x-4" : "translate-x-0"
            )}
          />
        </button>
      </div>
    </div>
  );
}
