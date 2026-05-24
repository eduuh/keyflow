// KeyFlow configuration types based on schema

export type KeyFlowConfig = {
  version: string;
  name?: string;
  strictMode?: boolean;
  debug?: {
    debugMode?: boolean;
    showAllKeys?: boolean;
    showPipeline?: boolean;
    showTimings?: boolean;
  };
  remapping?: Record<string, string>;
  noModCombos?: NoModCombo[];
  layers?: Layer[];
};

export type NoModCombo = {
  key: string;
  output: string;
  shift?: boolean;
  description?: string;
};

export type Layer = {
  name: string;
  trigger: string | string[];
  mappings: Record<string, string>;
  shiftMappings?: ShiftMapping[];
};

export type ShiftMapping = {
  key: string;
  output: string;
  shift?: boolean;
  description?: string;
};

// UI-specific types
export type ViewMode = "single" | "all";

export type KeyState = "default" | "mapped" | "selected" | "hover" | "trigger";

export type KeyPosition = {
  x: number;
  y: number;
  width: number; // in key units (1u, 1.5u, 2u, etc.)
  height: number;
};

export type KeyData = {
  id: string;
  label: string;
  keyCode: string;
  position: KeyPosition;
  mappings: {
    base?: string;
    l1?: string;
    l2?: string;
    l3?: string;
  };
};

export type KeyCategory = "basic" | "modifiers" | "navigation" | "numbers" | "symbols" | "function";
