"use client";

import { ansiLayout } from "@/lib/keyboardLayout";
import { useConfigStore } from "@/lib/store";
import { Key } from "./Key";

// The visible "chassis" wrapping the keys. Mimics a keyboard case: dark base,
// subtle drop shadow, an indicator strip up top showing the active layer.
// The strip is the only place layer status surfaces inside the chassis (the
// surrounding LayerTabs handle the controls).
export function Keyboard() {
  const { config, currentLayerIndex } = useConfigStore();
  const scale = 85;
  const width = 15 * scale;
  const height = 5 * scale;

  const layers = config.layers ?? [];
  const activeLayerName =
    currentLayerIndex === -1
      ? "BASE"
      : layers[currentLayerIndex]?.name ?? `Layer ${currentLayerIndex + 1}`;
  const activeTriggers =
    currentLayerIndex === -1 ? [] : layers[currentLayerIndex]?.triggers ?? [];

  return (
    <div className="w-full">
      <div
        className="relative rounded-xl bg-card shadow-chassis border border-border overflow-hidden"
        style={{ width: "fit-content", maxWidth: "100%" }}
      >
        {/* Chassis info strip — model name on the left, layer indicator on the right.
            On a real keyboard this would be the case bezel above the keys. */}
        <div className="flex items-center justify-between px-4 py-2 border-b border-border bg-muted/30">
          <div className="flex items-center gap-2 text-xs font-mono text-muted-foreground">
            <span className="inline-block w-1.5 h-1.5 rounded-full bg-primary shadow-[0_0_6px_hsl(var(--primary))]" />
            <span className="tracking-widest uppercase">
              {config.name || "Keyflow"}
            </span>
          </div>
          <div className="flex items-center gap-2 text-xs font-mono">
            <span className="text-muted-foreground">LAYER:</span>
            <span className="text-primary font-semibold uppercase tracking-wider">
              {activeLayerName}
            </span>
            {activeTriggers.length > 0 && (
              <span className="text-muted-foreground">
                ({activeTriggers.join(" / ")})
              </span>
            )}
          </div>
        </div>

        {/* Key bed */}
        <div className="p-4">
          <div
            className="relative"
            style={{ width: `${width}px`, height: `${height}px` }}
          >
            {ansiLayout.map((keyData) => (
              <Key key={keyData.id} keyData={keyData} scale={scale} />
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}
