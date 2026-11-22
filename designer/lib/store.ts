import { create } from "zustand";
import { persist } from "zustand/middleware";
import { CustomModifier, EditPlatform, KeyFlowConfig, Layer, NoModCombo, PlatformOverride } from "./types";
import { availableTriggers, normalizeConfig, OverrideField, OVERRIDE_FIELDS, resolveConfig } from "./config";
import { canonicalKey } from "./allKeys";

type ConfigState = {
  config: KeyFlowConfig;
  platform: EditPlatform;
  currentLayerIndex: number;
  selectedKey: string | null;
  setConfig: (config: unknown) => void;
  setPlatform: (platform: EditPlatform) => void;
  resetOverride: (field?: OverrideField, includeDependencies?: boolean) => boolean;
  applyPlatformPreset: () => void;
  setName: (name: string) => void;
  setKeyMapping: (layerIndex: number, key: string, target: string) => void;
  clearKeyMapping: (layerIndex: number, key: string) => void;
  setShiftMapping: (layerIndex: number, key: string, output: string, shift?: boolean) => void;
  clearShiftMapping: (layerIndex: number, key: string) => void;
  addLayer: (layer: Layer) => void;
  updateLayer: (index: number, partial: Partial<Layer>) => void;
  deleteLayer: (index: number) => void;
  setNoModCombo: (combo: NoModCombo, previousKey?: string) => void;
  removeNoModCombo: (key: string) => void;
  setCustomModifier: (mod: CustomModifier, previousName?: string) => void;
  removeCustomModifier: (modifierName: string) => void;
  setCurrentLayer: (index: number) => void;
  setSelectedKey: (key: string | null) => void;
  setDisableCapsLock: (value: boolean) => void;
  hasBaseModifiers: () => boolean;
};

const defaultConfig: KeyFlowConfig = {
  version: "1.0", name: "My Layout", disableCapsLock: false,
  remapping: {}, noModCombos: [], customModifiers: [], layers: [],
};

function removeMapping(mappings: Record<string, string> | undefined, key: string) {
  return Object.fromEntries(Object.entries(mappings ?? {}).filter(([source]) =>
    source.startsWith("_") || canonicalKey(source) !== canonicalKey(key)));
}

function updateInheritedModifierLayers(document: KeyFlowConfig, update: (layers: Layer[]) => Layer[]) {
  return Object.fromEntries(Object.entries(document.platformOverrides ?? {}).map(([platform, override]) => [
    platform,
    override.customModifiers === undefined && override.layers !== undefined
      ? { ...override, layers: update(override.layers) }
      : override.customModifiers !== undefined && override.layers === undefined
        ? { ...override, layers: document.layers ?? [] } : override,
  ]));
}

