"use client";

import { cn } from "@/lib/utils";
import { useConfigStore } from "@/lib/store";
import { KeyLayoutData } from "@/lib/keyboardLayout";
import { getKeyType, KEY_TYPE_STYLES } from "@/lib/keyTypes";

type KeyProps = {
  keyData: KeyLayoutData;
  scale?: number;
};

// One physical key on the keyboard view. Color-coded by what it produces
// in the current view (BASE → physical type; layer → output type), so the
// eye can scan and tell modifier/letter/number/symbol apart at a glance.
//
// Visual states:
//   - selected: ring around the key
//   - layer trigger: amber ring (when viewing the layer it triggers)
//   - unmapped in a layer: muted/dimmed
//   - has mappings in other layers: pip dots along the bottom (BASE view only)
export function Key({ keyData, scale = 50 }: KeyProps) {
  const { config, selectedKey, setSelectedKey, currentLayerIndex } = useConfigStore();

  const isSelected = selectedKey === keyData.keyCode;
  const baseMapping = config.remapping?.[keyData.keyCode];
  const layers = config.layers ?? [];

  const layerHits = layers.map((layer) => {
    const hasMapping = layer.mappings?.[keyData.keyCode] !== undefined;
    const hasShift = layer.shiftMappings?.some((m) => m.key === keyData.keyCode) ?? false;
    return hasMapping || hasShift;
  });

  const isLayerTrigger =
    currentLayerIndex >= 0 &&
    layers[currentLayerIndex] &&
    (layers[currentLayerIndex].triggers ?? []).some((t) => baseMapping === t);

  // What does this key produce in the current view? That drives the color tint.
  let legend = keyData.label;
  let isMapped = false;
  let isShiftLegend = false;
  let outputCode = keyData.keyCode; // What the produced output is (for color)

  if (currentLayerIndex === -1) {
    if (baseMapping) {
      legend = baseMapping;
      isMapped = true;
      outputCode = baseMapping;
    } else {
      outputCode = keyData.keyCode;
    }
  } else if (layers[currentLayerIndex]) {
    const layer = layers[currentLayerIndex];
    const shiftMapping = layer.shiftMappings?.find((m) => m.key === keyData.keyCode);
    const layerMapping = layer.mappings?.[keyData.keyCode];
    if (shiftMapping) {
      legend = shiftMapping.output;
      isMapped = true;
      isShiftLegend = true;
      outputCode = shiftMapping.output;
    } else if (layerMapping) {
      legend = layerMapping;
      isMapped = true;
      outputCode = layerMapping;
    } else {
      // Not mapped in this layer — dim and color by physical type.
      outputCode = keyData.keyCode;
    }
  }

  const keyType = getKeyType(outputCode);
  const typeStyle = KEY_TYPE_STYLES[keyType];
  const showLayerPips = currentLayerIndex === -1 && layerHits.some(Boolean);
  const isDim = currentLayerIndex !== -1 && !isMapped;

  return (
    <button
      type="button"
      onClick={() => setSelectedKey(keyData.keyCode)}
      className={cn(
        "absolute rounded-md border-2 p-0 font-mono text-xs font-medium",
        "transition-colors active:scale-[0.97]",
        "flex items-center justify-center",
        // Color by output type
        typeStyle.tint,
        // States
        isSelected && "ring-2 ring-primary ring-offset-2 ring-offset-background",
        isLayerTrigger && !isSelected && "ring-2 ring-amber-500",
        isShiftLegend && "italic",
        isDim && "opacity-30",
      )}
      style={{
        left: `${keyData.x * scale}px`,
        top: `${keyData.y * scale}px`,
        width: `${keyData.width * scale - 4}px`,
        height: `${keyData.height * scale - 4}px`,
      }}
    >
      <span className="truncate px-1">
        {isShiftLegend && <span className="opacity-60">⇧</span>}
        {legend}
      </span>
      {showLayerPips && (
        <span className="absolute bottom-1 right-1 flex gap-0.5 pointer-events-none">
          {layerHits.map((hit, i) =>
            hit ? <span key={i} className="h-1 w-1 rounded-full bg-current opacity-60" /> : null,
          )}
        </span>
      )}
    </button>
  );
}
