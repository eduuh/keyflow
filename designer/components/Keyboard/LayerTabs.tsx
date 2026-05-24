"use client";

import { useState } from "react";
import { Plus, Settings2 } from "lucide-react";
import { useConfigStore } from "@/lib/store";
import { cn } from "@/lib/utils";
import { LayerEditor } from "./LayerEditor";

// Layer tabs row. Visual: each tab is a small "keycap" matching the keyboard
// chassis below. Active tab gets the amber LED accent. No artificial layer
// cap (the previous 4-layer warning is gone — C++ supports unlimited).

const MODIFIER_NAMES = [
  "RALT",
  "LALT",
  "RCTRL",
  "LCTRL",
  "RSHIFT",
  "LSHIFT",
  "RWIN",
  "LWIN",
  "CAPS",
];

function formatTriggers(triggers: string[] | undefined): string {
  if (!triggers || triggers.length === 0) return "—";
  return triggers.map((t) => t.toUpperCase()).join(" / ");
}

export function LayerTabs() {
  const { config, currentLayerIndex, setCurrentLayer, addLayer, hasBaseModifiers } =
    useConfigStore();
  const [editingLayer, setEditingLayer] = useState<number | null>(null);

  const layers = config.layers || [];
  const canAddLayer = hasBaseModifiers();

  // Pick a default trigger for a new layer: first BASE-remapped modifier
  // that's not already used as a trigger.
  const getDefaultTrigger = () => {
    const baseRemapping = config.remapping || {};
    const availableModifiers = Object.values(baseRemapping).filter((target) =>
      MODIFIER_NAMES.includes(target),
    );
    const usedTriggers = layers
      .map((l) => l.triggers?.[0])
      .filter((t): t is string => !!t);
    const unused = availableModifiers.find((mod) => !usedTriggers.includes(mod));
    return unused || availableModifiers[0] || "RALT";
  };

  const handleAddLayer = () => {
    if (!canAddLayer) {
      alert(
        "Cannot add layer: No modifiers mapped in BASE layer.\n\n" +
          "Please map at least one modifier key (Ctrl, Alt, Shift, Win, or Caps) " +
          "in the BASE layer first.",
      );
      return;
    }

    const defaultTrigger = getDefaultTrigger();
    addLayer({
      name: `Layer ${layers.length + 1}`,
      triggers: [defaultTrigger],
      mappings: {},
    });
    setCurrentLayer(layers.length);
  };

  return (
    <div className="flex items-center gap-2 overflow-x-auto py-1">
      <LayerTab
        active={currentLayerIndex === -1}
        title="BASE"
        triggerLabel="Always active"
        count={Object.keys(config.remapping || {}).length}
        onClick={() => setCurrentLayer(-1)}
      />

      {layers.map((layer, index) => (
        <div key={index} className="relative group shrink-0">
          <LayerTab
            active={currentLayerIndex === index}
            title={formatTriggers(layer.triggers)}
            triggerLabel={layer.name}
            count={Object.keys(layer.mappings || {}).length}
            onClick={() => setCurrentLayer(index)}
          />
          <button
            onClick={(e) => {
              e.stopPropagation();
              setEditingLayer(index);
            }}
            className={cn(
              "absolute top-1.5 right-1.5 h-6 w-6 inline-flex items-center justify-center rounded-md",
              "text-muted-foreground hover:text-foreground hover:bg-accent",
              "opacity-0 group-hover:opacity-100 transition-opacity",
            )}
            title="Edit layer"
            aria-label="Edit layer"
          >
            <Settings2 size={12} />
          </button>
        </div>
      ))}

      <button
        onClick={handleAddLayer}
        disabled={!canAddLayer}
        className={cn(
          "shrink-0 inline-flex items-center gap-1.5 h-12 px-3 rounded-md font-mono text-xs",
          "border border-dashed border-border text-muted-foreground",
          canAddLayer
            ? "hover:border-primary/60 hover:text-foreground hover:bg-accent/40 cursor-pointer transition-colors"
            : "opacity-40 cursor-not-allowed",
        )}
        title={canAddLayer ? "Add new layer" : "Map a modifier in BASE first"}
      >
        <Plus size={14} />
        <span className="uppercase tracking-wider">Add layer</span>
      </button>

      {!canAddLayer && (
        <div className="shrink-0 text-xs font-mono uppercase tracking-wider text-destructive/80 px-2">
          ⚠ Map a modifier in BASE first
        </div>
      )}

      {editingLayer !== null && (
        <LayerEditor layerIndex={editingLayer} onClose={() => setEditingLayer(null)} />
      )}
    </div>
  );
}

type LayerTabProps = {
  active: boolean;
  title: string;
  triggerLabel: string;
  count: number;
  onClick: () => void;
};

// One layer button styled like a small keycap. Active = amber LED border.
function LayerTab({ active, title, triggerLabel, count, onClick }: LayerTabProps) {
  return (
    <button
      type="button"
      onClick={onClick}
      data-selected={active || undefined}
      className={cn(
        "key-face shrink-0 inline-flex flex-col items-start justify-center",
        "h-12 px-3 rounded-md min-w-[5.5rem]",
        "transition-colors",
      )}
    >
      <div className="flex items-center gap-2">
        <span className="font-mono text-xs font-semibold tracking-wider uppercase">
          {title}
        </span>
        <span className="font-mono text-[10px] text-muted-foreground">{count}</span>
      </div>
      <span className="text-[10px] text-muted-foreground truncate max-w-[12rem]">
        {triggerLabel}
      </span>
    </button>
  );
}
