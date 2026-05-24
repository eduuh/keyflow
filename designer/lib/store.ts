import { create } from "zustand";
import { persist } from "zustand/middleware";
import { KeyFlowConfig, Layer, NoModCombo, CustomModifier, ShiftMapping } from "./types";

// ─────────────────────────────────────────────────────────────────────────────
// Store contract
//
// Every field defined in `KeyFlowConfig` is preserved through the store.
// Importing a config and immediately exporting it must produce byte-equivalent
// JSON for every field the schema defines. That round-trip safety is the
// fundamental contract; broken in the previous store (customModifiers,
// disableCapsLock, multi-trigger triggers[] were dropped on round-trip).
// ─────────────────────────────────────────────────────────────────────────────

type ConfigState = {
  config: KeyFlowConfig;
  currentLayerIndex: number;   // -1 = BASE layer; 0+ = config.layers[index]
  selectedKey: string | null;

  // Whole-config replacement (used by import). Normalizes legacy shapes:
  //   - `trigger: "LALT"`         → `triggers: ["LALT"]`
  //   - `trigger: ["LALT","RALT"]`→ `triggers: ["LALT","RALT"]`
  // Drops obsolete `strictMode` / `debug` fields without losing anything else.
  setConfig: (config: KeyFlowConfig) => void;

  // BASE remapping + layer mappings
  setKeyMapping: (layerIndex: number, key: string, target: string) => void;
  clearKeyMapping: (layerIndex: number, key: string) => void;

  // Layer shift mappings (only valid for layerIndex >= 0)
  setShiftMapping: (layerIndex: number, key: string, output: string, shift?: boolean) => void;
  clearShiftMapping: (layerIndex: number, key: string) => void;

  // Layer CRUD
  addLayer: (layer: Layer) => void;
  updateLayer: (index: number, partial: Partial<Layer>) => void;
  deleteLayer: (index: number) => void;

  // noModCombos (number-row symbols, etc.)
  setNoModCombo: (combo: NoModCombo) => void;
  removeNoModCombo: (key: string) => void;

  // customModifiers (turn any key into a layer trigger)
  setCustomModifier: (mod: CustomModifier) => void;
  removeCustomModifier: (modifierName: string) => void;

  // Selection UI
  setCurrentLayer: (index: number) => void;
  setSelectedKey: (key: string | null) => void;

  // Toggles
  setDisableCapsLock: (value: boolean) => void;

  // Convenience read
  hasBaseModifiers: () => boolean;
};

const STANDARD_MODIFIERS = new Set([
  "LeftShift", "RightShift", "LeftCtrl", "RightCtrl",
  "LeftAlt", "RightAlt", "LeftWin", "RightWin", "PrintScreen",
  "LSHIFT", "RSHIFT", "LCTRL", "RCTRL", "LALT", "RALT", "LWIN", "RWIN",
]);

const defaultConfig: KeyFlowConfig = {
  version: "1.0",
  name: "My Layout",
  disableCapsLock: false,
  remapping: {},
  noModCombos: [],
  customModifiers: [],
  layers: [],
};

// Normalize legacy / variant input shapes into the canonical KeyFlowConfig.
// Drops obsolete fields (strictMode, debug) so they don't pollute the local
// store; everything else passes through unchanged.
function normalizeImport(raw: unknown): KeyFlowConfig {
  const obj = (raw && typeof raw === "object" ? raw : {}) as Record<string, unknown>;

  const layers: Layer[] = Array.isArray(obj.layers)
    ? (obj.layers as Array<Record<string, unknown>>).map(normalizeLayer)
    : [];

  return {
    version: typeof obj.version === "string" ? obj.version : "1.0",
    name: typeof obj.name === "string" ? obj.name : undefined,
    disableCapsLock: typeof obj.disableCapsLock === "boolean" ? obj.disableCapsLock : undefined,
    remapping: isStringRecord(obj.remapping) ? obj.remapping : {},
    noModCombos: Array.isArray(obj.noModCombos)
      ? (obj.noModCombos as NoModCombo[]).filter(isNoModCombo)
      : [],
    customModifiers: Array.isArray(obj.customModifiers)
      ? (obj.customModifiers as CustomModifier[]).filter(isCustomModifier)
      : [],
    layers,
  };
}

function normalizeLayer(raw: Record<string, unknown>): Layer {
  // C++ accepts either `trigger` (string | string[]) or `triggers`. Coerce to array.
  let triggers: string[] = [];
  if (Array.isArray(raw.triggers)) {
    triggers = (raw.triggers as unknown[]).filter((t): t is string => typeof t === "string");
  } else if (Array.isArray(raw.trigger)) {
    triggers = (raw.trigger as unknown[]).filter((t): t is string => typeof t === "string");
  } else if (typeof raw.trigger === "string") {
    triggers = [raw.trigger];
  }

  return {
    name: typeof raw.name === "string" ? raw.name : "Layer",
    triggers,
    mappings: isStringRecord(raw.mappings) ? raw.mappings : {},
    shiftMappings: Array.isArray(raw.shiftMappings)
      ? (raw.shiftMappings as ShiftMapping[]).filter(isShiftMapping)
      : undefined,
  };
}

function isStringRecord(v: unknown): v is Record<string, string> {
  if (!v || typeof v !== "object") return false;
  return Object.values(v as Record<string, unknown>).every((val) => typeof val === "string");
}

function isNoModCombo(v: unknown): v is NoModCombo {
  if (!v || typeof v !== "object") return false;
  const o = v as NoModCombo;
  return typeof o.key === "string" && typeof o.output === "string";
}

