"use client";

import { useState } from "react";
import { Plus, Settings2 } from "lucide-react";
import { useConfigStore } from "@/lib/store";
import { cn } from "@/lib/utils";
import { LayerEditor } from "./LayerEditor";

const MAX_DEFAULT_LAYERS = 4;

// Format trigger key for display
function formatTrigger(trigger: string | string[]): string {
  const triggerKey = Array.isArray(trigger) ? trigger[0] : trigger;

  // Triggers already use short names in the config (RALT, LCTRL, etc.)
  // Just return uppercase version
  return triggerKey.toUpperCase();
}

export function LayerTabs() {
  const { config, currentLayerIndex, setCurrentLayer, addLayer, hasBaseModifiers } =
    useConfigStore();
  const [editingLayer, setEditingLayer] = useState<number | null>(null);

  const layers = config.layers || [];
  const canAddLayer = hasBaseModifiers();

  // Get available modifiers from BASE for default trigger
  const getDefaultTrigger = () => {
    const baseRemapping = config.remapping || {};
    const modifiers = ["RALT", "LALT", "RCTRL", "LCTRL", "RSHIFT", "LSHIFT", "RWIN", "LWIN", "CAPS"];

    // Get all available modifiers from BASE
    const availableModifiers = Object.values(baseRemapping).filter((target) =>
      modifiers.includes(target)
    );

    // Get triggers already used by existing layers
    const usedTriggers = layers.map((l) => {
      return Array.isArray(l.trigger) ? l.trigger[0] : l.trigger;
    });

    // Find first available modifier that's not used
    const unusedModifier = availableModifiers.find((mod) => !usedTriggers.includes(mod));

    return unusedModifier || availableModifiers[0] || "RALT";
  };

  const handleAddLayer = () => {
    if (!canAddLayer) {
      alert("Cannot add layer: No modifiers mapped in BASE layer.\n\nPlease map at least one modifier key (Ctrl, Alt, Shift, Win, or Caps) in the BASE layer first.");
      return;
    }

    // Check if all available modifiers are used
    const defaultTrigger = getDefaultTrigger();
    const baseRemapping = config.remapping || {};
    const modifiers = ["RALT", "LALT", "RCTRL", "LCTRL", "RSHIFT", "LSHIFT", "RWIN", "LWIN", "CAPS"];
    const availableModifiers = Object.values(baseRemapping).filter((target) =>
      modifiers.includes(target)
    );
    const usedTriggers = layers.map((l) => Array.isArray(l.trigger) ? l.trigger[0] : l.trigger);
    const unusedModifiers = availableModifiers.filter((mod) => !usedTriggers.includes(mod));

    if (unusedModifiers.length === 0) {
      alert("Cannot add layer: All available modifiers are in use.\n\nEach layer needs a unique trigger modifier. Either:\n• Delete an existing layer\n• Map more modifiers in BASE layer");
      return;
    }

    if (layers.length >= MAX_DEFAULT_LAYERS) {
      const confirmed = confirm(
        `You've reached the recommended limit of ${MAX_DEFAULT_LAYERS} layers. More layers increase complexity. Continue anyway?`
      );
      if (!confirmed) return;
    }

    const layerNum = layers.length + 1;
    addLayer({
      name: `Layer ${layerNum}`,
      trigger: defaultTrigger,
      mappings: {},
    });
    setCurrentLayer(layers.length);
  };

  return (
    <div className="flex items-center gap-2 overflow-x-auto">
      {/* BASE Layer */}
      <button
        onClick={() => setCurrentLayer(-1)}
        className={cn(
          "inline-flex flex-col items-start rounded-lg border-2 px-3 py-2 transition-all shrink-0",
          "hover:bg-accent",
          currentLayerIndex === -1
            ? "border-primary bg-primary/5 shadow-sm"
            : "border-border"
        )}
      >
        <div className="flex items-center gap-2">
          <div className="font-semibold text-sm">BASE</div>
          <div className="text-xs text-muted-foreground font-mono">
            {Object.keys(config.remapping || {}).length}
          </div>
        </div>
        <div className="text-xs text-muted-foreground">Always active</div>
      </button>

      {/* Layer Tabs */}
      {layers.map((layer, index) => (
        <div key={index} className="relative group shrink-0">
          <button
            onClick={() => setCurrentLayer(index)}
            className={cn(
              "inline-flex flex-col items-start rounded-lg border-2 px-3 py-2 pr-8 transition-all w-full",
              "hover:bg-accent",
              currentLayerIndex === index
                ? "border-primary bg-primary/5 shadow-sm"
                : "border-border"
            )}
          >
            <div className="flex items-center gap-2">
              <div className="font-semibold text-sm">{formatTrigger(layer.trigger)}</div>
              <div className="text-xs text-muted-foreground font-mono">
                {Object.keys(layer.mappings || {}).length}
              </div>
            </div>
            <div className="text-xs text-muted-foreground">{layer.name}</div>
          </button>

          {/* Edit Button */}
          <button
            onClick={(e) => {
              e.stopPropagation();
              setEditingLayer(index);
            }}
            className="absolute top-2 right-2 inline-flex items-center justify-center rounded-md text-sm font-medium hover:bg-accent/80 h-6 w-6 opacity-0 group-hover:opacity-100 transition-opacity"
            title="Edit layer"
          >
            <Settings2 size={12} />
          </button>
        </div>
      ))}

      {/* Add Layer Button */}
      <button
        onClick={handleAddLayer}
        disabled={!canAddLayer}
        className={cn(
          "inline-flex items-center gap-2 rounded-lg border-2 border-dashed px-3 py-2 transition-all shrink-0",
          canAddLayer
            ? "hover:bg-accent hover:border-primary/50 cursor-pointer"
            : "opacity-50 cursor-not-allowed border-destructive/30"
        )}
        title={!canAddLayer ? "Map modifiers in BASE layer first" : "Add new layer"}
      >
        <Plus size={14} />
        <div className="text-sm font-medium">Add</div>
        <div className="text-xs text-muted-foreground font-mono">
          ({layers.length}/{MAX_DEFAULT_LAYERS})
        </div>
      </button>

      {!canAddLayer && (
        <div className="text-xs text-destructive bg-destructive/10 border border-destructive/30 rounded-lg px-3 py-2 shrink-0">
          ⚠️ Map modifiers in BASE first
        </div>
      )}

      {/* Layer Editor Modal */}
      {editingLayer !== null && (
        <LayerEditor
          layerIndex={editingLayer}
          onClose={() => setEditingLayer(null)}
        />
      )}
    </div>
  );
}
