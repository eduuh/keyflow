import bundledConfig from "../public/eduuh-dh-config.json";
import { normalizeConfig } from "./config";
import type { KeyFlowConfig } from "./types";

const QWERTY_ROWS = [
  "Q W E R T Y U I O P LeftBracket RightBracket Backslash",
  "A S D F G H J K L Semicolon Apostrophe",
  "Z X C V B N M Comma Period Slash",
  "Grave Minus Equals",
];

// US ANSI positions from xkeyboard-config symbols/us; no Caps Lock or AltGr changes.
export const LAYOUT_TEMPLATES = [
  { id: "blank", name: "Blank", description: "Unchanged keys. Build your own mappings." },
  { id: "qwerty", name: "QWERTY", description: "Standard US keyboard." },
  { id: "colemak", name: "Colemak", description: "US ANSI. Keeps familiar shortcut positions.", rows: [
    "Q W F P G J L U Y Semicolon LeftBracket RightBracket Backslash",
    "A R S T D H N E I O Apostrophe",
    "Z X C V B K M Comma Period Slash",
    "Grave Minus Equals",
  ] },
  { id: "colemak-dh", name: "Colemak-DH", description: "US ANSI Angle variant (not matrix).", rows: [
    "Q W F P B J L U Y Semicolon LeftBracket RightBracket Backslash",
    "A R S T G M N E I O Apostrophe",
    "X C D V Z K H Comma Period Slash",
    "Grave Minus Equals",
  ] },
  { id: "dvorak", name: "Dvorak", description: "Standard US Dvorak letters and punctuation.", rows: [
    "Apostrophe Comma Period P Y F G C R L Slash Equals Backslash",
    "A O E U I D H T N S Minus",
    "Semicolon Q J K X B M W V Z",
    "Grave LeftBracket RightBracket",
  ] },
  { id: "workman", name: "Workman", description: "US ANSI. Balanced finger movement.", rows: [
    "Q D R W B J F U P Semicolon LeftBracket RightBracket Backslash",
    "A S H T G Y N E O I Apostrophe",
    "Z X M C V K L Comma Period Slash",
    "Grave Minus Equals",
  ] },
  { id: "eduuh-dh", name: "Eduuh-DH", description: "Maintained layout, layers and platform overrides." },
] as const;

export type LayoutTemplateId = typeof LAYOUT_TEMPLATES[number]["id"];

export function createLayoutTemplate(id: LayoutTemplateId, name?: string): KeyFlowConfig {
  const template = LAYOUT_TEMPLATES.find((entry) => entry.id === id);
  if (!template) throw new Error("Unknown keyboard template");
  const config: KeyFlowConfig = id === "eduuh-dh"
    ? normalizeConfig(structuredClone(bundledConfig))
    : { version: "1.0", name: id === "blank" ? "My Layout" : template.name, remapping: {} };
  if ("rows" in template) {
    config.remapping = Object.fromEntries(template.rows.flatMap((row, index) => {
      const sources = QWERTY_ROWS[index].split(" ");
      const outputs = row.split(" ");
      if (outputs.length !== sources.length) throw new Error("Incomplete keyboard template");
      return sources.flatMap((source, i) => outputs[i] === source ? [] : [[source, outputs[i]]]);
    }));
  }
  return normalizeConfig({ ...config, name: name?.trim() || config.name });
}
