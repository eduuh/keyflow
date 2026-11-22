"use client";

import { cn } from "@/lib/utils";
import { useConfigStore, useEditingConfig } from "@/lib/store";
import { KeyLayoutData } from "@/lib/keyboardLayout";
import { displayLabel, shiftedDisplayLabel, TRIGGER_ALIAS } from "@/lib/keyDisplay";
import { canonicalKey } from "@/lib/allKeys";
import { keyMapping } from "@/lib/config";
import { layerAccent } from "@/lib/layerColors";

export function Key({ keyData, scale = 50 }: { keyData: KeyLayoutData; scale?: number }) {
  const { selectedKey, setSelectedKey, currentLayerIndex } = useConfigStore();
  const config = useEditingConfig();
  const layers = config.layers ?? [];
  const baseMapping = keyMapping(config.remapping, keyData.keyCode);
  const baseOutput = baseMapping ?? keyData.keyCode;
  const layer = layers[currentLayerIndex];
  const matches = (key: string) => canonicalKey(key) === keyData.keyCode;
  const shift = layer?.shiftMappings?.find((mapping) => matches(mapping.key));
  const combo = currentLayerIndex === -1 ? config.noModCombos?.find((mapping) => matches(mapping.key)) : undefined;
  const mapping = layer ? keyMapping(layer.mappings, keyData.keyCode) : baseMapping;
  const output = shift?.output ?? combo?.output ?? mapping ?? baseOutput;
  const shifted = shift ? shift.shift !== false : combo?.shift === true;
  const legend = shifted ? shiftedDisplayLabel(output) : displayLabel(output);
  const selected = selectedKey === keyData.keyCode;
  const trigger = layer?.triggers.some((name) => {
    const custom = config.customModifiers?.find((mod) => mod.modifierName === name);
    const key = custom?.key ?? TRIGGER_ALIAS[name];
    return key && (matches(key) || canonicalKey(baseOutput) === canonicalKey(key));
  });
  const bindings = currentLayerIndex === -1 ? layers.flatMap((entry, index) => {
    const mapped = keyMapping(entry.mappings, keyData.keyCode);
    const shiftedMapping = entry.shiftMappings?.find((item) => matches(item.key));
    if (!mapped && !shiftedMapping) return [];
    const label = shiftedMapping
      ? shiftedMapping.shift !== false ? shiftedDisplayLabel(shiftedMapping.output) : displayLabel(shiftedMapping.output)
      : displayLabel(mapped!);
    return [{ index, label, name: entry.name }];
  }) : [];
  const changed = !!(mapping || shift || combo);
  const fontSize = legend.length <= 2 ? scale * 0.34 : legend.length <= 5 ? scale * 0.2 : scale * 0.14;
  const details = bindings.map((entry) => `${entry.name}: ${entry.label}`).join("\n");
  return (
    <button type="button" onClick={() => setSelectedKey(keyData.keyCode)} aria-pressed={selected}
      aria-label={`${keyData.keyCode}${changed ? ` → ${legend}` : ""}`}
      title={`${keyData.keyCode} → ${legend}${trigger ? " (layer trigger)" : ""}${details ? `\n${details}` : ""}`}
      data-key={keyData.keyCode}
      className={cn(
        "absolute rounded-md border bg-background shadow-sm text-foreground transition-colors",
        "hover:border-primary hover:bg-accent",
        selected && "ring-2 ring-primary ring-offset-1 ring-offset-card border-primary bg-accent",
        trigger && "border-primary border-2",
        currentLayerIndex >= 0 && !changed && !trigger && "text-muted-foreground",
      )}
      style={{ left: keyData.x * scale + 2, top: keyData.y * scale + 2,
        width: keyData.width * scale - 5, height: keyData.height * scale - 5 }}>
      <span className="absolute top-1 left-1.5 text-muted-foreground font-mono leading-none"
        style={{ fontSize: Math.max(8, scale * 0.13) }}>{keyData.label}</span>
      <span className={cn("font-keycap font-semibold leading-none", changed && "text-primary")}
        style={{ fontSize }}>{legend}</span>
      {bindings.length > 0 && (
        <span className="absolute bottom-1 left-1 right-1 flex justify-center gap-1 overflow-hidden">
          {bindings.slice(0, 2).map((entry) => (
            <span key={entry.index} className="font-mono font-medium leading-none px-1 rounded-sm text-white"
              style={{ background: layerAccent(entry.index).hex, fontSize: Math.max(8, scale * 0.13) }}>
              {entry.label}
            </span>
          ))}
          {bindings.length > 2 && <span className="text-[9px] text-muted-foreground">+{bindings.length - 2}</span>}
        </span>
      )}
    </button>
  );
}
