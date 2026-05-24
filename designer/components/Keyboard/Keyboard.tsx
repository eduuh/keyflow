"use client";

import { useEffect, useRef, useState } from "react";
import { Card } from "@/components/ui/card";
import { ansiLayout } from "@/lib/keyboardLayout";
import { Key } from "./Key";
import { KeyTypeLegend } from "./KeyTypeLegend";

// Responsive ANSI 104-key view. Measures the container with a ResizeObserver
// and picks the largest scale that fits without horizontal scroll. Always
// centered, always fills the available width.
//
// Keyboard is 15u wide; key width in pixels = scale; we leave 32px of card
// padding on each side. Clamp scale to a sensible range so it doesn't go
// microscopic on phones or absurdly large on ultra-wide monitors.

const MIN_SCALE = 44;
const MAX_SCALE = 110;
const KEYBOARD_UNITS_WIDE = 15;
const KEYBOARD_UNITS_TALL = 5;
const CARD_PADDING_PX = 32;

export function Keyboard() {
  const containerRef = useRef<HTMLDivElement>(null);
  const [scale, setScale] = useState(72);

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

  return (
    <Card ref={containerRef} className="p-4 w-full overflow-hidden">
      <div className="flex justify-center">
        <div className="relative" style={{ width: `${width}px`, height: `${height}px` }}>
          {ansiLayout.map((keyData) => (
            <Key key={keyData.id} keyData={keyData} scale={scale} />
          ))}
        </div>
      </div>
      <div className="mt-3 pt-3 border-t flex justify-center">
        <KeyTypeLegend />
      </div>
    </Card>
  );
}
