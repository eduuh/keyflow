"use client";

import { useState } from "react";
import { Plus, Settings2 } from "lucide-react";
import { useConfigStore, useEditingConfig } from "@/lib/store";
import { Button } from "@/components/ui/button";
import { cn } from "@/lib/utils";
import { layerAccent } from "@/lib/layerColors";
import { LayerEditor } from "./LayerEditor";

function formatTriggers(triggers: string[] | undefined): string {
  if (!triggers || triggers.length === 0) return "—";
  return triggers.map((t) => t.toUpperCase()).join(" / ");
}

export function LayerTabs() {
  const { currentLayerIndex, setCurrentLayer } =
    useConfigStore();
  const config = useEditingConfig();
  const [editingLayer, setEditingLayer] = useState<number | null>(null);
  const [creatingLayer, setCreatingLayer] = useState(false);

  const layers = config.layers || [];
  return (
    <div
      data-tour="layer-tabs"
      className="flex flex-wrap items-center gap-2 py-1"
    >
      <div className="flex items-center gap-2 overflow-x-auto flex-1 min-w-0">
      <LayerTab
        active={currentLayerIndex === -1}
        title="BASE"
        subtitle="Always active"
        count={Object.keys(config.remapping || {}).filter((key) => !key.startsWith("_")).length}
        onClick={() => setCurrentLayer(-1)}
      />

      {layers.map((layer, index) => (
        <div key={index} className="relative group shrink-0">
          <LayerTab
            active={currentLayerIndex === index}
            title={layer.name}
            subtitle={formatTriggers(layer.triggers)}
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
      </div>

      <Button
        variant="outline"
        onClick={() => setCreatingLayer(true)}
        className="shrink-0 h-[3.25rem] gap-1 px-3"
      >
        <Plus className="h-4 w-4" />
        New layer
      </Button>

      {editingLayer !== null && (
        <LayerEditor layerIndex={editingLayer} onClose={() => setEditingLayer(null)} />
      )}
      {creatingLayer && <LayerEditor onClose={() => setCreatingLayer(false)} />}
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
        <span className="text-sm font-medium">
          {title}
        </span>
        <span
          className="ml-auto text-[10px] font-keycap font-medium px-1.5 py-px rounded-[3px] bg-muted text-muted-foreground"
        >
          {count}
        </span>
      </div>
      <span
        className="font-mono text-[11px] truncate max-w-full mt-0.5 pr-5"
      >
        {subtitle}
      </span>
    </button>
  );
}
