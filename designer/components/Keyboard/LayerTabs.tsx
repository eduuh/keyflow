"use client";

import { useState } from "react";
import { Plus, Settings2 } from "lucide-react";
import { useConfigStore } from "@/lib/store";
import { Button } from "@/components/ui/button";
import { cn } from "@/lib/utils";
import { layerAccent } from "@/lib/layerColors";
import { LayerEditor } from "./LayerEditor";

// Layer navigation. Each tab adopts its layer's accent color; the active
// tab fills with that color and shifts to cream text. The BASE tab is the
// "all layers visible" view — its tab takes the burgundy brand color.

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
    <div
      data-tour="layer-tabs"
      className="flex items-center gap-2 overflow-x-auto py-1"
    >
      <LayerTab
        active={currentLayerIndex === -1}
        title="BASE"
        subtitle="All layers visible"
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
            accentIndex={index}
          />
          <button
            type="button"
            className="absolute top-1.5 right-1.5 h-5 w-5 rounded grid place-items-center opacity-0 group-hover:opacity-100 transition-opacity hover:bg-white/15"
            onClick={(e) => {
              e.stopPropagation();
              setEditingLayer(index);
            }}
            aria-label="Edit layer"
          >
            <Settings2 className="h-3 w-3" />
          </button>
        </div>
      ))}

      <Button
        variant="outline"
        onClick={handleAddLayer}
        disabled={!canAddLayer}
        className="border-dashed shrink-0 h-[3.25rem] gap-1 px-3 font-serif italic"
        style={{ fontVariationSettings: "'opsz' 14" }}
      >
        <Plus className="h-4 w-4" />
        Add layer
      </Button>

      {!canAddLayer && (
        <span className="text-xs text-muted-foreground shrink-0 font-serif italic">
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
  const activeColor = accent?.hex ?? "#6B2D3F"; // BASE → burgundy
  const dotColor = accent?.hex ?? "#6B2D3F";

  const inactiveStyle: React.CSSProperties = {
    background: "rgba(250,247,240,0.6)",
    borderColor: "rgba(61,53,48,0.18)",
    color: "#3D3530",
    boxShadow: "0 1px 2px rgba(61,53,48,0.06)",
  };
  const activeStyle: React.CSSProperties = {
    background: activeColor,
    borderColor: activeColor,
    color: "#F5EBDB",
    boxShadow: `0 3px 0 ${activeColor}55, 0 4px 12px ${activeColor}38`,
  };

  return (
    <button
      type="button"
      onClick={onClick}
      style={active ? activeStyle : inactiveStyle}
      className={cn(
        "shrink-0 flex flex-col items-start rounded-md border px-3.5 py-2 text-left transition-all",
        "min-w-[7.5rem]",
        active && "-translate-y-[1px]",
      )}
    >
      <div className="flex items-center gap-2 w-full">
        <span
          className="h-[6px] w-[6px] rounded-full shrink-0"
          style={{
            background: active ? "rgba(245,235,219,0.95)" : dotColor,
          }}
        />
        <span className="font-keycap text-[12px] font-medium tracking-[0.05em] uppercase">
          {title}
        </span>
        <span
          className="ml-auto text-[10px] font-keycap font-medium px-1.5 py-px rounded-[3px]"
          style={{
            background: active ? "rgba(245,235,219,0.18)" : "rgba(61,53,48,0.08)",
            color: active ? "rgba(245,235,219,0.9)" : "rgba(61,53,48,0.7)",
          }}
        >
          {count}
        </span>
      </div>
      <span
        className="font-serif italic text-[12px] truncate max-w-full mt-0.5"
        style={{
          color: active ? "rgba(245,235,219,0.82)" : "rgba(61,53,48,0.62)",
          fontVariationSettings: "'opsz' 14",
        }}
      >
        {subtitle}
      </span>
    </button>
  );
}