export const useConfigStore = create<ConfigState>()(
  persist(
    (set, get) => {
      // Edits target the effective config. Only changed fields are written to
      // the selected override; inherited arrays are copied on first edit.
      const edit = (update: (state: ConfigState) => Partial<ConfigState>) => set((state) => {
        const effective = resolveConfig(state.config, state.platform);
        const result = update({ ...state, config: effective });
        if (!result.config) return result;
        if (state.platform === "shared") {
          return { ...result, config: { ...result.config, platformOverrides: result.config.platformOverrides ?? state.config.platformOverrides } };
        }
        const override: PlatformOverride = { ...state.config.platformOverrides?.[state.platform] };
        for (const field of OVERRIDE_FIELDS) {
          if (result.config[field] !== effective[field]) {
            Object.assign(override, { [field]: result.config[field] });
          }
        }
        return {
          ...result,
          config: {
            ...state.config,
            platformOverrides: { ...state.config.platformOverrides, [state.platform]: override },
          },
        };
      });

      const updateLayer = (index: number, update: (layer: Layer) => Layer) => edit((state) => {
        const layers = [...state.config.layers ?? []];
        if (!layers[index]) return {};
        layers[index] = update(layers[index]);
        return { config: { ...state.config, layers } };
      });

      return {
        config: defaultConfig, platform: "shared", currentLayerIndex: -1, selectedKey: null,
        setConfig: (raw) => set({
          config: normalizeConfig(raw), currentLayerIndex: -1, selectedKey: null, platform: "shared",
        }),
        setName: (name) => set((state) => ({ config: { ...state.config, name } })),
        setPlatform: (platform) => set({ platform, currentLayerIndex: -1, selectedKey: null }),
        setCurrentLayer: (currentLayerIndex) => set({ currentLayerIndex, selectedKey: null }),
        setSelectedKey: (selectedKey) => set({ selectedKey }),
        resetOverride: (field, includeDependencies = false) => {
          const state = get();
          if (state.platform === "shared") return true;
          const platformOverrides = { ...state.config.platformOverrides };
          if (field) {
            const override = { ...platformOverrides[state.platform] };
            delete override[field];
            if (includeDependencies && (field === "layers" || field === "customModifiers")) {
              delete override.layers;
              delete override.customModifiers;
            }
            platformOverrides[state.platform] = override;
          } else delete platformOverrides[state.platform];
          const config = { ...state.config, platformOverrides };
          try {
            normalizeConfig(config);
          } catch {
            return false;
          }
          set({ config, currentLayerIndex: -1, selectedKey: null });
          return true;
        },
        applyPlatformPreset: () => {
          const { platform } = get();
          if (platform === "shared") return;
          const target = platform === "windows" ? "RightCtrl" : "LeftWin";
          get().setKeyMapping(-1, "PrintScreen", target);
        },
        setDisableCapsLock: (disableCapsLock) => edit((state) => ({
          config: { ...state.config, disableCapsLock },
        })),
        setKeyMapping: (index, key, target) => {
          if (index === -1) {
            set((state) => {
              if (state.platform === "shared") {
                const source = Object.keys(state.config.remapping ?? {}).find((source) =>
                  canonicalKey(source) === canonicalKey(key)) ?? key;
                return { config: { ...state.config, remapping: { ...removeMapping(state.config.remapping, key), [source]: target } } };
              }
              const override = { ...state.config.platformOverrides?.[state.platform] };
              const source = Object.keys(state.config.remapping ?? {}).find((source) =>
                canonicalKey(source) === canonicalKey(key)) ?? key;
              override.remapping = { ...removeMapping(override.remapping, key), [source]: target };
              return { config: { ...state.config, platformOverrides: {
                ...state.config.platformOverrides, [state.platform]: override,
              } } };
            });
          } else updateLayer(index, (layer) => ({
            ...layer, mappings: { ...removeMapping(layer.mappings, key), [key]: target },
            shiftMappings: layer.shiftMappings?.filter((mapping) => canonicalKey(mapping.key) !== canonicalKey(key)),
          }));
        },
        clearKeyMapping: (index, key) => {
          if (index === -1) {
            set((state) => {
              if (state.platform === "shared") {
                return { config: { ...state.config, remapping: removeMapping(state.config.remapping, key) } };
              }
              const override = { ...state.config.platformOverrides?.[state.platform] };
              override.remapping = removeMapping(override.remapping, key);
              return { config: { ...state.config, platformOverrides: {
                ...state.config.platformOverrides, [state.platform]: override,
              } } };
            });
          } else updateLayer(index, (layer) => ({ ...layer, mappings: removeMapping(layer.mappings, key) }));
        },
        setShiftMapping: (index, key, output, shift = true) => {
          if (index < 0) return;
          updateLayer(index, (layer) => {
            const previous = layer.shiftMappings?.find((mapping) => canonicalKey(mapping.key) === canonicalKey(key));
            return {
              ...layer, mappings: removeMapping(layer.mappings, key),
              shiftMappings: [
                ...layer.shiftMappings?.filter((mapping) => canonicalKey(mapping.key) !== canonicalKey(key)) ?? [],
                { ...previous, key, output, shift },
              ],
            };
          });
        },
        clearShiftMapping: (index, key) => updateLayer(index, (layer) => ({
          ...layer, shiftMappings: layer.shiftMappings?.filter((mapping) => canonicalKey(mapping.key) !== canonicalKey(key)),
        })),
        addLayer: (layer) => edit((state) => ({
          config: { ...state.config, layers: [...state.config.layers ?? [], layer] },
        })),
        updateLayer: (index, partial) => updateLayer(index, (layer) => ({ ...layer, ...partial })),
        deleteLayer: (index) => edit((state) => ({
          config: { ...state.config, layers: state.config.layers?.filter((_, i) => i !== index) },
          currentLayerIndex: state.currentLayerIndex === index ? -1
            : state.currentLayerIndex > index ? state.currentLayerIndex - 1 : state.currentLayerIndex,
          selectedKey: null,
        })),
        setNoModCombo: (combo, previousKey = combo.key) => edit((state) => {
          const combos = [...state.config.noModCombos ?? []];
          const index = combos.findIndex((entry) => entry.key === previousKey);
          if (combos.some((entry, i) => i !== index && canonicalKey(entry.key) === canonicalKey(combo.key))) return {};
          if (index < 0) combos.push(combo);
          else combos[index] = combo;
          return { config: { ...state.config, noModCombos: combos } };
        }),
        removeNoModCombo: (key) => edit((state) => ({
          config: { ...state.config, noModCombos: state.config.noModCombos?.filter((combo) => combo.key !== key) },
        })),
        setCustomModifier: (mod, previousName = mod.modifierName) => edit((state) => {
          const mods = [...state.config.customModifiers ?? []];
          const index = mods.findIndex((entry) => entry.modifierName === previousName);
          if (!mod.modifierName.trim() || mods.some((entry, i) => i !== index && entry.modifierName === mod.modifierName)) return {};
          if (index < 0) {
            if (mods.length >= 23) return {};
            mods.push(mod);
          } else mods[index] = mod;
          const rename = (layers: Layer[]) => layers.map((layer) => ({
            ...layer, triggers: layer.triggers.map((trigger) => trigger === previousName ? mod.modifierName : trigger),
          }));
          return { config: {
            ...state.config, customModifiers: mods,
            ...(previousName !== mod.modifierName ? {
              layers: rename(state.config.layers ?? []),
              ...(state.platform === "shared" ? { platformOverrides: updateInheritedModifierLayers(get().config, rename) } : {}),
            } : {}),
          } };
        }),
        removeCustomModifier: (name) => edit((state) => {
          const remove = (layers: Layer[]) => layers.filter((layer) => layer.triggers.some((trigger) => trigger !== name))
            .map((layer) => ({ ...layer, triggers: layer.triggers.filter((trigger) => trigger !== name) }));
          return {
            config: {
              ...state.config, customModifiers: state.config.customModifiers?.filter((mod) => mod.modifierName !== name),
              layers: remove(state.config.layers ?? []),
              ...(state.platform === "shared" ? { platformOverrides: updateInheritedModifierLayers(get().config, remove) } : {}),
            },
            currentLayerIndex: -1, selectedKey: null,
          };
        }),
        hasBaseModifiers: () => availableTriggers(resolveConfig(get().config, get().platform)).length > 0,
      };
    },
    {
      name: "keyflow-config-storage",
      version: 2,
      partialize: (state) => ({ config: state.config }),
      merge: (persisted, current) => {
        const saved = persisted as { config?: unknown };
        try {
          return { ...current, config: saved?.config ? normalizeConfig(saved.config) : current.config };
        } catch {
          return current;
        }
      },
    },
  ),
);

export function useEditingConfig(): KeyFlowConfig {
  const config = useConfigStore((state) => state.config);
  const platform = useConfigStore((state) => state.platform);
  return resolveConfig(config, platform);
}
