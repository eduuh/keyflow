// Designer types track JsonConfig.h and ConfigLoader.h's document overrides.
// When the runtime schema changes, this file must too — that's the contract
// flagged in BUILD.md and CONTRIBUTING.md. A round-trip (import → export)
// must preserve every field defined here without loss.

/** Full layout config — one-to-one with C++ JsonConfig. */
export type KeyFlowConfig = {
  version: string;
  name?: string;
  disableCapsLock?: boolean;
  remapping?: Record<string, string>;
  noModCombos?: NoModCombo[];
  customModifiers?: CustomModifier[];
  layers?: Layer[];
  platformOverrides?: Partial<Record<Platform, PlatformOverride>>;
};

export type Platform = "linux" | "windows" | "mac";
export type EditPlatform = "shared" | Platform;
export type PlatformOverride = Pick<KeyFlowConfig,
  "remapping" | "noModCombos" | "customModifiers" | "layers" | "disableCapsLock">;

/** Number-row symbol combo. Fires only when no modifiers held (C++ blocks via blockedModifiers=0xFFFFFFFF). */
export type NoModCombo = {
  key: string;
  output: string;
  shift?: boolean;
  description?: string;
};

/** Per-layer shift mapping. Injects Shift with the output even though the user didn't press it. */
export type ShiftMapping = {
  key: string;
  output: string;
  /** Defaults to true in C++ (struct ShiftMapping.shift = true). Always serialize explicitly to avoid round-trip drift. */
  shift?: boolean;
  description?: string;
};

/** Any physical key promoted to a layer trigger by name, such as Space or PrintScreen. */
export type CustomModifier = {
  key: string;
  modifierName: string;
  blockOutput?: boolean;
};

/** Layer activated when one of its triggers is held. C++ schema accepts both `trigger: "LALT"` and `triggers: ["LALT","RALT"]`. */
export type Layer = {
  name: string;
  /** Canonical form is the array. On import we coerce the singular `trigger` into this array; on export we always write `triggers`. */
  triggers: string[];
  mappings: Record<string, string>;
  shiftMappings?: ShiftMapping[];
};

// ─────────────────────────────────────────────────────────────────────────────
// UI-side types (not part of the C++ schema)

export type ViewMode = "single" | "all";

export type KeyState = "default" | "mapped" | "selected" | "hover" | "trigger";

export type KeyPosition = {
  x: number;
  y: number;
  width: number;  // in key units (1u = 1)
  height: number;
};

export type KeyData = {
  id: string;
  label: string;
  keyCode: string;
  position: KeyPosition;
};

export type KeyCategory = "basic" | "modifiers" | "navigation" | "numbers" | "symbols" | "function";
