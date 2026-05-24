"use client";

import { useState } from "react";
import { Plus, Settings2 } from "lucide-react";
import { useConfigStore } from "@/lib/store";
import { Button } from "@/components/ui/button";
import { Badge } from "@/components/ui/badge";
import { cn } from "@/lib/utils";
import { LayerEditor } from "./LayerEditor";

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
      alert("Map a modifier in BASE first — layers need a trigger.");
      return;
    }
    addLayer({
      name: `Layer ${layers.length + 1}`,
      triggers: [getDefaultTrigger()],
      mappings: {},
    });
    setCurrentLayer(layers.length);
  };

  return (
    <div className="flex items-center gap-2 overflow-x-auto">
      <LayerTab
        active={currentLayerIndex === -1}
        title="BASE"
        subtitle="Always active"
        count={Object.keys(config.remapping || {}).length}
        onClick={() => setCurrentLayer(-1)}
      />

      {layers.map((layer, index) => (
        <div key={index} className="relative group shrink-0">
          <LayerTab
            active={currentLayerIndex === index}
            title={formatTriggers(layer.triggers)}
            subtitle={layer.name}
            count={Object.keys(layer.mappings || {}).length}
            onClick={() => setCurrentLayer(index)}
          />
          <Button
            variant="ghost"
            size="icon"
            className="absolute top-1 right-1 h-6 w-6 opacity-0 group-hover:opacity-100"
            onClick={(e) => {
              e.stopPropagation();
              setEditingLayer(index);
            }}
            aria-label="Edit layer"
          >
            <Settings2 className="h-3 w-3" />
          </Button>
        </div>
      ))}

      <Button
        variant="outline"
        onClick={handleAddLayer}
        disabled={!canAddLayer}
        className="border-dashed shrink-0"
      >
        <Plus className="h-4 w-4 mr-1" />
        Add layer
      </Button>

      {!canAddLayer && (
        <span className="text-xs text-muted-foreground shrink-0">
          Map a modifier in BASE first
        </span>
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
  subtitle: string;
  count: number;
  onClick: () => void;
};

function LayerTab({ active, title, subtitle, count, onClick }: LayerTabProps) {
  return (
    <button
      type="button"
      onClick={onClick}
      className={cn(
        "shrink-0 flex flex-col items-start rounded-md border px-3 py-1.5 text-left transition-colors",
        "min-w-[7rem]",
        active
          ? "border-primary bg-primary text-primary-foreground"
          : "border-border bg-card hover:bg-accent",
      )}
    >
      <div className="flex items-center gap-2 w-full">
        <span className="text-sm font-semibold">{title}</span>
        <Badge variant={active ? "secondary" : "outline"} className="ml-auto text-[10px] py-0">
          {count}
        </Badge>
      </div>
      <span className={cn("text-xs truncate max-w-full", active ? "text-primary-foreground/80" : "text-muted-foreground")}>
        {subtitle}
      </span>
    </button>
  );
}
