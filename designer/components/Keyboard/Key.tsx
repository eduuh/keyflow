"use client";

import { cn } from "@/lib/utils";
import { useConfigStore } from "@/lib/store";
import { KeyLayoutData } from "@/lib/keyboardLayout";
import { getKeyType } from "@/lib/keyTypes";
import { displayLabel, shiftedDisplayLabel, TRIGGER_ALIAS } from "@/lib/keyDisplay";
import { layerAccent } from "@/lib/layerColors";

type KeyProps = {
  keyData: KeyLayoutData;
  scale?: number;
};

// A sculpted keycap.
//
// Two-tone GMK-Olivia convention:
//   alphas / numbers / symbols → cream face, mocha legend
//   modifiers / Tab / Enter / Space → mocha face, cream legend
//   F-row / layer triggers → burgundy face, cream legend
//
// Bottom edge of the cap carries "side-print" bands — one band per layer
// that maps this key, colored in that layer's hue. So the whole keyboard
// can be read in BASE view: the main legend tells you what the key types
// today; the bands tell you what each layer turns it into.
//
// Sizing is scale-relative so legends stay legible from the 44px minimum
// (mobile) up to the 110px maximum (ultrawide).

type LayerStripData = {
  index: number;
  out?: string;
  shifted?: string;
};

type CapTone = "cream" | "mocha" | "burgundy";

function toneFor(keyCode: string): CapTone {
  const type = getKeyType(keyCode);
  if (type === "modifier" || type === "other") return "mocha";
  if (type === "function") return "burgundy";
  return "cream";
}

const TONE_STYLES: Record<
  CapTone,
  { gradient: string; topHighlight: string; bottomShade: string; legendColor: string; borderColor: string }
> = {
  cream: {
    gradient: "linear-gradient(180deg, #FAF7F0 0%, #F5F1E8 55%, #EBE5D3 100%)",
    topHighlight: "rgba(255,255,255,0.75)",
    bottomShade: "rgba(61,53,48,0.18)",
    legendColor: "#3D3530",
    borderColor: "rgba(61,53,48,0.22)",
  },
  mocha: {
    gradient: "linear-gradient(180deg, #4D443E 0%, #3D3530 55%, #2A231E 100%)",
    topHighlight: "rgba(255,255,255,0.10)",
    bottomShade: "rgba(0,0,0,0.45)",
    legendColor: "#F0E9D8",
    borderColor: "rgba(0,0,0,0.40)",
  },
  burgundy: {
    gradient: "linear-gradient(180deg, #7B3548 0%, #6B2D3F 55%, #4D1F2D 100%)",
    topHighlight: "rgba(255,255,255,0.14)",
    bottomShade: "rgba(0,0,0,0.40)",
    legendColor: "#F5EBDB",
    borderColor: "rgba(0,0,0,0.35)",
  },
};

