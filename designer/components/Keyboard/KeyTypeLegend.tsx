"use client";

import { KEY_TYPE_STYLES, KeyType } from "@/lib/keyTypes";
import { cn } from "@/lib/utils";

// Color-coding legend shown below the keyboard. Tells users which color
// means what category. Categories shown in the order users most commonly
// scan for (modifiers first — they're the ergonomic foundation).

const ORDER: KeyType[] = [
  "modifier",
  "letter",
  "number",
  "symbol",
  "navigation",
  "function",
  "other",
];

export function KeyTypeLegend() {
  return (
    <div className="flex flex-wrap items-center gap-x-3 gap-y-1 text-xs">
      {ORDER.map((type) => (
        <div key={type} className="inline-flex items-center gap-1.5">
          <span className={cn("h-2.5 w-2.5 rounded-sm", KEY_TYPE_STYLES[type].dot)} />
          <span className="text-muted-foreground">{KEY_TYPE_STYLES[type].label}</span>
        </div>
      ))}
    </div>
  );
}
