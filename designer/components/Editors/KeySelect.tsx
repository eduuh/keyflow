"use client";

import { cn } from "@/lib/utils";
import { KEY_GROUPS } from "@/lib/allKeys";

// Native <select> dressed up to match the hardware-y aesthetic. Used by the
// combos and custom-modifiers editors so adding a row is fast and obvious —
// no modal, no key picker overlay.
//
// Native <select> isn't pretty by default but it's accessible (keyboard
// navigation, type-ahead, screen-reader friendly) and works on every browser.
// Wrapping it ourselves lets us style the trigger to match while keeping
// the OS-native dropdown for the option list.

type KeySelectProps = {
  value: string;
  onChange: (value: string) => void;
  placeholder?: string;
  className?: string;
};

export function KeySelect({ value, onChange, placeholder, className }: KeySelectProps) {
  return (
    <select
      value={value}
      onChange={(e) => onChange(e.target.value)}
      className={cn(
        "h-8 px-2 rounded-md bg-background border border-input",
        "text-xs font-mono font-semibold",
        "focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring",
        "appearance-none cursor-pointer",
        className,
      )}
    >
      {placeholder && (
        <option value="" disabled>
          {placeholder}
        </option>
      )}
      {KEY_GROUPS.map((group) => (
        <optgroup key={group.group} label={group.group}>
          {group.keys.map((k) => (
            <option key={k.code} value={k.code}>
              {k.label}
            </option>
          ))}
        </optgroup>
      ))}
    </select>
  );
}