export function Key({ keyData, scale = 50 }: KeyProps) {
  const { config, selectedKey, setSelectedKey, currentLayerIndex } = useConfigStore();

  const isSelected = selectedKey === keyData.keyCode;
  const baseMapping = config.remapping?.[keyData.keyCode];
  const layers = config.layers ?? [];

  // Resolve legend + cap tone for the current view.
  let legend = keyData.label;
  let isMapped = false;
  let isShiftLegend = false;
  let outputCode = keyData.keyCode;

  if (currentLayerIndex === -1) {
    const combo = config.noModCombos?.find((c) => c.key === keyData.keyCode);
    if (combo) {
      legend = combo.shift ? shiftedDisplayLabel(combo.output) : displayLabel(combo.output);
      isMapped = true;
      outputCode = combo.output;
    } else if (baseMapping) {
      legend = displayLabel(baseMapping);
      isMapped = true;
      outputCode = baseMapping;
    }
  } else if (layers[currentLayerIndex]) {
    const layer = layers[currentLayerIndex];
    const sm = layer.shiftMappings?.find((m) => m.key === keyData.keyCode);
    const lm = layer.mappings?.[keyData.keyCode];
    if (sm) {
      legend = shiftedDisplayLabel(sm.output);
      isMapped = true;
      isShiftLegend = true;
      outputCode = sm.output;
    } else if (lm) {
      legend = displayLabel(lm);
      isMapped = true;
      outputCode = lm;
    }
  }

  // Tone is driven by what the keycap PRODUCES (so a CapsLock remapped to
  // LeftShift still reads as a modifier keycap, mocha).
  const tone = toneFor(outputCode);
  const toneStyle = TONE_STYLES[tone];

  // Trigger ring uses the layer's color, so the eye binds "this hue = this layer".
  const isLayerTrigger =
    currentLayerIndex >= 0 &&
    layers[currentLayerIndex] &&
    (layers[currentLayerIndex].triggers ?? []).some((t) => {
      const physical = TRIGGER_ALIAS[t] ?? t;
      return keyData.keyCode === physical || baseMapping === physical;
    });
  const triggerAccent = isLayerTrigger ? layerAccent(currentLayerIndex) : null;

  // Side-print bands — only meaningful in BASE view.
  const layerStrips: LayerStripData[] =
    currentLayerIndex === -1
      ? layers
          .map<LayerStripData>((layer, index) => {
            const m = layer.mappings?.[keyData.keyCode];
            const s = layer.shiftMappings?.find((x) => x.key === keyData.keyCode);
            return {
              index,
              out: m ? displayLabel(m) : undefined,
              shifted: s ? shiftedDisplayLabel(s.output) : undefined,
            };
          })
          .filter((s) => s.out || s.shifted)
      : [];

  const isDim = currentLayerIndex !== -1 && !isMapped;
  const showBands = layerStrips.length > 0;

  // Scale-relative typography. Single-char legends get the biggest cap face;
  // long labels like "SPACE"/"PRTSC" shrink to fit without truncation.
  const charCount = legend.length;
  const legendFontPx =
    charCount <= 1 ? scale * 0.46 :
    charCount <= 2 ? scale * 0.36 :
    charCount <= 4 ? scale * 0.22 :
                     scale * 0.16;
  const bandHeightPx = Math.max(9, Math.round(scale * 0.13));
  const bandFontPx = Math.max(8, Math.round(scale * 0.13));

  // Cap rect — slightly inset from the chassis so the "front edge" (bottom
  // dark line) reads as a sculpted lower lip of the keycap.
  const inset = 2;
  const width = keyData.width * scale - 4;
  const height = keyData.height * scale - 4;

  const ringStyle = triggerAccent
    ? { boxShadow: `0 0 0 2px ${triggerAccent.hex}, 0 0 14px ${triggerAccent.hex}55, 0 2px 4px rgba(0,0,0,0.12)` }
    : isSelected
      ? { boxShadow: `0 0 0 2px #6B2D3F, 0 0 14px rgba(107,45,63,0.45), 0 2px 4px rgba(0,0,0,0.12)` }
      : { boxShadow: "0 2px 3px rgba(0,0,0,0.14), 0 1px 1px rgba(0,0,0,0.06)" };

  return (
    <button
      type="button"
      onClick={() => setSelectedKey(keyData.keyCode)}
      aria-label={`${keyData.keyCode}${isMapped ? ` → ${legend}` : ""}`}
      className={cn(
        "absolute p-0 rounded-[7px]",
        "transition-[transform,box-shadow,opacity] duration-150 ease-out",
        "hover:-translate-y-[1px] active:translate-y-[1px]",
        isDim && "opacity-[0.22]",
      )}
      style={{
        left: `${keyData.x * scale + inset}px`,
        top: `${keyData.y * scale + inset}px`,
        width: `${width - inset * 2}px`,
        height: `${height - inset * 2}px`,
        ...ringStyle,
      }}
    >
      {/* Sculpted cap surface: gradient + inner highlight + lower shade. */}
      <div
        className="absolute inset-0 rounded-[7px] overflow-hidden flex flex-col"
        style={{
          background: toneStyle.gradient,
          border: `1px solid ${toneStyle.borderColor}`,
          boxShadow: `inset 0 1px 0 ${toneStyle.topHighlight}, inset 0 -2px 0 ${toneStyle.bottomShade}`,
        }}
      >
        {/* Main legend face */}
        <div
          className="flex-1 flex items-center justify-center px-1 select-none"
          style={{ color: toneStyle.legendColor }}
        >
          <span
            className={cn("font-keycap leading-none tracking-tight", isShiftLegend && "italic")}
            style={{
              fontSize: `${legendFontPx}px`,
              fontWeight: tone === "cream" ? 500 : 400,
              letterSpacing: charCount <= 2 ? "-0.02em" : "0",
            }}
          >
            {legend}
          </span>
        </div>

        {/* Side-print bands (BASE view only) */}
        {showBands && (
          <div className="flex flex-col">
            {layerStrips.map((strip) => {
              const accent = layerAccent(strip.index);
              return (
                <div
                  key={strip.index}
                  className="flex items-center justify-center gap-[3px] text-white/95 select-none"
                  style={{
                    height: `${bandHeightPx}px`,
                    background: accent.hex,
                    fontSize: `${bandFontPx}px`,
                    borderTop: `1px solid ${accent.hexDeep}`,
                    lineHeight: 1,
                  }}
                >
                  {strip.out && (
                    <span className="font-keycap font-medium tracking-tight">
                      {strip.out}
                    </span>
                  )}
                  {strip.shifted && (
                    <span className="font-keycap italic opacity-80 tracking-tight">
                      {strip.shifted}
                    </span>
                  )}
                </div>
              );
            })}
          </div>
        )}
      </div>
    </button>
  );
}
