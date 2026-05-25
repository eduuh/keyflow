"use client";

import { useEffect, useRef, useState } from "react";
import { useConfigStore } from "@/lib/store";
import { ansiLayout } from "@/lib/keyboardLayout";
import { layerAccent } from "@/lib/layerColors";
import { Key } from "./Key";

// The keyboard stage.
//
// Conceit: the keyboard is presented as a "specimen" on a curated surface
// — a faint dotted blueprint grid behind, a hairline frame around, and a
// small editorial caption beneath. The serif (Fraunces) carries the brand
// moments; the mono (IBM Plex) lives on the keycaps.
//
// Responsive scale: ResizeObserver measures the container and picks the
// largest keycap unit that fits without horizontal scroll, clamped to a
// readable range.

const MIN_SCALE = 50;
const MAX_SCALE = 120;
const KEYBOARD_UNITS_WIDE = 15;
const KEYBOARD_UNITS_TALL = 5;
const FRAME_INSET_PX = 28; // breathing room around the keyboard inside the frame
const CARD_PADDING_PX = 56; // outer card padding budget for ResizeObserver math

export function Keyboard() {
  const containerRef = useRef<HTMLDivElement>(null);
  const [scale, setScale] = useState(72);
  const { config, currentLayerIndex } = useConfigStore();
  const layers = config.layers ?? [];

  useEffect(() => {
    if (!containerRef.current) return;
    const el = containerRef.current;
    const recalc = () => {
      const avail = el.clientWidth - CARD_PADDING_PX * 2;
      const fit = avail / KEYBOARD_UNITS_WIDE;
      const next = Math.min(MAX_SCALE, Math.max(MIN_SCALE, Math.floor(fit)));
      setScale(next);
    };
    recalc();
    const ro = new ResizeObserver(recalc);
    ro.observe(el);
    return () => ro.disconnect();
  }, []);

  const width = KEYBOARD_UNITS_WIDE * scale;
  const height = KEYBOARD_UNITS_TALL * scale;

  const activeLayerName =
    currentLayerIndex === -1
      ? "BASE — all layers visible"
      : layers[currentLayerIndex]?.name ?? `Layer ${currentLayerIndex + 1}`;

  return (
    <div
      ref={containerRef}
      data-tour="keyboard"
      className="relative w-full rounded-2xl overflow-hidden"
      style={{
        background:
          "radial-gradient(ellipse 80% 60% at 50% 30%, #F8F2E4 0%, #F2EBD9 55%, #ECE3CC 100%)",
        border: "1px solid rgba(61,53,48,0.18)",
        boxShadow:
          "inset 0 1px 0 rgba(255,255,255,0.7), inset 0 -1px 0 rgba(61,53,48,0.08), 0 8px 24px -12px rgba(61,53,48,0.25)",
      }}
    >
      {/* Blueprint grid — faint, only behind the keyboard */}
      <div
        aria-hidden
        className="absolute inset-0 opacity-[0.18] pointer-events-none"
        style={{
          backgroundImage:
            "radial-gradient(circle at 1px 1px, rgba(61,53,48,0.42) 1px, transparent 0)",
          backgroundSize: "14px 14px",
          maskImage:
            "radial-gradient(ellipse 70% 55% at 50% 50%, black 40%, transparent 90%)",
          WebkitMaskImage:
            "radial-gradient(ellipse 70% 55% at 50% 50%, black 40%, transparent 90%)",
        }}
      />

      {/* Editorial header — small specimen tag, then the brand */}
      <div className="relative flex items-baseline justify-between px-7 pt-6">
        <div className="flex items-baseline gap-3">
          <span
            className="font-serif italic text-[#6B2D3F]"
            style={{ fontSize: "1.05rem", fontVariationSettings: "'opsz' 144" }}
          >
            Specimen N°{(currentLayerIndex + 2).toString().padStart(2, "0")}
          </span>
          <span className="text-[10px] tracking-[0.18em] uppercase text-[#3D3530]/55 font-sans">
            ANSI 104 · 15u
          </span>
        </div>
        <span className="text-[10px] tracking-[0.22em] uppercase text-[#3D3530]/55 font-sans">
          {activeLayerName}
        </span>
      </div>

      {/* The keyboard, centered on its stage */}
      <div
        className="relative flex justify-center"
        style={{ paddingTop: `${FRAME_INSET_PX}px`, paddingBottom: `${FRAME_INSET_PX}px` }}
      >
        <div className="relative" style={{ width: `${width}px`, height: `${height}px` }}>
          {/* Hairline crop-marks at the four corners of the keyboard */}
          <CropMarks width={width} height={height} />
          {ansiLayout.map((keyData) => (
            <Key key={keyData.id} keyData={keyData} scale={scale} />
          ))}
        </div>
      </div>

      {/* Layer legend footer — color swatches + names */}
      <div className="relative px-7 pb-5 flex items-center justify-between gap-4 flex-wrap">
        <div className="flex items-center gap-4 flex-wrap">
          {layers.map((layer, i) => {
            const accent = layerAccent(i);
            const isActive = currentLayerIndex === i;
            const triggerLabel = (layer.triggers ?? []).join(" / ") || "—";
            return (
              <div key={i} className="flex items-center gap-2">
                <span
                  className="inline-block rounded-[2px]"
                  style={{
                    width: 10,
                    height: 10,
                    background: accent.hex,
                    boxShadow: isActive
                      ? `0 0 0 2px ${accent.hex}33`
                      : "0 0 0 1px rgba(61,53,48,0.18)",
                  }}
                />
                <span
                  className="font-serif text-[13px] tracking-tight"
                  style={{
                    color: isActive ? accent.hex : "#3D3530",
                    fontVariationSettings: "'opsz' 14",
                  }}
                >
                  {layer.name}
                </span>
                <span className="text-[10px] tracking-[0.14em] uppercase text-[#3D3530]/55 font-sans">
                  {triggerLabel}
                </span>
              </div>
            );
          })}
        </div>
        <span
          className="font-serif italic text-[#3D3530]/60 text-[12px]"
          style={{ fontVariationSettings: "'opsz' 14" }}
        >
          {config.name ?? "Untitled"}
        </span>
      </div>
    </div>
  );
}

// Faint corner crop marks framing the keyboard, like a print specimen.
function CropMarks({ width, height }: { width: number; height: number }) {
  const len = 14;
  const off = 8;
  const stroke = "rgba(61,53,48,0.28)";
  return (
    <svg
      aria-hidden
      className="absolute pointer-events-none"
      width={width + off * 2}
      height={height + off * 2}
      style={{ left: -off, top: -off }}
    >
      {/* TL */}
      <line x1={off} y1={off - len} x2={off} y2={off} stroke={stroke} />
      <line x1={off - len} y1={off} x2={off} y2={off} stroke={stroke} />
      {/* TR */}
      <line x1={off + width} y1={off - len} x2={off + width} y2={off} stroke={stroke} />
      <line x1={off + width} y1={off} x2={off + width + len} y2={off} stroke={stroke} />
      {/* BL */}
      <line x1={off} y1={off + height} x2={off} y2={off + height + len} stroke={stroke} />
      <line x1={off - len} y1={off + height} x2={off} y2={off + height} stroke={stroke} />
      {/* BR */}
      <line x1={off + width} y1={off + height} x2={off + width} y2={off + height + len} stroke={stroke} />
      <line x1={off + width} y1={off + height} x2={off + width + len} y2={off + height} stroke={stroke} />
    </svg>
  );
}
