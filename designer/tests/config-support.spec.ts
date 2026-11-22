import { test, expect } from "@playwright/test";
import { useConfigStore } from "../lib/store";
import { normalizeConfig, resolveConfig, availableTriggers } from "../lib/config";
import { readFileSync } from "node:fs";
import path from "node:path";
import { ALL_KEY_CODES, canonicalKey } from "../lib/allKeys";

test.beforeEach(() => {
  useConfigStore.getState().setConfig({ version: "1.0", remapping: {}, layers: [] });
});

test("import preserves platform overrides, empty replacements and trigger variants", () => {
  const config = {
    version: "1.0",
    remapping: { A: "B", _comment: "Keep me" },
    layers: [{ name: "Navigation", triggers: ["LALT"], mappings: { H: "Left" } }],
    platformOverrides: {
      linux: { disableCapsLock: false, layers: [], noModCombos: [] },
      windows: { remapping: { A: "C" }, customModifiers: [] },
      mac: { layers: [{ name: "Mac", triggers: ["RWIN"], mappings: { J: "Down" } }] },
    },
  };
  useConfigStore.getState().setConfig(config);
  expect(useConfigStore.getState().config.platformOverrides).toEqual(config.platformOverrides);
  expect(useConfigStore.getState().config.remapping).toEqual(config.remapping);

  useConfigStore.getState().setConfig({
    version: "1.0",
    layers: [{ name: "Legacy", trigger: ["LALT", "RALT"], mappings: {} }],
  } as never);
  expect(useConfigStore.getState().config.layers?.[0].triggers).toEqual(["LALT", "RALT"]);
});

test("maintained config round-trips all platforms without materializing absent fields", () => {
  const source = JSON.parse(readFileSync(path.resolve(__dirname, "../../src/config.json"), "utf8"));
  const expected = structuredClone(source);
  for (const config of [expected, ...Object.values(expected.platformOverrides ?? {})] as Array<{ layers?: Array<Record<string, unknown>> }>) {
    for (const layer of config.layers ?? []) {
      if (layer.trigger !== undefined) {
        layer.triggers = Array.isArray(layer.trigger) ? layer.trigger : [layer.trigger];
        delete layer.trigger;
      }
    }
  }
  useConfigStore.getState().setConfig(source);
  expect(useConfigStore.getState().config).toEqual(expected);
});

test("platform edits merge remaps, replace arrays and restore live inheritance", () => {
  const source = {
    version: "1.0", remapping: { A: "B", CapsLock: "LeftCtrl" },
    noModCombos: [{ key: "1", output: "1", shift: true }],
    layers: [{ name: "Base nav", triggers: ["LCTRL"], mappings: { H: "Left" } }],
    platformOverrides: { windows: { remapping: { A: "C" } }, linux: { layers: [] } },
  };
  useConfigStore.getState().setConfig(source);
  useConfigStore.getState().setPlatform("windows");
  useConfigStore.getState().setKeyMapping(-1, "A", "D");
  useConfigStore.getState().setKeyMapping(0, "J", "Down");
  useConfigStore.getState().setNoModCombo({ key: "2", output: "LeftBracket" });
  let document = useConfigStore.getState().config;
  expect(document.remapping).toEqual(source.remapping);
  expect(document.platformOverrides?.windows?.remapping).toEqual({ A: "D" });
  expect(document.layers).toEqual(source.layers);
  expect(document.platformOverrides?.windows?.layers?.[0].mappings).toEqual({ H: "Left", J: "Down" });
  expect(resolveConfig(document, "windows").noModCombos).toHaveLength(2);
  expect(resolveConfig(document, "linux").layers).toEqual([]);
  useConfigStore.getState().clearKeyMapping(-1, "A");
  useConfigStore.getState().resetOverride("layers");
  document = useConfigStore.getState().config;
  expect(resolveConfig(document, "windows").remapping).toEqual(source.remapping);
  expect(resolveConfig(document, "windows").layers).toEqual(source.layers);
  useConfigStore.getState().setPlatform("shared");
  useConfigStore.getState().setKeyMapping(-1, "A", "E");
  expect(resolveConfig(useConfigStore.getState().config, "windows").remapping?.A).toBe("E");
});

