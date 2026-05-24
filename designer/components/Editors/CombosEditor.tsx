"use client";

import { Plus, Trash2 } from "lucide-react";
import { useConfigStore } from "@/lib/store";
import { cn } from "@/lib/utils";
import { CollapsiblePanel } from "./CollapsiblePanel";
import { KeySelect } from "./KeySelect";

// Editor for `noModCombos` — fires only when no modifier is held. The C++
// side blocks the combo from firing if any modifier bit is set
// (blockedModifiers = 0xFFFFFFFF), so e.g. `1 → !` only happens for a bare
// "1" press, not Shift+1. UI rules mirror this implicit-no-modifier behavior.
//
// Each combo: { key, output, shift?, description? }
//   - key:    the physical key the user presses
//   - output: the scancode that gets emitted
//   - shift:  if true, also inject Shift (so "1" → "!" uses output:1 + shift)
//   - description: free-form note ("1 → !")

export function CombosEditor() {
  const { config, setNoModCombo, removeNoModCombo } = useConfigStore();
  const combos = config.noModCombos ?? [];

  const handleAdd = () => {
    // Pick a key that isn't already a combo source so we don't shadow
    // an existing entry by mistake.
    const used = new Set(combos.map((c) => c.key));
    const allCandidates = ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"];
    const next = allCandidates.find((k) => !used.has(k)) ?? "Grave";
    setNoModCombo({ key: next, output: next, shift: false, description: "" });
  };

  return (
    <CollapsiblePanel title="No-modifier combos" count={combos.length}>
      <div className="space-y-2">
        {combos.length === 0 && (
          <p className="text-xs text-muted-foreground italic font-mono px-1">
            No combos defined. Use these to remap the number row to symbols (e.g. 2 → [)
            without holding Shift.
          </p>
        )}

        {combos.map((combo, i) => (
          <ComboRow
            key={`${combo.key}-${i}`}
            comboKey={combo.key}
            output={combo.output}
            shift={combo.shift ?? false}
            description={combo.description ?? ""}
            onChange={(updated) => setNoModCombo(updated)}
            onDelete={() => removeNoModCombo(combo.key)}
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
          Add combo
        </button>
      </div>
    </CollapsiblePanel>
  );
}

type ComboRowProps = {
  comboKey: string;
  output: string;
  shift: boolean;
  description: string;
  onChange: (updated: {
    key: string;
    output: string;
    shift: boolean;
    description: string;
  }) => void;
  onDelete: () => void;
};

function ComboRow({
  comboKey,
  output,
  shift,
  description,
  onChange,
  onDelete,
}: ComboRowProps) {
  const update = (patch: Partial<ComboRowProps>) =>
    onChange({
      key: patch.comboKey ?? comboKey,
      output: patch.output ?? output,
      shift: patch.shift ?? shift,
      description: patch.description ?? description,
    });

  return (
    <div className="flex items-center gap-2 px-2 py-1.5 rounded-md bg-background border border-border">
      {/* Source */}
      <KeySelect
        value={comboKey}
        onChange={(v) => update({ comboKey: v })}
        className="min-w-[7rem]"
      />

      <span className="text-muted-foreground font-mono text-xs">→</span>

      {/* Shift prefix indicator + output */}
      <button
        type="button"
        onClick={() => update({ shift: !shift })}
        title="Inject Shift with output"
        className={cn(
          "h-8 px-2 rounded-md text-xs font-mono font-semibold uppercase tracking-wider transition-colors",
          shift
            ? "bg-primary/15 text-primary border border-primary/40"
            : "bg-muted/40 text-muted-foreground border border-border hover:bg-accent/40",
        )}
      >
        ⇧
      </button>

      <KeySelect
        value={output}
        onChange={(v) => update({ output: v })}
        className="min-w-[7rem]"
      />

      {/* Description (free-form note) */}
      <input
        type="text"
        value={description}
        onChange={(e) => update({ description: e.target.value })}
        placeholder="Description (optional)"
        className={cn(
          "flex-1 min-w-0 h-8 px-2 rounded-md bg-background border border-input",
          "text-xs",
          "focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring",
        )}
      />

      <button
        type="button"
        onClick={onDelete}
        className={cn(
          "h-8 w-8 inline-flex items-center justify-center rounded-md",
          "text-muted-foreground hover:text-destructive hover:bg-destructive/10 transition-colors",
        )}
        title="Remove combo"
        aria-label="Remove combo"
      >
        <Trash2 size={14} />
      </button>
    </div>
  );
}
