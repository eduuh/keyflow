"use client";

import { useState, useRef, useEffect } from "react";
import { X, Download, FileJson, Sun, Moon, Package } from "lucide-react";
import { toPng } from "html-to-image";
import { useConfigStore } from "@/lib/store";
import { useTheme } from "./ThemeProvider";
import { ansiLayout, KeyLayoutData } from "@/lib/keyboardLayout";
import { getKeyType } from "@/lib/keyTypes";
import { Key } from "./Keyboard/Key";
import { cn } from "@/lib/utils";

type ExportPreviewModalProps = {
  isOpen: boolean;
  onClose: () => void;
};

type ExportTheme = "light" | "dark";

export function ExportPreviewModal({ isOpen, onClose }: ExportPreviewModalProps) {
  const { config } = useConfigStore();
  const { theme: appTheme } = useTheme();
  const [isGenerating, setIsGenerating] = useState(false);
  const [previewTheme, setPreviewTheme] = useState<ExportTheme>(appTheme);
  const exportContainerRef = useRef<HTMLDivElement>(null);

  // Update preview theme when app theme changes
  useEffect(() => {
    if (isOpen) {
      setPreviewTheme(appTheme);
    }
  }, [isOpen, appTheme]);

  // Close on Escape key
  useEffect(() => {
    const handleEscape = (e: KeyboardEvent) => {
      if (e.key === "Escape") onClose();
    };
    if (isOpen) {
      document.addEventListener("keydown", handleEscape);
      return () => document.removeEventListener("keydown", handleEscape);
    }
  }, [isOpen, onClose]);

  if (!isOpen) return null;

  const captureWithTheme = async (theme: ExportTheme): Promise<string> => {
    if (!exportContainerRef.current) throw new Error("Export container not found");

    // Temporarily switch preview theme
    const originalTheme = previewTheme;
    setPreviewTheme(theme);

    // Wait for theme to apply
    await new Promise((resolve) => setTimeout(resolve, 100));

    const bgColor = theme === "dark" ? "#0a0a0a" : "#ffffff";

    const dataUrl = await toPng(exportContainerRef.current, {
      quality: 1.0,
      pixelRatio: 2,
      backgroundColor: bgColor,
    });

    // Restore original theme
    setPreviewTheme(originalTheme);

    return dataUrl;
  };

  const handleExportCurrentTheme = async () => {
    setIsGenerating(true);
    try {
      const dataUrl = await captureWithTheme(previewTheme);

      const link = document.createElement("a");
      link.download = `keyflow-layout-${previewTheme}.png`;
      link.href = dataUrl;
      link.click();
    } catch (error) {
      console.error("Failed to generate image:", error);
      alert("Failed to generate image. Please try again.");
    } finally {
      setIsGenerating(false);
    }
  };

  const handleExportBothThemes = async () => {
    setIsGenerating(true);
    try {
      // Dynamic import JSZip
      const JSZip = (await import("jszip")).default;
      const zip = new JSZip();

      // Capture light theme
      const lightDataUrl = await captureWithTheme("light");
      const lightBlob = await fetch(lightDataUrl).then((r) => r.blob());
      zip.file("keyflow-layout-light.png", lightBlob);

      // Capture dark theme
      const darkDataUrl = await captureWithTheme("dark");
      const darkBlob = await fetch(darkDataUrl).then((r) => r.blob());
      zip.file("keyflow-layout-dark.png", darkBlob);

      // Generate zip
      const zipBlob = await zip.generateAsync({ type: "blob" });
      const zipUrl = URL.createObjectURL(zipBlob);

      const link = document.createElement("a");
      link.download = "keyflow-layouts.zip";
      link.href = zipUrl;
      link.click();

      URL.revokeObjectURL(zipUrl);
    } catch (error) {
      console.error("Failed to generate bundle:", error);
      alert("Failed to generate bundle. Please try again.");
    } finally {
      setIsGenerating(false);
    }
  };

  const handleExportConfig = () => {
    const json = JSON.stringify(config, null, 2);
    const blob = new Blob([json], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.download = "config.json";
    link.href = url;
    link.click();
    URL.revokeObjectURL(url);
  };

  const layers = config.layers || [];
  const scale = 42; // Smaller scale for quad view
  const keyboardWidth = 15 * scale;
  const keyboardHeight = 5 * scale;

  // Color legend data
  const legendItems = [
    {
      color: "border-orange-300 bg-orange-50 text-orange-700",
      colorDark: "border-orange-700 bg-orange-950 text-orange-300",
      label: "Modifiers",
    },
    {
      color: "border-blue-300 bg-blue-50 text-blue-700",
      colorDark: "border-blue-700 bg-blue-950 text-blue-300",
      label: "Navigation",
    },
    {
      color: "border-emerald-300 bg-emerald-50 text-emerald-700",
      colorDark: "border-emerald-700 bg-emerald-950 text-emerald-300",
      label: "Numbers",
    },
    {
      color: "border-violet-300 bg-violet-50 text-violet-700",
      colorDark: "border-violet-700 bg-violet-950 text-violet-300",
      label: "Symbols",
    },
    {
      color: "border-gray-300 bg-white text-gray-700",
      colorDark: "border-gray-700 bg-gray-900 text-gray-300",
      label: "Basic Keys",
    },
  ];

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/80 backdrop-blur-md">
      <div className="relative max-h-[95vh] w-[95vw] max-w-[1800px] overflow-auto rounded-xl border bg-background shadow-2xl">
        {/* Header */}
        <div className="sticky top-0 z-10 border-b bg-background/95 backdrop-blur">
          <div className="flex items-center justify-between px-6 py-4">
            <div>
              <h2 className="text-xl font-semibold">Export KeyFlow Layout</h2>
              <p className="text-sm text-muted-foreground">
                Choose what to export: visual design or config file
              </p>
            </div>
            <button
              onClick={onClose}
              className="inline-flex items-center justify-center rounded-md text-sm font-medium border border-input bg-background hover:bg-accent h-9 w-9"
              aria-label="Close"
            >
              <X size={18} />
            </button>
          </div>

          {/* Export Controls */}
          <div className="px-6 py-3 bg-muted/30 border-t flex items-center justify-between gap-4">
            <div className="flex items-center gap-3">
              {/* Theme Toggle */}
              <div className="flex items-center gap-2">
                <span className="text-xs font-medium text-muted-foreground">Preview:</span>
                <div className="inline-flex rounded-lg border p-1 gap-1">
                  <button
                    onClick={() => setPreviewTheme("light")}
                    className={cn(
                      "inline-flex items-center justify-center rounded-md px-3 py-1 text-xs font-medium transition-colors",
                      previewTheme === "light"
                        ? "bg-background shadow-sm"
                        : "hover:bg-background/50"
                    )}
                  >
                    <Sun size={12} className="mr-1.5" />
                    Light
                  </button>
                  <button
                    onClick={() => setPreviewTheme("dark")}
                    className={cn(
                      "inline-flex items-center justify-center rounded-md px-3 py-1 text-xs font-medium transition-colors",
                      previewTheme === "dark"
                        ? "bg-background shadow-sm"
                        : "hover:bg-background/50"
                    )}
                  >
                    <Moon size={12} className="mr-1.5" />
                    Dark
                  </button>
                </div>
              </div>
            </div>

            {/* Export Buttons */}
            <div className="flex items-center gap-2">
              <button
                onClick={handleExportConfig}
                disabled={isGenerating}
                className={cn(
                  "inline-flex items-center justify-center rounded-md text-sm font-medium transition-colors h-9 px-4 gap-2",
                  "border border-input bg-background hover:bg-accent",
                  "disabled:opacity-50 disabled:cursor-not-allowed"
                )}
              >
                <FileJson size={16} />
                Export Config
              </button>
              <button
                onClick={handleExportCurrentTheme}
                disabled={isGenerating}
                className={cn(
                  "inline-flex items-center justify-center rounded-md text-sm font-medium transition-colors h-9 px-4 gap-2",
                  "bg-primary text-primary-foreground hover:bg-primary/90",
                  "disabled:opacity-50 disabled:cursor-not-allowed"
                )}
              >
                <Download size={16} />
                {isGenerating ? "Generating..." : `Export ${previewTheme === "light" ? "Light" : "Dark"}`}
              </button>
              <button
                onClick={handleExportBothThemes}
                disabled={isGenerating}
                className={cn(
                  "inline-flex items-center justify-center rounded-md text-sm font-medium transition-colors h-9 px-4 gap-2",
                  "bg-violet-600 text-white hover:bg-violet-700",
                  "disabled:opacity-50 disabled:cursor-not-allowed"
                )}
              >
                <Package size={16} />
                {isGenerating ? "Generating..." : "Export Both (ZIP)"}
              </button>
            </div>
          </div>
        </div>

        {/* Export Content */}
        <div className="p-6">
          <div
            ref={exportContainerRef}
            className={cn(
              "p-8 rounded-lg",
              previewTheme === "dark" ? "bg-[#0a0a0a]" : "bg-white"
            )}
          >
            {/* Title */}
            <div className="mb-6 text-center">
              <h1
                className={cn(
                  "text-2xl font-bold",
                  previewTheme === "dark" ? "text-gray-100" : "text-gray-900"
                )}
              >
                KeyFlow Layout
              </h1>
              <p
                className={cn(
                  "text-sm mt-1",
                  previewTheme === "dark" ? "text-gray-400" : "text-gray-600"
                )}
              >
                {config.strictMode ? "Strict Mode Enabled" : "Standard Mode"}
              </p>
            </div>

            {/* Quad View Grid */}
            <div className="grid grid-cols-2 gap-6 mb-6">
              {/* BASE Layer */}
              <LayerPreview
                title="BASE Layer"
                layerIndex={-1}
                scale={scale}
                width={keyboardWidth}
                height={keyboardHeight}
                theme={previewTheme}
              />

              {/* Layer 1 */}
              {layers[0] && (
                <LayerPreview
                  title={`Layer 1: ${layers[0].name}`}
                  layerIndex={0}
                  scale={scale}
                  width={keyboardWidth}
                  height={keyboardHeight}
                  theme={previewTheme}
                />
              )}

              {/* Layer 2 */}
              {layers[1] && (
                <LayerPreview
                  title={`Layer 2: ${layers[1].name}`}
                  layerIndex={1}
                  scale={scale}
                  width={keyboardWidth}
                  height={keyboardHeight}
                  theme={previewTheme}
                />
              )}

              {/* Layer 3 */}
              {layers[2] && (
                <LayerPreview
                  title={`Layer 3: ${layers[2].name}`}
                  layerIndex={2}
                  scale={scale}
                  width={keyboardWidth}
                  height={keyboardHeight}
                  theme={previewTheme}
                />
              )}
            </div>

            {/* Color Legend */}
            <div
              className={cn(
                "border-t pt-4",
                previewTheme === "dark" ? "border-gray-800" : "border-gray-200"
              )}
            >
              <h3
                className={cn(
                  "text-sm font-semibold mb-3",
                  previewTheme === "dark" ? "text-gray-100" : "text-gray-900"
                )}
              >
                Color Legend
              </h3>
              <div className="flex flex-wrap gap-3">
                {legendItems.map((item, index) => (
                  <div key={index} className="flex items-center gap-2">
                    <div
                      className={cn(
                        "h-6 w-6 rounded border-2",
                        previewTheme === "dark" ? item.colorDark : item.color
                      )}
                    />
                    <span
                      className={cn(
                        "text-xs",
                        previewTheme === "dark" ? "text-gray-300" : "text-gray-700"
                      )}
                    >
                      {item.label}
                    </span>
                  </div>
                ))}
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

type LayerPreviewProps = {
  title: string;
  layerIndex: number;
  scale: number;
  width: number;
  height: number;
  theme: ExportTheme;
};

function LayerPreview({ title, layerIndex, scale, width, height, theme }: LayerPreviewProps) {
  return (
    <div>
      <h3
        className={cn(
          "text-sm font-semibold mb-2",
          theme === "dark" ? "text-gray-100" : "text-gray-900"
        )}
      >
        {title}
      </h3>
      <div
        className={cn(
          "border rounded-lg p-2",
          theme === "dark"
            ? "border-gray-700 bg-gray-900/50"
            : "border-gray-300 bg-gray-50"
        )}
      >
        <div
          className={cn(
            "relative rounded",
            theme === "dark" ? "bg-gray-950" : "bg-white"
          )}
          style={{
            width: `${width}px`,
            height: `${height}px`,
          }}
        >
          {ansiLayout.map((keyData) => (
            <ExportKey
              key={keyData.id}
              keyData={keyData}
              scale={scale}
              layerIndex={layerIndex}
              theme={theme}
            />
          ))}
        </div>
      </div>
    </div>
  );
}

type ExportKeyProps = {
  keyData: KeyLayoutData;
  scale: number;
  layerIndex: number;
  theme: ExportTheme;
};

function ExportKey({ keyData, scale, layerIndex, theme }: ExportKeyProps) {
  const { config } = useConfigStore();
  const strictMode = config.strictMode || false;

  // Get mappings
  const baseMapping = config.remapping?.[keyData.keyCode];
  const layers = config.layers || [];

  // Determine display value
  let displayValue = keyData.label;
  let isShiftMapping = false;

  if (layerIndex === -1) {
    // BASE layer
    if (strictMode && !baseMapping) {
      displayValue = "✕";
    } else {
      displayValue = baseMapping || keyData.label;
    }
  } else if (layers[layerIndex]) {
    // Layer view - strict layer behavior
    const layer = layers[layerIndex];
    const layerMapping = layer.mappings[keyData.keyCode];
    const shiftMapping = layer.shiftMappings?.find((m) => m.key === keyData.keyCode);

    if (shiftMapping) {
      displayValue = `⇧${shiftMapping.output}`;
      isShiftMapping = true;
    } else if (layerMapping) {
      displayValue = layerMapping;
    } else {
      displayValue = "✕";
    }
  }

  // Determine if key has mappings
  const hasMappings =
    layerIndex === -1
      ? !!baseMapping
      : !!(layers[layerIndex]?.mappings[keyData.keyCode] ||
          layers[layerIndex]?.shiftMappings?.some((m) => m.key === keyData.keyCode));

  // Color classes
  const getColorClasses = () => {
    const keyType = getKeyType(keyData.keyCode);
    const isDark = theme === "dark";

    switch (keyType) {
      case "modifier":
        return cn(
          isDark ? "border-orange-700/50" : "border-orange-300",
          hasMappings
            ? isDark
              ? "bg-orange-950/30 text-orange-300"
              : "bg-orange-50 text-orange-700"
            : isDark
            ? "text-orange-400"
            : "text-orange-600"
        );
      case "navigation":
        return cn(
          isDark ? "border-blue-700/50" : "border-blue-300",
          hasMappings
            ? isDark
              ? "bg-blue-950/30 text-blue-300"
              : "bg-blue-50 text-blue-700"
            : isDark
            ? "text-blue-400"
            : "text-blue-600"
        );
      case "number":
        return cn(
          isDark ? "border-emerald-700/50" : "border-emerald-300",
          hasMappings
            ? isDark
              ? "bg-emerald-950/30 text-emerald-300"
              : "bg-emerald-50 text-emerald-700"
            : isDark
            ? "text-emerald-400"
            : "text-emerald-600"
        );
      case "symbol":
        return cn(
          isDark ? "border-violet-700/50" : "border-violet-300",
          hasMappings
            ? isDark
              ? "bg-violet-950/30 text-violet-300"
              : "bg-violet-50 text-violet-700"
            : isDark
            ? "text-violet-400"
            : "text-violet-600"
        );
      default:
        return cn(
          hasMappings
            ? isDark
              ? "border-blue-700/50 bg-blue-950/30 text-blue-300"
              : "border-blue-400 bg-blue-50 text-blue-700"
            : isDark
            ? "border-gray-700 text-gray-400"
            : "border-gray-300 text-gray-700"
        );
    }
  };

  return (
    <div
      className={cn(
        "absolute rounded border-2",
        "flex items-center justify-center",
        "text-[10px] font-semibold",
        theme === "dark" ? "bg-gray-900" : "bg-white",
        getColorClasses()
      )}
      style={{
        left: `${keyData.x * scale}px`,
        top: `${keyData.y * scale}px`,
        width: `${keyData.width * scale - 2}px`,
        height: `${keyData.height * scale - 2}px`,
      }}
    >
      <span
        className={cn(
          isShiftMapping &&
            (theme === "dark" ? "text-purple-400 font-bold" : "text-purple-700 font-bold")
        )}
      >
        {displayValue}
      </span>
    </div>
  );
}

// Helper function to determine key type
