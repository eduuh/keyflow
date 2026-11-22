// Per-layer accent palette. Each layer maps to one hue used in three places:
// the layer tab (active state), the trigger-key ring on the keyboard, and the
// side-print band on each mapped keycap. Cycles past 5.
//
export type LayerAccent = {
  hex: string;
  /** Slightly lighter variant for hover / soft accents. */
  hexSoft: string;
  /** Darker variant for the keycap front-edge shadow on the band. */
  hexDeep: string;
  /** Human-readable name (used in the side-legend on the keyboard). */
  name: string;
};

const PALETTE: LayerAccent[] = [
  { name: "teal",    hex: "#0F766E", hexSoft: "#14B8A6", hexDeep: "#115E59" },
  { name: "orange",  hex: "#C2410C", hexSoft: "#F97316", hexDeep: "#9A3412" },
  { name: "blue",    hex: "#1D4ED8", hexSoft: "#60A5FA", hexDeep: "#1E40AF" },
  { name: "violet",  hex: "#7E22CE", hexSoft: "#A855F7", hexDeep: "#6B21A8" },
  { name: "green",   hex: "#15803D", hexSoft: "#4ADE80", hexDeep: "#166534" },
];

export function layerAccent(index: number): LayerAccent {
  return PALETTE[index % PALETTE.length];
}
