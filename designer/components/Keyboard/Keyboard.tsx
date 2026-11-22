"use client";

import { useEffect, useRef, useState } from "react";
import { useConfigStore, useEditingConfig } from "@/lib/store";
import { ansiLayout } from "@/lib/keyboardLayout";
import { KEY_GROUPS } from "@/lib/allKeys";
import { PLATFORMS } from "@/lib/config";
import { layerAccent } from "@/lib/layerColors";
import { Key } from "./Key";

export function Keyboard() {
  const containerRef = useRef<HTMLDivElement>(null);
  const [scale, setScale] = useState(60);
  const { currentLayerIndex, platform, selectedKey, setSelectedKey } = useConfigStore();
  const config = useEditingConfig();
  const layers = config.layers ?? [];
  const visibleKeys = new Set(ansiLayout.map((key) => key.keyCode));

  useEffect(() => {
    const element = containerRef.current;
    if (!element) return;
    const recalculate = () => setScale(Math.max(44, Math.min(80, Math.floor((element.clientWidth - 32) / 15))));
    recalculate();
    const observer = new ResizeObserver(recalculate);
    observer.observe(element);
    return () => observer.disconnect();
  }, []);

  return (
    <section ref={containerRef} data-tour="keyboard" aria-label="Keyboard canvas"
      className="min-w-0 rounded-xl border bg-card overflow-hidden">
      <div className="px-5 py-4 border-b flex flex-wrap items-center justify-between gap-2">
        <div>
          <h3 className="font-semibold text-sm">{currentLayerIndex === -1 ? "BASE · layer overview" : layers[currentLayerIndex]?.name}</h3>
          <p className="text-xs text-muted-foreground mt-1">ANSI main block · physical key above, output below</p>
        </div>
        <span className="text-xs font-medium rounded-full border bg-muted px-3 py-1">
          {platform === "shared" ? "Shared" : PLATFORMS[platform]}
        </span>
      </div>
      <div className="overflow-x-auto p-4 bg-muted/30" tabIndex={0} aria-label="Scrollable keyboard">
        <div className="relative mx-auto" style={{ width: 15 * scale, height: 5 * scale }}>
          {ansiLayout.map((keyData) => <Key key={keyData.id} keyData={keyData} scale={scale} />)}
        </div>
      </div>
      <div className="border-t p-4 space-y-3">
        <div className="flex flex-wrap gap-x-4 gap-y-2">
          {layers.map((layer, index) => (
            <button key={index} type="button" onClick={() => useConfigStore.getState().setCurrentLayer(index)}
              className="inline-flex items-center gap-2 text-xs hover:underline">
              <span className="w-2.5 h-2.5 rounded-sm" style={{ background: layerAccent(index).hex }} />
              {layer.name}<span className="text-muted-foreground">{layer.triggers.join(" / ")}</span>
            </button>
          ))}
          <span className="text-xs text-muted-foreground">Outlined = selected · colored edge = layer binding</span>
        </div>
        <div className="flex flex-wrap items-center gap-3">
          <label htmlFor="other-key" className="text-xs font-medium">Other physical keys</label>
          <select id="other-key" className="min-w-0 max-w-full rounded-md border bg-background px-2 py-1.5 text-sm"
            value={selectedKey && !visibleKeys.has(selectedKey) ? selectedKey : ""}
            onChange={(event) => event.target.value && setSelectedKey(event.target.value)}>
            <option value="">Function, arrows, numpad, media...</option>
            {KEY_GROUPS.map((group) => (
              <optgroup key={group.group} label={group.group}>
                {group.keys.filter((key) => !visibleKeys.has(key.code) && key.code !== "NOP")
                  .map((key) => <option key={key.code} value={key.code}>{key.label}</option>)}
              </optgroup>
            ))}
          </select>
        </div>
      </div>
    </section>
  );
}
