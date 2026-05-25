// Per-layer accent palette. Each layer maps to one hue used in three places:
// the layer tab (active state), the trigger-key ring on the keyboard, and the
// side-print band on each mapped keycap. Cycles past 5.
//
// Palette: refined earth tones tuned to sit with the GMK Olivia cream/mocha/
// burgundy theme. Each hue is desaturated enough to feel curated, not neon.
//   teal     — sophisticated cool counterpoint to the warm cream
//   clay     — earthy red-orange, echoes burgundy at lower intensity
//   mustard  — golden yellow-brown, distinct from cream by chroma
//   plum     — deep purple-red, refined and editorial
//   olive    — natural green-brown, organic

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
  { name: "teal",    hex: "#2C5F5D", hexSoft: "#3D7977", hexDeep: "#1F4644" },
  { name: "clay",    hex: "#C5704C", hexSoft: "#D88B6A", hexDeep: "#94522F" },
  { name: "mustard", hex: "#B89548", hexSoft: "#CDAF66", hexDeep: "#876B2C" },
  { name: "plum",    hex: "#5F3850", hexSoft: "#7C4D6A", hexDeep: "#3F2334" },
  { name: "olive",   hex: "#6E7A3F", hexSoft: "#8B9657", hexDeep: "#4E5728" },
];

export function layerAccent(index: number): LayerAccent {
  return PALETTE[index % PALETTE.length];
}
