"use client";

import { useState } from "react";
import { Trash2, X, Check, Plus } from "lucide-react";
import { useConfigStore } from "@/lib/store";
import { cn } from "@/lib/utils";

// Multi-trigger layer editor. The C++ schema supports a layer with multiple
// trigger modifiers (OR-joined — any one activates the layer). UI shows the
// current set of triggers as chips; users can add/remove individually.
//
// Validation rules (match the C++ side):
//   - At least one trigger required
//   - Each trigger must be a modifier currently mapped in the BASE layer
//   - The same modifier can't trigger two different layers (would be ambiguous)

type LayerEditorProps = {
  layerIndex: number;
  onClose: () => void;
};

const MODIFIER_LABELS: Record<string, string> = {
  RALT: "Right Alt",
  LALT: "Left Alt",
  RCTRL: "Right Ctrl",
  LCTRL: "Left Ctrl",
  RSHIFT: "Right Shift",
  LSHIFT: "Left Shift",
  RWIN: "Right Win",
  LWIN: "Left Win",
  CAPS: "Caps Lock",
};

const MODIFIER_KEYS = Object.keys(MODIFIER_LABELS);

export function LayerEditor({ layerIndex, onClose }: LayerEditorProps) {
  const { config, updateLayer, deleteLayer } = useConfigStore();
  const layer = config.layers?.[layerIndex];

  const [name, setName] = useState(layer?.name || "");
  const [triggers, setTriggers] = useState<string[]>(layer?.triggers ?? []);

  if (!layer) return null;

  // Modifiers available in this config (must be the *target* of a BASE remap).
  const baseRemapping = config.remapping || {};
  const availableModifiers = Array.from(
    new Set(Object.values(baseRemapping).filter((t) => MODIFIER_KEYS.includes(t))),
  );

  // Triggers already claimed by *other* layers — can't reuse without ambiguity.
  const layers = config.layers || [];
  const usedByOthers = new Set<string>();
  layers.forEach((l, i) => {
    if (i === layerIndex) return;
    (l.triggers || []).forEach((t) => usedByOthers.add(t));
  });

  // What can we still add to this layer's trigger set?
  const addable = availableModifiers.filter(
    (m) => !triggers.includes(m) && !usedByOthers.has(m),
  );

  const handleAddTrigger = (mod: string) => setTriggers([...triggers, mod]);
  const handleRemoveTrigger = (mod: string) =>
    setTriggers(triggers.filter((t) => t !== mod));

  const handleSave = () => {
    if (triggers.length === 0) {
      alert("Layer needs at least one trigger modifier.");
      return;
    }
    updateLayer(layerIndex, { name, triggers });
    onClose();
  };

  const handleDelete = () => {
    if (confirm(`Delete "${layer.name}"? All mappings will be lost.`)) {
      deleteLayer(layerIndex);
      onClose();
    }
  };

  return (
    <div
      className="fixed inset-0 z-50 flex items-center justify-center bg-black/70 backdrop-blur-sm p-4"
      onClick={onClose}
    >
      <div
        className="relative w-full max-w-md rounded-lg border border-border bg-card p-5 shadow-chassis font-mono"
        onClick={(e) => e.stopPropagation()}
      >
        <div className="mb-4 flex items-center justify-between">
          <h2 className="text-sm font-semibold uppercase tracking-wider">
            Edit Layer
          </h2>
          <button
            onClick={onClose}
            className="inline-flex items-center justify-center h-7 w-7 rounded-md hover:bg-accent"
            aria-label="Close"
          >
            <X size={14} />
          </button>
        </div>

        <div className="space-y-4">
          {/* Layer Name */}
          <div>
            <label className="text-[11px] font-semibold uppercase tracking-wider text-muted-foreground mb-1.5 block">
              Name
            </label>
            <input
              type="text"
              value={name}
              onChange={(e) => setName(e.target.value)}
              placeholder="e.g., Numpad, Arrows & Symbols"
              className={cn(
                "w-full h-9 px-3 rounded-md bg-background border border-input",
                "text-sm font-sans",
                "focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring",
              )}
            />
          </div>

          {/* Triggers */}
          <div>
            <label className="text-[11px] font-semibold uppercase tracking-wider text-muted-foreground mb-1.5 block">
              Triggers (any one activates)
            </label>

            <div className="flex flex-wrap gap-1.5 min-h-[2.25rem] p-1.5 rounded-md bg-background border border-input">
              {triggers.length === 0 && (
                <span className="px-2 py-1 text-xs text-muted-foreground italic">
                  No triggers — add at least one below
                </span>
              )}
              {triggers.map((t) => (
                <span
                  key={t}
                  className={cn(
                    "inline-flex items-center gap-1 h-7 px-2 rounded text-xs font-semibold uppercase tracking-wider",
                    "bg-primary/15 text-primary border border-primary/40",
                  )}
                >
                  {t}
                  <button
                    onClick={() => handleRemoveTrigger(t)}
                    className="hover:text-foreground"
                    aria-label={`Remove ${t}`}
                  >
                    <X size={12} />
                  </button>
                </span>
              ))}
            </div>

            {/* Add menu */}
            {addable.length > 0 && (
              <div className="mt-2 flex flex-wrap gap-1.5">
                {addable.map((mod) => (
                  <button
                    key={mod}
                    onClick={() => handleAddTrigger(mod)}
                    className={cn(
                      "inline-flex items-center gap-1 h-7 px-2 rounded text-xs font-semibold uppercase tracking-wider",
                      "border border-dashed border-border text-muted-foreground",
                      "hover:border-primary/60 hover:text-foreground hover:bg-accent/40 transition-colors",
                    )}
                  >
                    <Plus size={11} />
                    {mod}
                  </button>
                ))}
              </div>
            )}

            {/* Help */}
            {availableModifiers.length === 0 ? (
              <p className="text-xs text-destructive mt-2">
                ⚠ Map a modifier in BASE first — layers need a trigger.
              </p>
            ) : addable.length === 0 && triggers.length === 0 ? (
              <p className="text-xs text-destructive mt-2">
                ⚠ All BASE modifiers are claimed by other layers.
              </p>
            ) : (
              <p className="text-[10px] text-muted-foreground mt-2 font-sans">
                Only modifiers mapped in BASE (and not already used by another layer) appear above.
              </p>
            )}
          </div>

          {/* Stats */}
          <div className="rounded-md bg-muted/40 border border-border p-3 text-xs">
            <div className="flex items-center justify-between">
              <span className="text-muted-foreground uppercase tracking-wider text-[10px]">
                Mapped keys
              </span>
              <span className="font-semibold">
                {Object.keys(layer.mappings || {}).length}
              </span>
            </div>
            <div className="flex items-center justify-between mt-1">
              <span className="text-muted-foreground uppercase tracking-wider text-[10px]">
                Shift mappings
              </span>
              <span className="font-semibold">{(layer.shiftMappings || []).length}</span>
            </div>
          </div>
        </div>

        {/* Actions */}
        <div className="mt-5 flex items-center justify-between">
          <button
            onClick={handleDelete}
            className={cn(
              "inline-flex items-center gap-2 h-9 px-3 rounded-md text-sm font-semibold uppercase tracking-wider",
              "border border-destructive/60 text-destructive",
              "hover:bg-destructive hover:text-destructive-foreground transition-colors",
            )}
          >
            <Trash2 size={14} />
            Delete
          </button>
          <div className="flex items-center gap-2">
            <button
              onClick={onClose}
              className={cn(
                "inline-flex items-center h-9 px-3 rounded-md text-sm font-semibold uppercase tracking-wider",
                "border border-input bg-background hover:bg-accent",
              )}
            >
              Cancel
            </button>
            <button
              onClick={handleSave}
              className={cn(
                "inline-flex items-center gap-2 h-9 px-3 rounded-md text-sm font-semibold uppercase tracking-wider",
                "bg-primary text-primary-foreground hover:bg-primary/90 transition-colors",
                "shadow-[0_0_12px_hsl(var(--primary)/0.3)]",
              )}
            >
              <Check size={14} />
              Save
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}
