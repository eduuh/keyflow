"use client";

import { useState } from "react";
import { Edit2, Trash2, X, Check } from "lucide-react";
import { useConfigStore } from "@/lib/store";
import { cn } from "@/lib/utils";

type LayerEditorProps = {
  layerIndex: number;
  onClose: () => void;
};

export function LayerEditor({ layerIndex, onClose }: LayerEditorProps) {
  const { config, updateLayer, deleteLayer } = useConfigStore();
  const layer = config.layers?.[layerIndex];

  const [name, setName] = useState(layer?.name || "");
  const [trigger, setTrigger] = useState(
    Array.isArray(layer?.trigger) ? layer.trigger[0] : layer?.trigger || "RALT"
  );

  if (!layer) return null;

  const handleSave = () => {
    // Don't save if no modifiers are available
    if (triggerOptions.length === 0) {
      if (availableModifiers.length === 0) {
        alert("Cannot save layer: No modifiers mapped in BASE layer.");
      } else {
        alert("Cannot save layer: All available modifiers are already used by other layers.\n\nEach layer needs a unique trigger modifier.");
      }
      return;
    }

    // Check if trying to use a trigger that's in use by another layer
    const isCurrentTriggerAvailable = triggerOptions.some((opt) => opt.value === trigger);
    if (!isCurrentTriggerAvailable) {
      // Auto-select first available trigger
      setTrigger(triggerOptions[0].value);
      alert(`The selected trigger is now used by another layer. Changed to ${triggerOptions[0].label}.`);
      return;
    }

    updateLayer(layerIndex, { name, trigger });
    onClose();
  };

  const handleDelete = () => {
    if (confirm(`Delete "${layer.name}"? All mappings will be lost.`)) {
      deleteLayer(layerIndex);
      onClose();
    }
  };

  // Get available triggers from BASE layer remapping
  const baseRemapping = config.remapping || {};
  const availableModifiers = Object.entries(baseRemapping)
    .filter(([_, target]) =>
      ["RALT", "LALT", "RCTRL", "LCTRL", "RSHIFT", "LSHIFT", "RWIN", "LWIN", "CAPS"].includes(target)
    )
    .map(([source, target]) => ({
      source,
      target,
    }));

  // Get triggers already used by other layers
  const layers = config.layers || [];
  const usedTriggers = layers
    .map((l, idx) => {
      const layerTrigger = Array.isArray(l.trigger) ? l.trigger[0] : l.trigger;
      return { trigger: layerTrigger, layerIndex: idx, layerName: l.name };
    })
    .filter((t) => t.layerIndex !== layerIndex); // Exclude current layer

  // Map modifier names to labels
  const modifierLabels: Record<string, string> = {
    RALT: "Right Alt (RALT)",
    LALT: "Left Alt (LALT)",
    RCTRL: "Right Ctrl (RCTRL)",
    LCTRL: "Left Ctrl (LCTRL)",
    RSHIFT: "Right Shift (RSHIFT)",
    LSHIFT: "Left Shift (LSHIFT)",
    RWIN: "Right Win (RWIN)",
    LWIN: "Left Win (LWIN)",
    CAPS: "Caps Lock (CAPS)",
  };

  // Only show modifiers that are available in BASE and not used by other layers
  const triggerOptions = availableModifiers
    .map(({ source, target }) => {
      const usedBy = usedTriggers.find((t) => t.trigger === target);
      return {
        value: target,
        label: `${modifierLabels[target]} ← ${source}`,
        isUsed: !!usedBy,
        usedByLayer: usedBy?.layerName,
      };
    })
    .filter((opt) => !opt.isUsed); // Filter out already used triggers

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/50 backdrop-blur-sm">
      <div className="relative w-full max-w-md rounded-lg border bg-background p-6 shadow-lg">
        {/* Header */}
        <div className="mb-4 flex items-center justify-between">
          <h2 className="text-lg font-semibold">Edit Layer</h2>
          <button
            onClick={onClose}
            className="inline-flex items-center justify-center rounded-md text-sm font-medium hover:bg-accent h-8 w-8"
          >
            <X size={16} />
          </button>
        </div>

        {/* Form */}
        <div className="space-y-4">
          {/* Layer Name */}
          <div>
            <label className="text-sm font-medium mb-1.5 block">Layer Name</label>
            <input
              type="text"
              value={name}
              onChange={(e) => setName(e.target.value)}
              placeholder="e.g., Symbol Layer, Numpad Layer"
              className="flex h-9 w-full rounded-md border border-input bg-background px-3 py-1 text-sm shadow-sm transition-colors placeholder:text-muted-foreground focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
            />
          </div>

          {/* Trigger Key */}
          <div>
            <label className="text-sm font-medium mb-1.5 block">
              Trigger Modifier
            </label>
            {triggerOptions.length > 0 ? (
              <>
                <select
                  value={trigger}
                  onChange={(e) => setTrigger(e.target.value)}
                  className="flex h-9 w-full rounded-md border border-input bg-background px-3 py-1 text-sm shadow-sm transition-colors focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
                >
                  {triggerOptions.map((opt) => (
                    <option key={opt.value} value={opt.value}>
                      {opt.label}
                    </option>
                  ))}
                </select>
                <p className="text-xs text-muted-foreground mt-1">
                  Hold this key to activate this layer (one modifier per layer)
                </p>
              </>
            ) : availableModifiers.length === 0 ? (
              <>
                <div className="flex h-9 w-full items-center rounded-md border border-destructive/50 bg-destructive/10 px-3 py-1 text-sm text-destructive">
                  No modifiers in BASE
                </div>
                <p className="text-xs text-destructive mt-1">
                  ⚠️ Map at least one modifier in BASE layer first
                </p>
              </>
            ) : (
              <>
                <div className="flex h-9 w-full items-center rounded-md border border-warning/50 bg-warning/10 px-3 py-1 text-sm text-warning">
                  All modifiers in use
                </div>
                <p className="text-xs text-warning mt-1">
                  ⚠️ All available modifiers are used by other layers. Delete a layer or map more modifiers in BASE.
                </p>
              </>
            )}
          </div>

          {/* Stats */}
          <div className="rounded-lg bg-muted p-3">
            <div className="flex items-center justify-between text-sm">
              <span className="text-muted-foreground">Mapped keys:</span>
              <span className="font-semibold">
                {Object.keys(layer.mappings || {}).length}
              </span>
            </div>
            <div className="flex items-center justify-between text-sm mt-1">
              <span className="text-muted-foreground">Shift mappings:</span>
              <span className="font-semibold">
                {(layer.shiftMappings || []).length}
              </span>
            </div>
          </div>
        </div>

        {/* Actions */}
        <div className="mt-6 flex items-center justify-between">
          <button
            onClick={handleDelete}
            className={cn(
              "inline-flex items-center justify-center rounded-md text-sm font-medium transition-colors h-9 px-3 gap-2",
              "border border-destructive text-destructive hover:bg-destructive hover:text-destructive-foreground"
            )}
          >
            <Trash2 size={14} />
            Delete
          </button>

          <div className="flex items-center gap-2">
            <button
              onClick={onClose}
              className="inline-flex items-center justify-center rounded-md text-sm font-medium border border-input bg-background hover:bg-accent h-9 px-4"
            >
              Cancel
            </button>
            <button
              onClick={handleSave}
              className="inline-flex items-center justify-center rounded-md text-sm font-medium bg-primary text-primary-foreground hover:bg-primary/90 h-9 px-4 gap-2"
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