function isShiftMapping(v: unknown): v is ShiftMapping {
  if (!v || typeof v !== "object") return false;
  const o = v as ShiftMapping;
  return typeof o.key === "string" && typeof o.output === "string";
}

function isCustomModifier(v: unknown): v is CustomModifier {
  if (!v || typeof v !== "object") return false;
  const o = v as CustomModifier;
  return typeof o.key === "string" && typeof o.modifierName === "string";
}

export const useConfigStore = create<ConfigState>()(
  persist(
    (set, get) => ({
      config: defaultConfig,
      currentLayerIndex: -1,
      selectedKey: null,

      setConfig: (config) => set({ config: normalizeImport(config) }),

      setCurrentLayer: (index) => set({ currentLayerIndex: index, selectedKey: null }),
      setSelectedKey: (key) => set({ selectedKey: key }),

      setDisableCapsLock: (value) =>
        set((s) => ({ config: { ...s.config, disableCapsLock: value } })),

      setKeyMapping: (layerIndex, key, target) =>
        set((s) => {
          if (layerIndex === -1) {
            return {
              config: {
                ...s.config,
                remapping: { ...(s.config.remapping ?? {}), [key]: target },
              },
            };
          }
          const layers = [...(s.config.layers ?? [])];
          const layer = layers[layerIndex];
          if (!layer) return s;
          layers[layerIndex] = {
            ...layer,
            mappings: { ...layer.mappings, [key]: target },
          };
          return { config: { ...s.config, layers } };
        }),

      clearKeyMapping: (layerIndex, key) =>
        set((s) => {
          if (layerIndex === -1) {
            const remapping = { ...(s.config.remapping ?? {}) };
            delete remapping[key];
            return { config: { ...s.config, remapping } };
          }
          const layers = [...(s.config.layers ?? [])];
          const layer = layers[layerIndex];
          if (!layer) return s;
          const mappings = { ...layer.mappings };
          delete mappings[key];
          layers[layerIndex] = { ...layer, mappings };
          return { config: { ...s.config, layers } };
        }),

      setShiftMapping: (layerIndex, key, output, shift = true) =>
        set((s) => {
          if (layerIndex === -1) return s;
          const layers = [...(s.config.layers ?? [])];
          const layer = layers[layerIndex];
          if (!layer) return s;
          const shiftMappings = [...(layer.shiftMappings ?? [])];
          const existing = shiftMappings.findIndex((m) => m.key === key);
          const entry: ShiftMapping = { key, output, shift };
          if (existing >= 0) shiftMappings[existing] = entry;
          else shiftMappings.push(entry);
          layers[layerIndex] = { ...layer, shiftMappings };
          return { config: { ...s.config, layers } };
        }),

      clearShiftMapping: (layerIndex, key) =>
        set((s) => {
          if (layerIndex === -1) return s;
          const layers = [...(s.config.layers ?? [])];
          const layer = layers[layerIndex];
          if (!layer) return s;
          const shiftMappings = (layer.shiftMappings ?? []).filter((m) => m.key !== key);
          layers[layerIndex] = { ...layer, shiftMappings };
          return { config: { ...s.config, layers } };
        }),

      addLayer: (layer) =>
        set((s) => ({
          config: { ...s.config, layers: [...(s.config.layers ?? []), layer] },
        })),

      updateLayer: (index, partial) =>
        set((s) => {
          const layers = [...(s.config.layers ?? [])];
          if (!layers[index]) return s;
          layers[index] = { ...layers[index], ...partial };
          return { config: { ...s.config, layers } };
        }),

      deleteLayer: (index) =>
        set((s) => {
          const layers = [...(s.config.layers ?? [])];
          if (!layers[index]) return s;
          layers.splice(index, 1);
          let newLayerIndex = s.currentLayerIndex;
          if (s.currentLayerIndex === index) newLayerIndex = -1;
          else if (s.currentLayerIndex > index) newLayerIndex = s.currentLayerIndex - 1;
          return {
            config: { ...s.config, layers },
            currentLayerIndex: newLayerIndex,
          };
        }),

      setNoModCombo: (combo) =>
        set((s) => {
          const list = [...(s.config.noModCombos ?? [])];
          const existing = list.findIndex((c) => c.key === combo.key);
          if (existing >= 0) list[existing] = combo;
          else list.push(combo);
          return { config: { ...s.config, noModCombos: list } };
        }),

      removeNoModCombo: (key) =>
        set((s) => ({
          config: {
            ...s.config,
            noModCombos: (s.config.noModCombos ?? []).filter((c) => c.key !== key),
          },
        })),

      setCustomModifier: (mod) =>
        set((s) => {
          const list = [...(s.config.customModifiers ?? [])];
          const existing = list.findIndex((m) => m.modifierName === mod.modifierName);
          if (existing >= 0) list[existing] = mod;
          else list.push(mod);
          return { config: { ...s.config, customModifiers: list } };
        }),

      removeCustomModifier: (modifierName) =>
        set((s) => ({
          config: {
            ...s.config,
            customModifiers: (s.config.customModifiers ?? []).filter(
              (m) => m.modifierName !== modifierName,
            ),
          },
        })),

      hasBaseModifiers: () => {
        const remapping = get().config.remapping ?? {};
        return Object.values(remapping).some((target) => STANDARD_MODIFIERS.has(target));
      },
    }),
    {
      // Bump version when the persisted shape changes. The previous store wrote
      // strictMode/debug fields; on first load with this new code we ignore them.
      name: "keyflow-config-storage",
      version: 2,
    },
  ),
);
