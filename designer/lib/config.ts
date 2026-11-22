import { EditPlatform, KeyFlowConfig, Layer, PlatformOverride } from "./types";
import { canonicalKey } from "./allKeys";
import { TRIGGER_ALIAS } from "./keyDisplay";

export const PLATFORMS = { linux: "Linux", windows: "Windows", mac: "macOS" } as const;
export const OVERRIDE_FIELDS = ["remapping", "noModCombos", "customModifiers", "layers", "disableCapsLock"] as const;
export type OverrideField = typeof OVERRIDE_FIELDS[number];

function object(value: unknown, context: string): Record<string, unknown> {
  if (!value || typeof value !== "object" || Array.isArray(value)) {
    throw new Error(`${context} must be an object`);
  }
  return value as Record<string, unknown>;
}

function string(value: unknown, context: string): string {
  if (typeof value !== "string") throw new Error(`${context} must be a string`);
  return value;
}

function mapping(value: unknown, context: string): Record<string, string> {
  const result: Record<string, string> = {};
  for (const [key, target] of Object.entries(object(value, context))) {
    result[key] = string(target, `${context}.${key}`);
    if (!key.startsWith("_") && (!canonicalKey(key) || !canonicalKey(result[key]))) {
      throw new Error(`${context}.${key}: unknown key name`);
    }
  }
  return result;
}

function entries(value: unknown, context: string): Record<string, unknown>[] {
  if (!Array.isArray(value)) throw new Error(`${context} must be an array`);
  return value.map((entry, i) => object(entry, `${context}[${i}]`));
}

function optionalFields(raw: Record<string, unknown>, context: string) {
  for (const field of ["shift", "blockOutput", "disableCapsLock"]) {
    if (raw[field] !== undefined && typeof raw[field] !== "boolean") {
      throw new Error(`${context}.${field} must be a boolean`);
    }
  }
  if (raw.description !== undefined) string(raw.description, `${context}.description`);
}

function normalizeLayer(raw: Record<string, unknown>, context: string): Layer {
  const trigger = raw.trigger !== undefined ? raw.trigger : raw.triggers;
  const triggers = typeof trigger === "string" ? [trigger] : trigger;
  if (!Array.isArray(triggers) || !triggers.length || !triggers.every((t) => typeof t === "string")) {
    throw new Error(`${context}.triggers must contain at least one modifier`);
  }
  const name = string(raw.name, `${context}.name`);
  if (!name.trim()) throw new Error(`${context}.name cannot be empty`);
  return {
    name, triggers,
    mappings: mapping(raw.mappings, `${context}.mappings`),
    ...(raw.shiftMappings !== undefined ? {
      shiftMappings: normalizeOutputs(raw.shiftMappings, `${context}.shiftMappings`),
    } : {}),
  };
}

function normalizeOutputs(value: unknown, context: string) {
  return entries(value, context).map((raw, i) => {
    optionalFields(raw, `${context}[${i}]`);
    const key = string(raw.key, `${context}[${i}].key`);
    const output = string(raw.output, `${context}[${i}].output`);
    if (!canonicalKey(key) || !canonicalKey(output)) throw new Error(`${context}[${i}]: unknown key name`);
    return {
      key, output,
      ...(raw.shift !== undefined ? { shift: raw.shift as boolean } : {}),
      ...(raw.description !== undefined ? { description: raw.description as string } : {}),
    };
  });
}

function normalizeOverride(raw: Record<string, unknown>, context: string): PlatformOverride {
  optionalFields(raw, context);
  const result: PlatformOverride = {};
  if (raw.disableCapsLock !== undefined) result.disableCapsLock = raw.disableCapsLock as boolean;
  if (raw.remapping !== undefined) result.remapping = mapping(raw.remapping, `${context}.remapping`);
  if (raw.noModCombos !== undefined) result.noModCombos = normalizeOutputs(raw.noModCombos, `${context}.noModCombos`);
  if (raw.layers !== undefined) result.layers = entries(raw.layers, `${context}.layers`)
    .map((layer, i) => normalizeLayer(layer, `${context}.layers[${i}]`));
  if (raw.customModifiers !== undefined) {
    result.customModifiers = entries(raw.customModifiers, `${context}.customModifiers`).map((mod, i) => {
      optionalFields(mod, `${context}.customModifiers[${i}]`);
      const key = string(mod.key, `${context}.customModifiers[${i}].key`);
      const modifierName = string(mod.modifierName, `${context}.customModifiers[${i}].modifierName`);
      if (!canonicalKey(key) || !modifierName.trim()) throw new Error(`${context}.customModifiers[${i}]: invalid key or modifier name`);
      return { key, modifierName, ...(mod.blockOutput !== undefined ? { blockOutput: mod.blockOutput as boolean } : {}) };
    });
  }
  return result;
}

export function normalizeConfig(value: unknown): KeyFlowConfig {
  const raw = object(value, "Config");
  const config: KeyFlowConfig = {
    version: string(raw.version, "Config.version"),
    ...normalizeOverride(raw, "Config"),
    ...(raw.name !== undefined ? { name: string(raw.name, "Config.name") } : {}),
  };
  if (raw.platformOverrides !== undefined) {
    const overrides = object(raw.platformOverrides, "platformOverrides");
    config.platformOverrides = {};
    for (const platform of Object.keys(PLATFORMS) as Array<keyof typeof PLATFORMS>) {
      if (overrides[platform] !== undefined) {
        config.platformOverrides[platform] = normalizeOverride(
          object(overrides[platform], `platformOverrides.${platform}`), `platformOverrides.${platform}`,
        );
      }
    }
  }
  for (const platform of ["shared", ...Object.keys(PLATFORMS)] as EditPlatform[]) {
    const effective = resolveConfig(config, platform);
    const names = (effective.customModifiers ?? []).map((mod) => mod.modifierName);
    if (names.length > 23 || new Set(names).size !== names.length) {
      throw new Error(`${platform}: custom modifiers must have unique names (maximum 23)`);
    }
    for (const layer of effective.layers ?? []) {
      for (const trigger of layer.triggers) {
        if (!TRIGGER_ALIAS[trigger] && !names.includes(trigger)) {
          throw new Error(`${platform}: unknown trigger modifier '${trigger}' in ${layer.name}`);
        }
      }
    }
  }
  return config;
}

export function resolveConfig(config: KeyFlowConfig, platform: EditPlatform): KeyFlowConfig {
  const { platformOverrides, ...base } = config;
  if (platform === "shared") return base;
  const override = platformOverrides?.[platform];
  if (!override) return base;
  return {
    ...base, ...override,
    remapping: { ...base.remapping, ...override.remapping },
  };
}

export function keyMapping(mappings: Record<string, string> | undefined, key: string): string | undefined {
  return Object.entries(mappings ?? {}).find(([source]) => canonicalKey(source) === canonicalKey(key))?.[1];
}

export function availableTriggers(config: KeyFlowConfig): string[] {
  const physical: Record<string, string> = {
    LeftShift: "LSHIFT", RightShift: "RSHIFT", LeftCtrl: "LCTRL", RightCtrl: "RCTRL",
    LeftAlt: "LALT", RightAlt: "RALT", LeftWin: "LWIN", RightWin: "RWIN", PrintScreen: "PRINT",
  };
  return Array.from(new Set([
    ...Object.entries(config.remapping ?? {}).filter(([key]) => !key.startsWith("_"))
      .map(([, target]) => physical[canonicalKey(target) ?? target]).filter(Boolean),
    ...(config.customModifiers ?? []).map((mod) => mod.modifierName),
  ]));
}
