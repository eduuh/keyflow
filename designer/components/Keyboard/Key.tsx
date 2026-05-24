"use client";

import { cn } from "@/lib/utils";
import { useConfigStore } from "@/lib/store";
import { KeyLayoutData } from "@/lib/keyboardLayout";
import { getKeyType } from "@/lib/keyTypes";

type KeyProps = {
  keyData: KeyLayoutData;
  scale?: number;
};

export function Key({ keyData, scale = 50 }: KeyProps) {
  const { config, selectedKey, setSelectedKey, currentLayerIndex } =
    useConfigStore();

  const isSelected = selectedKey === keyData.keyCode;
  const keyType = getKeyType(keyData.keyCode);
  const strictMode = config.strictMode || false;

  // Get mappings for this key
  const baseMapping = config.remapping?.[keyData.keyCode];
  const layers = config.layers || [];

  // Dynamic layer mappings for all layers
  const layerMappings = layers.map((layer) => layer.mappings?.[keyData.keyCode]);

  const layerShiftMappings = layers.map((layer) =>
    layer.shiftMappings?.some((m) => m.key === keyData.keyCode)
  );

  const hasMappings =
    baseMapping ||
    layerMappings.some((mapping) => mapping !== undefined) ||
    layerShiftMappings.some((hasShift) => hasShift);

  // Check if this key is a trigger for the current layer
  const isLayerTrigger = currentLayerIndex !== -1 && layers[currentLayerIndex] && (() => {
    const trigger = layers[currentLayerIndex].trigger;
    const triggers = Array.isArray(trigger) ? trigger : [trigger];
    // Check if this key is remapped to one of the triggers in BASE
    return triggers.some(t => baseMapping === t);
  })();

  // Check if this key has mappings in other layers (for BASE view)
  const hasLayerMappings = currentLayerIndex === -1 && (
    layerMappings.some((mapping) => mapping !== undefined) ||
    layerShiftMappings.some((hasShift) => hasShift)
  );

  const handleClick = () => {
    setSelectedKey(keyData.keyCode);
  };

  // Determine display value
  let displayValue = keyData.label;
  let isShiftMapping = false;

  if (currentLayerIndex === -1) {
    // BASE layer - respect global strict mode
    if (strictMode && !baseMapping) {
      displayValue = "✕"; // Show blocked in strict mode if not mapped
    } else {
      displayValue = baseMapping || keyData.label;
    }
  } else if (layers[currentLayerIndex]) {
    // Layer view - ALWAYS show only keys in this layer (strict layer behavior)
    const layer = layers[currentLayerIndex];
    const layerMapping = layer.mappings[keyData.keyCode];
    const shiftMapping = layer.shiftMappings?.find((m) => m.key === keyData.keyCode);

    if (shiftMapping) {
      // Show shift mapping with ⇧ prefix
      displayValue = `⇧${shiftMapping.output}`;
      isShiftMapping = true;
    } else if (layerMapping) {
      displayValue = layerMapping;
    } else {
      displayValue = "✕"; // Show blocked if not in this layer
    }
  }

  // Color classes based on key type
  const getColorClasses = () => {
    if (isSelected) {
      return "border-primary shadow-lg ring-2 ring-primary/20 text-foreground";
    }

    // Highlight layer trigger modifier
    if (isLayerTrigger) {
      return "border-amber-400 dark:border-amber-600 bg-amber-100 dark:bg-amber-900/30 text-amber-900 dark:text-amber-200 ring-2 ring-amber-400/50 dark:ring-amber-600/50";
    }

    // Color coding for different key types
    switch (keyType) {
      case "modifier":
        return cn(
          "border-orange-300 dark:border-orange-700/50",
          hasMappings
            ? "bg-orange-50 dark:bg-orange-950/30 text-orange-700 dark:text-orange-300"
            : "text-orange-600 dark:text-orange-400 hover:bg-orange-50 dark:hover:bg-orange-950/20"
        );
      case "navigation":
        return cn(
          "border-blue-300 dark:border-blue-700/50",
          hasMappings
            ? "bg-blue-50 dark:bg-blue-950/30 text-blue-700 dark:text-blue-300"
            : "text-blue-600 dark:text-blue-400 hover:bg-blue-50 dark:hover:bg-blue-950/20"
        );
      case "number":
        return cn(
          "border-emerald-300 dark:border-emerald-700/50",
          hasMappings
            ? "bg-emerald-50 dark:bg-emerald-950/30 text-emerald-700 dark:text-emerald-300"
            : "text-emerald-600 dark:text-emerald-400 hover:bg-emerald-50 dark:hover:bg-emerald-950/20"
        );
      case "symbol":
        return cn(
          "border-violet-300 dark:border-violet-700/50",
          hasMappings
            ? "bg-violet-50 dark:bg-violet-950/30 text-violet-700 dark:text-violet-300"
            : "text-violet-600 dark:text-violet-400 hover:bg-violet-50 dark:hover:bg-violet-950/20"
        );
      default:
        return cn(
          hasMappings
            ? "border-primary/30 bg-primary/5 text-primary"
            : "border-border text-foreground hover:bg-accent"
        );
    }
  };

  return (
    <div
      className={cn(
        "absolute cursor-pointer rounded-md transition-all duration-150",
        "bg-background border-2",
        "flex items-center justify-center",
        "text-base font-semibold",
        "hover:border-primary/50",
        "active:scale-95",
        getColorClasses()
      )}
      style={{
        left: `${keyData.x * scale}px`,
        top: `${keyData.y * scale}px`,
        width: `${keyData.width * scale - 4}px`,
        height: `${keyData.height * scale - 4}px`,
      }}
      onClick={handleClick}
    >
      <span className={cn(isShiftMapping && "text-purple-600 dark:text-purple-400 font-bold")}>
        {displayValue}
      </span>
      {hasLayerMappings && (
        <span className="absolute bottom-0.5 right-0.5 flex gap-0.5">
          {layerMappings.map((mapping, index) => {
            if (!mapping && !layerShiftMappings[index]) return null;
            // Color palette for layer indicators (up to 9 layers)
            const colors = [
              "bg-blue-500",
              "bg-green-500",
              "bg-red-500",
              "bg-yellow-500",
              "bg-purple-500",
              "bg-pink-500",
              "bg-orange-500",
              "bg-teal-500",
              "bg-indigo-500",
            ];
            const color = colors[index % colors.length];
            return <span key={index} className={`w-1 h-1 rounded-full ${color}`} />;
          })}
        </span>
      )}
    </div>
  );
}
