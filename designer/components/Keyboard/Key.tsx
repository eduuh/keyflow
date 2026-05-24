"use client";

import { cn } from "@/lib/utils";
import { useConfigStore } from "@/lib/store";
import { KeyLayoutData } from "@/lib/keyboardLayout";

type KeyProps = {
  keyData: KeyLayoutData;
  scale?: number;
};

// One physical key on the keyboard view.
//
// Visual rules (all expressed via data-attributes on the .key-face element so
// the CSS in globals.css owns the look):
//   data-selected="true" → bright amber ring + glow (current selection)
//   data-trigger="true"  → amber border + soft glow (this key triggers the
//                          currently-viewed layer)
//   data-mapped="true"   → faint cyan inner glow (this key is configured)
//
// The amber and cyan are LED-style accents on a dark-slate keycap face. The
// face itself is a top→bottom gradient with an inset highlight and a
// bottom-edge shadow, so keys feel slightly raised.
export function Key({ keyData, scale = 50 }: KeyProps) {
  const { config, selectedKey, setSelectedKey, currentLayerIndex } = useConfigStore();

  const isSelected = selectedKey === keyData.keyCode;
  const baseMapping = config.remapping?.[keyData.keyCode];
  const layers = config.layers ?? [];

  // Which layers have a mapping (regular or shift) for this physical key.
  // Used both for "configured" status and the LED-pip strip in BASE view.
  const layerHits = layers.map((layer) => {
    const hasMapping = layer.mappings?.[keyData.keyCode] !== undefined;
    const hasShift = layer.shiftMappings?.some((m) => m.key === keyData.keyCode) ?? false;
    return hasMapping || hasShift;
  });

  const hasAnyMapping = !!baseMapping || layerHits.some(Boolean);

  // Is this key the trigger for the layer we're viewing right now?
  const isLayerTrigger =
    currentLayerIndex >= 0 &&
    layers[currentLayerIndex] &&
    (layers[currentLayerIndex].triggers ?? []).some((t) => baseMapping === t);

  // What text to render on the key face.
  let legend = keyData.label;
  let legendIsMapped = false;
  let isShiftLegend = false;

  if (currentLayerIndex === -1) {
    if (baseMapping) {
      legend = baseMapping;
      legendIsMapped = true;
    }
  } else if (layers[currentLayerIndex]) {
    const layer = layers[currentLayerIndex];
    const shiftMapping = layer.shiftMappings?.find((m) => m.key === keyData.keyCode);
    const layerMapping = layer.mappings?.[keyData.keyCode];
    if (shiftMapping) {
      legend = `⇧${shiftMapping.output}`;
      legendIsMapped = true;
      isShiftLegend = true;
    } else if (layerMapping) {
      legend = layerMapping;
      legendIsMapped = true;
    } else {
      // Key not mapped in this layer. Dim the label so the layer story reads.
      legend = keyData.label;
      legendIsMapped = false;
    }
  }

  const isMapped = currentLayerIndex === -1 ? !!baseMapping : legendIsMapped;
  const showLayerPips = currentLayerIndex === -1 && hasAnyMapping;

  return (
    <button
      type="button"
      className={cn(
        "key-face absolute flex items-center justify-center rounded-md cursor-pointer",
        "px-1.5 py-0.5",
        "focus:outline-none",
      )}
      data-selected={isSelected || undefined}
      data-trigger={isLayerTrigger || undefined}
      data-mapped={isMapped || undefined}
      style={{
        left: `${keyData.x * scale}px`,
        top: `${keyData.y * scale}px`,
        width: `${keyData.width * scale - 4}px`,
        height: `${keyData.height * scale - 4}px`,
      }}
      onClick={() => setSelectedKey(keyData.keyCode)}
    >
      <span
        className={cn(
          "key-legend text-sm font-semibold tracking-tight",
          // Larger main row keys can fit a heavier glyph
          keyData.width === 1 && keyData.height === 1 && "text-base",
          isShiftLegend && "key-legend-mapped",
          legendIsMapped && !isShiftLegend && "key-legend-mapped",
          !legendIsMapped && currentLayerIndex !== -1 && "opacity-30",
        )}
      >
        {legend}
      </span>

      {/* LED pips along the bottom-right edge — one per layer that has a
          mapping for this physical key. Only shown in BASE view. */}
      {showLayerPips && (
        <span className="absolute bottom-0.5 right-1 flex gap-0.5 pointer-events-none">
          {layerHits.map((hit, i) => (hit ? <span key={i} className="key-led-pip" /> : null))}
        </span>
      )}
    </button>
  );
}
