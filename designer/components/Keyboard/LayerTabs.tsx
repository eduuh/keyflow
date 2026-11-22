"use client";

import { useState } from "react";
import { Plus, Settings2 } from "lucide-react";
import { useConfigStore, useEditingConfig } from "@/lib/store";
import { availableTriggers } from "@/lib/config";
import { Button } from "@/components/ui/button";
import { cn } from "@/lib/utils";
import { layerAccent } from "@/lib/layerColors";
import { LayerEditor } from "./LayerEditor";

function formatTriggers(triggers: string[] | undefined): string {
  if (!triggers || triggers.length === 0) return "—";
  return triggers.map((t) => t.toUpperCase()).join(" / ");
}

export function LayerTabs() {
  const { currentLayerIndex, setCurrentLayer, addLayer, hasBaseModifiers } =
    useConfigStore();
  const config = useEditingConfig();
  const [editingLayer, setEditingLayer] = useState<number | null>(null);

  const layers = config.layers || [];
  const canAddLayer = hasBaseModifiers();

  const getDefaultTrigger = () => {
    const availableModifiers = availableTriggers(config);
    const usedTriggers = layers.flatMap((layer) => layer.triggers);
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
    <div
      data-tour="layer-tabs"
      className="flex items-center gap-2 overflow-x-auto py-1"
    >
      <LayerTab
        active={currentLayerIndex === -1}
        title="BASE"
        subtitle="All layers visible"
        count={Object.keys(config.remapping || {}).filter((key) => !key.startsWith("_")).length}
        onClick={() => setCurrentLayer(-1)}
      />

      {layers.map((layer, index) => (
        <div key={index} className="relative group shrink-0">
          <LayerTab
            active={currentLayerIndex === index}
            title={formatTriggers(layer.triggers)}
            subtitle={layer.name}
            count={Object.keys(layer.mappings || {}).filter((key) => !key.startsWith("_")).length + (layer.shiftMappings?.length ?? 0)}
            onClick={() => setCurrentLayer(index)}
            accentIndex={index}
          />
          <button
            type="button"
            className="absolute bottom-1.5 right-1.5 h-6 w-6 rounded grid place-items-center bg-card text-foreground border hover:bg-accent"
            onClick={(e) => {
              e.stopPropagation();
              setEditingLayer(index);
            }}
            aria-label={`Edit layer ${layer.name}`}
          >
            <Settings2 className="h-3 w-3" />
          </button>
        </div>
      ))}

      <Button
        variant="outline"
        onClick={handleAddLayer}
        disabled={!canAddLayer}
        className="border-dashed shrink-0 h-[3.25rem] gap-1 px-3"
      >
        <Plus className="h-4 w-4" />
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
  accentIndex?: number;
};

function LayerTab({ active, title, subtitle, count, onClick, accentIndex }: LayerTabProps) {
  const accent = accentIndex !== undefined ? layerAccent(accentIndex) : null;
  const dotColor = accent?.hex ?? "hsl(var(--primary))";

  return (
    <button
      type="button"
      onClick={onClick}
      aria-pressed={active}
      className={cn(
        "shrink-0 flex flex-col items-start rounded-md border px-3.5 py-2 text-left transition-all",
        "min-w-[7.5rem]",
        active ? "bg-accent border-primary text-foreground" : "bg-card text-muted-foreground hover:bg-muted",
      )}
    >
      <div className="flex items-center gap-2 w-full">
        <span
          className="h-[6px] w-[6px] rounded-full shrink-0"
          style={{
            background: dotColor,
          }}
        />
        <span className="font-keycap text-[12px] font-medium tracking-[0.05em] uppercase">
          {title}
        </span>
        <span
          className="ml-auto text-[10px] font-keycap font-medium px-1.5 py-px rounded-[3px] bg-muted text-muted-foreground"
        >
          {count}
        </span>
      </div>
      <span
        className="text-[12px] truncate max-w-full mt-0.5 pr-5"
      >
        {subtitle}
      </span>
    </button>
  );
}
