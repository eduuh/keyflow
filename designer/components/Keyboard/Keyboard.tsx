"use client";

import { ansiLayout } from "@/lib/keyboardLayout";
import { Key } from "./Key";

export function Keyboard() {
  const scale = 85; // Increased to fill more width
  const width = 15 * scale;
  const height = 5 * scale;

  return (
    <div className="w-full">
      <div
        className="relative bg-card rounded-lg p-3 border w-full shadow-sm"
        style={{
          height: `${height + 24}px`,
        }}
      >
        <div className="relative w-full h-full">
          {ansiLayout.map((keyData) => (
            <Key key={keyData.id} keyData={keyData} scale={scale} />
          ))}
        </div>
      </div>
    </div>
  );
}
