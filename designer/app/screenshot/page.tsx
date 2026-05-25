"use client";

import { useEffect, useState } from "react";
import {
  ExportKeyboard,
  buildKeyContexts,
  type LayerFilter,
} from "@/components/Export/ExportKeyboard";
import type { KeyFlowConfig } from "@/lib/types";

// Renderer for documentation screenshots. Read by the Playwright capture spec
// (designer/tests/capture-images.spec.ts) which iterates over ?layer= values
// and snapshots the #screenshot-root element to docs/images/.
//
// Why fetch instead of import: Turbopack refuses cross-root imports, so the
// bundled config can't be `import`ed from ../../src/. A `predev` / `prebuild`
// npm hook copies src/config.json into designer/public/eduuh-dh-config.json
// (see scripts/copy-config.mjs), and this page fetches that static asset.
//
// Why window.location instead of useSearchParams: useSearchParams in the App
// Router requires a Suspense boundary in static-export mode, and without it
// the route silently bails out of prerender. Reading window.location.search
// in useEffect avoids the Suspense requirement entirely since the whole page
// only matters client-side anyway.
//
// URL params:
//   ?layer=all   → BASE + every layer mapping stacked per keycap (composite)
//   ?layer=base  → BASE remap only
//   ?layer=0     → BASE + layer 0 (Numpad for Eduuh-DH)
//   ?layer=1     → BASE + layer 1 (Arrows & Symbols)

function parseLayerParam(raw: string | null): LayerFilter {
  if (raw === "base") return "base";
  const idx = Number(raw);
  if (Number.isInteger(idx) && idx >= 0) return idx;
  return "all";
}

export default function ScreenshotPage() {
  const [layerFilter, setLayerFilter] = useState<LayerFilter>("all");
  const [config, setConfig] = useState<KeyFlowConfig | null>(null);

  useEffect(() => {
    // Force dark theme so screenshots don't depend on the visitor's persisted
    // ThemeProvider preference (which lives in localStorage).
    const root = document.documentElement;
    root.classList.remove("light");
    root.classList.add("dark");

    setLayerFilter(parseLayerParam(new URLSearchParams(window.location.search).get("layer")));

    // Fetch the bundled Eduuh-DH config. Absolute path because relative would
    // resolve under /screenshot/ and 404 against the public asset at /.
    // Note: this route is intended for local capture (Playwright + npm run
    // capture-images), not for production viewing. On GitHub Pages with the
    // /keyflow basePath, the absolute path would 404 — a separate concern.
    fetch("/eduuh-dh-config.json")
      .then((r) => r.json())
      .then((c) => setConfig(c as KeyFlowConfig))
      .catch((err) => {
        console.error("Failed to load eduuh-dh-config.json", err);
      });
  }, []);

  if (!config) {
    // Render nothing until the config is loaded — keeps the
    // [data-screenshot-ready] selector from matching before the keyboard is real.
    return <div style={{ background: "#0a0a0a", minHeight: "100vh" }} />;
  }

  const keyContexts = buildKeyContexts(config);

  return (
    <div
      style={{
        background:
          "radial-gradient(ellipse at top, #161B2A 0%, #0B0E16 60%, #060810 100%)",
        padding: "32px 24px",
        minHeight: "100vh",
        display: "flex",
        alignItems: "flex-start",
        justifyContent: "flex-start",
      }}
    >
      <div
        id="screenshot-root"
        data-screenshot-ready
        style={{
          // Subtle mounting-plate frame so the keyboard reads as a unit
          // rather than floating cells on bare background.
          padding: "20px",
          background:
            "linear-gradient(180deg, #1A1F2C 0%, #131724 100%)",
          borderRadius: "14px",
          boxShadow:
            "inset 0 1px 0 rgba(255,255,255,0.04), 0 24px 60px -20px rgba(0,0,0,0.6)",
          border: "1px solid rgba(255,255,255,0.04)",
        }}
      >
        <ExportKeyboard
          keyContexts={keyContexts}
          isHighlighted={() => false}
          config={config}
          scale={80}
          layerFilter={layerFilter}
        />
      </div>
    </div>
  );
}
