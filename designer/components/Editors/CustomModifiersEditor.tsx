"use client";

import { Plus, Trash2 } from "lucide-react";
import { useConfigStore } from "@/lib/store";
import { cn } from "@/lib/utils";
import { CollapsiblePanel } from "./CollapsiblePanel";
import { KeySelect } from "./KeySelect";

// Editor for `customModifiers` — turn any physical key into a layer trigger
// by giving it a name. The Space-Cadet config uses this: `Space` → "SPACE_MOD",
// then a layer's `triggers: ["SPACE_MOD"]` activates when Space is held.
//
// Each entry: { key, modifierName, blockOutput? }
//   - key:          the physical key being promoted to a modifier
//   - modifierName: arbitrary uppercase identifier (e.g. SPACE_MOD)
//   - blockOutput:  if true, suppress the key's normal output while held
//                   (recommended for Space — otherwise typing while in the
//                   layer also inserts spaces)
//
// The C++ side assigns each customModifier a bit (Custom1..Custom23) in
// declaration order. The designer doesn't need to expose those bits;
// just preserve the order.

export function CustomModifiersEditor() {
  const { config, setCustomModifier, removeCustomModifier } = useConfigStore();
  const mods = config.customModifiers ?? [];

  const handleAdd = () => {
    const used = new Set(mods.map((m) => m.modifierName));
    const candidates = ["SPACE_MOD", "TAB_MOD", "CAPS_MOD", "ENTER_MOD"];
    const name = candidates.find((c) => !used.has(c)) ?? `MOD_${mods.length + 1}`;
    setCustomModifier({ key: "Space", modifierName: name, blockOutput: true });
  };

  return (
    <CollapsiblePanel title="Custom modifiers" count={mods.length}>
      <div className="space-y-2">
        {mods.length === 0 && (
          <p className="text-xs text-muted-foreground italic font-mono px-1">
            No custom modifiers defined. Add one if you want a non-modifier key (e.g. Space, Tab)
            to act as a layer trigger.
          </p>
        )}

        {mods.map((mod, i) => (
          <ModRow
            key={`${mod.modifierName}-${i}`}
            modKey={mod.key}
            modifierName={mod.modifierName}
            blockOutput={mod.blockOutput ?? true}
            onChange={(updated) => setCustomModifier(updated)}
            onDelete={() => removeCustomModifier(mod.modifierName)}
          />
        ))}

        <button
          type="button"
          onClick={handleAdd}
          className={cn(
            "inline-flex items-center gap-1.5 h-8 px-3 rounded-md text-[11px] uppercase tracking-wider font-mono font-semibold",
            "border border-dashed border-border text-muted-foreground",
            "hover:border-primary/60 hover:text-foreground hover:bg-accent/40 transition-colors",
          )}
        >
          <Plus size={12} />
          Add custom modifier
        </button>
      </div>
    </CollapsiblePanel>
  );
}

type ModRowProps = {
  modKey: string;
  modifierName: string;
  blockOutput: boolean;
  onChange: (updated: {
    key: string;
    modifierName: string;
    blockOutput: boolean;
  }) => void;
  onDelete: () => void;
};

function ModRow({ modKey, modifierName, blockOutput, onChange, onDelete }: ModRowProps) {
  const update = (patch: Partial<ModRowProps>) =>
    onChange({
      key: patch.modKey ?? modKey,
      modifierName: patch.modifierName ?? modifierName,
      blockOutput: patch.blockOutput ?? blockOutput,
    });

  return (
    <div className="flex items-center gap-2 px-2 py-1.5 rounded-md bg-background border border-border">
      <KeySelect
        value={modKey}
        onChange={(v) => update({ modKey: v })}
        className="min-w-[7rem]"
      />

      <span className="text-muted-foreground font-mono text-xs">→</span>

      <input
        type="text"
        value={modifierName}
        onChange={(e) =>
          update({
            modifierName: e.target.value.toUpperCase().replace(/[^A-Z0-9_]/g, "_"),
          })
        }
        placeholder="MOD_NAME"
        className={cn(
          "h-8 px-2 rounded-md bg-background border border-input",
          "text-xs font-mono font-semibold uppercase tracking-wider",
          "focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring",
          "min-w-[8rem]",
        )}
      />

      <button
        type="button"
        onClick={() => update({ blockOutput: !blockOutput })}
        title={
          blockOutput
            ? "Output blocked while held (recommended)"
            : "Output passes through while held"
        }
        className={cn(
          "h-8 px-2.5 rounded-md text-[10px] font-mono font-semibold uppercase tracking-wider transition-colors",
          blockOutput
            ? "bg-primary/15 text-primary border border-primary/40"
            : "bg-muted/40 text-muted-foreground border border-border hover:bg-accent/40",
        )}
      >
        Block output
      </button>

      <div className="flex-1" />

      <button
        type="button"
        onClick={onDelete}
        className={cn(
          "h-8 w-8 inline-flex items-center justify-center rounded-md",
          "text-muted-foreground hover:text-destructive hover:bg-destructive/10 transition-colors",
        )}
        title="Remove custom modifier"
        aria-label="Remove custom modifier"
      >
        <Trash2 size={14} />
      </button>
    </div>
  );
}