test("trigger choices include physical-name aliases, PRINT and custom modifiers", () => {
  const config = { version: "1.0", remapping: { CapsLock: "LeftCtrl", Enter: "RAlt", PrintScreen: "Print" },
    customModifiers: [{ key: "Space", modifierName: "SPACE_MOD" }] };
  expect(availableTriggers(config)).toEqual(["LCTRL", "RALT", "PRINT", "SPACE_MOD"]);
  expect(normalizeConfig({ ...config, layers: [{ name: "Custom", triggers: "SPACE_MOD", mappings: {} }] }).layers?.[0].triggers)
    .toEqual(["SPACE_MOD"]);
});

test("designer key choices and imports stay aligned with the runtime key namespace", () => {
  const mapper = readFileSync(path.resolve(__dirname, "../../src/config/KeyNameMapper.h"), "utf8");
  const names = [...mapper.matchAll(/\{"([^"]+)",\s*SC_/g)].map((match) => match[1]);
  for (const name of names) expect(canonicalKey(name), `Runtime key ${name} must be recognized`).toBeDefined();
  for (const name of ALL_KEY_CODES) expect(names, `Designer key ${name} must be supported by runtime`).toContain(name);
  const remapping = Object.fromEntries(names.map((name) => [name, name]));
  expect(normalizeConfig({ version: "1.0", remapping }).remapping).toEqual(remapping);
});

test("renaming combos and custom modifiers replaces rows and retargets layers", () => {
  useConfigStore.getState().setConfig({
    version: "1.0", noModCombos: [{ key: "1", output: "1", description: "keep" }],
    customModifiers: [{ key: "Space", modifierName: "SPACE_MOD" }],
    layers: [{ name: "Custom", triggers: ["SPACE_MOD", "LALT"], mappings: {} }],
  });
  useConfigStore.getState().setNoModCombo({ key: "2", output: "1", description: "keep" }, "1");
  useConfigStore.getState().setCustomModifier({ key: "Space", modifierName: "SPACE_NEW" }, "SPACE_MOD");
  expect(useConfigStore.getState().config.noModCombos).toEqual([{ key: "2", output: "1", description: "keep" }]);
  expect(useConfigStore.getState().config.customModifiers).toHaveLength(1);
  expect(useConfigStore.getState().config.layers?.[0].triggers).toEqual(["SPACE_NEW", "LALT"]);
  useConfigStore.getState().removeCustomModifier("SPACE_NEW");
  expect(useConfigStore.getState().config.layers?.[0].triggers).toEqual(["LALT"]);
});

test("switching output kind clears conflicting bindings and preserves shift descriptions", () => {
  useConfigStore.getState().setConfig({ version: "1.0", layers: [
    { name: "Symbols", triggers: ["LALT"], mappings: {}, shiftMappings: [{ key: "A", output: "1", description: "symbol" }] },
  ] });
  useConfigStore.getState().setShiftMapping(0, "A", "2", false);
  expect(useConfigStore.getState().config.layers?.[0].shiftMappings?.[0])
    .toEqual({ key: "A", output: "2", shift: false, description: "symbol" });
  useConfigStore.getState().setKeyMapping(0, "A", "F1");
  expect(useConfigStore.getState().config.layers?.[0].shiftMappings).toEqual([]);
  useConfigStore.getState().setShiftMapping(0, "A", "3");
  expect(useConfigStore.getState().config.layers?.[0].mappings).toEqual({});
});

test("shared modifier changes retarget platform layers only where modifiers are inherited", () => {
  const layer = { name: "Custom", triggers: ["SPACE_MOD"], mappings: { A: "B" } };
  useConfigStore.getState().setConfig({
    version: "1.0", customModifiers: [{ key: "Space", modifierName: "SPACE_MOD" }], layers: [layer],
    platformOverrides: {
      linux: { layers: [layer] },
      windows: { customModifiers: [{ key: "Tab", modifierName: "SPACE_MOD" }], layers: [layer] },
      mac: { customModifiers: [{ key: "Tab", modifierName: "SPACE_MOD" }] },
    },
  });

  useConfigStore.getState().setCustomModifier({ key: "Space", modifierName: "NEW_MOD" }, "SPACE_MOD");
  expect(resolveConfig(useConfigStore.getState().config, "linux").layers?.[0].triggers).toEqual(["NEW_MOD"]);
  expect(resolveConfig(useConfigStore.getState().config, "windows").layers?.[0].triggers).toEqual(["SPACE_MOD"]);
  expect(resolveConfig(useConfigStore.getState().config, "mac").layers?.[0].triggers).toEqual(["SPACE_MOD"]);
  useConfigStore.getState().removeCustomModifier("NEW_MOD");
  expect(resolveConfig(useConfigStore.getState().config, "linux").layers).toEqual([]);
  expect(() => normalizeConfig(useConfigStore.getState().config)).not.toThrow();
});

test("resetting coupled platform sections is atomic and requires opting in to both", () => {
  for (const field of ["layers", "customModifiers"] as const) {
    useConfigStore.getState().setConfig({
      version: "1.0", customModifiers: [{ key: "Space", modifierName: "SPACE_MOD" }],
      layers: [{ name: "Custom", triggers: ["SPACE_MOD"], mappings: {} }],
    });
    useConfigStore.getState().setPlatform("linux");
    useConfigStore.getState().setCustomModifier({ key: "Space", modifierName: "SPACE_NEW" }, "SPACE_MOD");
    const before = useConfigStore.getState().config;
    expect(useConfigStore.getState().resetOverride(field)).toBe(false);
    expect(useConfigStore.getState().config).toEqual(before);
    expect(useConfigStore.getState().resetOverride(field, true)).toBe(true);
    expect(resolveConfig(useConfigStore.getState().config, "linux").layers?.[0].triggers).toEqual(["SPACE_MOD"]);
    expect(() => normalizeConfig(useConfigStore.getState().config)).not.toThrow();
  }
});

test("shared alias source spellings keep exact platform remapping overrides intact", () => {
  useConfigStore.getState().setConfig({
    version: "1.0", remapping: { Return: "A" },
    platformOverrides: { linux: { remapping: { Return: "B" } } },
  });
  useConfigStore.getState().setKeyMapping(-1, "Enter", "C");
  expect(useConfigStore.getState().config.remapping).toEqual({ Return: "C" });
  expect(resolveConfig(useConfigStore.getState().config, "linux").remapping).toEqual({ Return: "B" });
  useConfigStore.getState().setPlatform("linux");
  useConfigStore.getState().setKeyMapping(-1, "Enter", "D");
  expect(resolveConfig(useConfigStore.getState().config, "linux").remapping).toEqual({ Return: "D" });
});

test("invalid imports fail atomically instead of silently discarding data", () => {
  const original = useConfigStore.getState().config;
  for (const invalid of [
    { version: 1 }, { version: "1.0", platformOverrides: [] },
    { version: "1.0", platformOverrides: { linux: null } },
    { version: "1.0", layers: [null] },
    { version: "1.0", remapping: { A: "Unknown" } },
    { version: "1.0", noModCombos: [{ key: "A", output: "B", shift: "yes" }] },
    { version: "1.0", layers: [{ name: "Bad", triggers: ["CAPS"], mappings: {} }] },
  ]) {
    expect(() => useConfigStore.getState().setConfig(invalid)).toThrow();
    expect(useConfigStore.getState().config).toEqual(original);
  }
});
