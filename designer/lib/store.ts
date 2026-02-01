import { create } from "zustand";
import { persist } from "zustand/middleware";
import { KeyFlowConfig, Layer } from "./types";

type ConfigState = {
  config: KeyFlowConfig;
  currentLayerIndex: number;
  selectedKey: string | null;

  // Actions
  setConfig: (config: KeyFlowConfig) => void;
  setCurrentLayer: (index: number) => void;
  setSelectedKey: (key: string | null) => void;
  addLayer: (layer: Layer) => void;
  updateLayer: (index: number, layer: Partial<Layer>) => void;
  deleteLayer: (index: number) => void;
  setKeyMapping: (layerIndex: number, key: string, value: string) => void;
  clearKeyMapping: (layerIndex: number, key: string) => void;
  setShiftMapping: (layerIndex: number, key: string, output: string) => void;
  clearShiftMapping: (layerIndex: number, key: string) => void;
  hasBaseModifiers: () => boolean;
};

const defaultConfig: KeyFlowConfig = {
  version: "1.0",
  name: "My Custom Layout",
  strictMode: false,
  remapping: {},
  layers: [],
  noModCombos: [],
};

export const useConfigStore = create<ConfigState>()(
  persist(
    (set, get) => ({
      config: defaultConfig,
      currentLayerIndex: -1, // -1 means BASE layer (remapping)
      selectedKey: null,

      setConfig: (config) => set({ config }),

      hasBaseModifiers: () => {
        const { config } = get();
        const baseRemapping = config.remapping || {};
        const modifiers = ["RALT", "LALT", "RCTRL", "LCTRL", "RSHIFT", "LSHIFT", "RWIN", "LWIN", "CAPS"];
        return Object.values(baseRemapping).some((target) => modifiers.includes(target));
      },

      setCurrentLayer: (index) =>
        set({ currentLayerIndex: index, selectedKey: null }),

      setSelectedKey: (key) => set({ selectedKey: key }),

      addLayer: (layer) =>
        set((state) => ({
          config: {
            ...state.config,
            layers: [...(state.config.layers || []), layer],
          },
        })),

      updateLayer: (index, layerUpdate) =>
    set((state) => {
      const layers = [...(state.config.layers || [])];
      layers[index] = { ...layers[index], ...layerUpdate };
      return {
        config: {
          ...state.config,
          layers,
        },
      };
    }),

      deleteLayer: (index) =>
        set((state) => {
          const layers = [...(state.config.layers || [])];
          layers.splice(index, 1);

          // Fix: Properly update currentLayerIndex after deletion
          let newLayerIndex = state.currentLayerIndex;
          if (state.currentLayerIndex === index) {
            // If deleting current layer, go to BASE
            newLayerIndex = -1;
          } else if (state.currentLayerIndex > index) {
            // If current layer is after deleted layer, shift index down
            newLayerIndex = state.currentLayerIndex - 1;
          }
          // If current layer is before deleted layer, index stays the same

          return {
            config: {
              ...state.config,
              layers,
            },
            currentLayerIndex: newLayerIndex,
          };
        }),

      setKeyMapping: (layerIndex, key, value) =>
    set((state) => {
      if (layerIndex === -1) {
        // BASE layer (remapping)
        return {
          config: {
            ...state.config,
            remapping: {
              ...state.config.remapping,
              [key]: value,
            },
          },
        };
      } else {
        // Layer
        const layers = [...(state.config.layers || [])];
        const layer = layers[layerIndex];
        layers[layerIndex] = {
          ...layer,
          mappings: {
            ...layer.mappings,
            [key]: value,
          },
        };
        return {
          config: {
            ...state.config,
            layers,
          },
        };
      }
    }),

      clearKeyMapping: (layerIndex, key) =>
    set((state) => {
      if (layerIndex === -1) {
        // BASE layer
        const remapping = { ...state.config.remapping };
        delete remapping[key];
        return {
          config: {
            ...state.config,
            remapping,
          },
        };
      } else {
        // Layer
        const layers = [...(state.config.layers || [])];
        const layer = layers[layerIndex];
        const mappings = { ...layer.mappings };
        delete mappings[key];
        layers[layerIndex] = {
          ...layer,
          mappings,
        };
        return {
          config: {
            ...state.config,
            layers,
          },
        };
      }
    }),

      setShiftMapping: (layerIndex, key, output) =>
    set((state) => {
      if (layerIndex === -1) {
        // Shift mappings only work in layers, not BASE
        return state;
      }

      const layers = [...(state.config.layers || [])];
      const layer = layers[layerIndex];
      const shiftMappings = [...(layer.shiftMappings || [])];

      // Remove existing shift mapping for this key if any
      const existingIndex = shiftMappings.findIndex((m) => m.key === key);
      if (existingIndex >= 0) {
        shiftMappings[existingIndex] = { key, output, shift: true };
      } else {
        shiftMappings.push({ key, output, shift: true });
      }

      layers[layerIndex] = {
        ...layer,
        shiftMappings,
      };

      return {
        config: {
          ...state.config,
          layers,
        },
      };
    }),

      clearShiftMapping: (layerIndex, key) =>
        set((state) => {
          if (layerIndex === -1) {
            return state;
          }

          const layers = [...(state.config.layers || [])];
          const layer = layers[layerIndex];
          const shiftMappings = (layer.shiftMappings || []).filter((m) => m.key !== key);

          layers[layerIndex] = {
            ...layer,
            shiftMappings,
          };

          return {
            config: {
              ...state.config,
              layers,
            },
          };
        }),
    }),
    {
      name: "keyflow-config-storage",
      version: 1,
    }
  )
);
