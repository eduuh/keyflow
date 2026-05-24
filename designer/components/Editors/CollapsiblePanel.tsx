"use client";

import { useState, ReactNode } from "react";
import { ChevronDown } from "lucide-react";
import { cn } from "@/lib/utils";

// A small disclosure panel matching the chassis aesthetic: mono uppercase
// title strip with a count badge and chevron, expandable body. Used to host
// the combos and custom-modifier editors at the bottom of the page so they're
// available without cluttering the keyboard view by default.

type CollapsiblePanelProps = {
  title: string;
  count?: number;
  defaultOpen?: boolean;
  children: ReactNode;
};

export function CollapsiblePanel({
  title,
  count,
  defaultOpen = false,
  children,
}: CollapsiblePanelProps) {
  const [open, setOpen] = useState(defaultOpen);

  return (
    <div className="rounded-lg border border-border bg-card overflow-hidden">
      <button
        type="button"
        onClick={() => setOpen(!open)}
        aria-expanded={open}
        className={cn(
          "w-full flex items-center justify-between px-4 py-2 font-mono",
          "border-b border-transparent",
          "hover:bg-accent/30 transition-colors",
          open && "border-border bg-muted/30",
        )}
      >
        <div className="flex items-center gap-3 text-xs">
          <ChevronDown
            size={14}
            className={cn(
              "text-muted-foreground transition-transform",
              open && "rotate-180",
            )}
          />
          <span className="uppercase tracking-widest font-semibold">{title}</span>
          {typeof count === "number" && (
            <span className="text-muted-foreground">({count})</span>
          )}
        </div>
      </button>
      {open && <div className="p-4">{children}</div>}
    </div>
  );
}
