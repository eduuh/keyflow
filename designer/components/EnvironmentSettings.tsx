"use client";

import { Monitor, RotateCcw } from "lucide-react";
import { useConfigStore, useEditingConfig } from "@/lib/store";
import { OVERRIDE_FIELDS, OverrideField, PLATFORMS } from "@/lib/config";
import { EditPlatform } from "@/lib/types";
import { Button } from "./ui/button";
import { Input } from "./ui/input";
import { Label } from "./ui/label";
import { Switch } from "./ui/switch";

const FIELD_LABELS = {
  remapping: "Key remaps", layers: "Layers", noModCombos: "Combos",
  customModifiers: "Custom modifiers", disableCapsLock: "Caps Lock",
};

export function EnvironmentSettings() {
  const { config: document, platform, resetOverride, applyPlatformPreset, setName, setDisableCapsLock } = useConfigStore();
  const config = useEditingConfig();
  const override = platform === "shared" ? undefined : document.platformOverrides?.[platform];
  const resetSection = (field: OverrideField) => {
    if (resetOverride(field)) return;
    if ((field === "layers" || field === "customModifiers") &&
      confirm("Layers and custom modifiers have linked triggers. Restore Shared for both sections? Their platform edits will be removed.")) {
      resetOverride(field, true);
    }
  };
  return (
    <section data-tour="environment-options" aria-label="Configuration environment" className="rounded-xl border bg-card p-4 space-y-4">
      <div className="flex flex-wrap items-center justify-between gap-4">
        <div className="space-y-1.5 w-full sm:w-64">
          <Label htmlFor="config-name" className="text-xs text-muted-foreground">Layout name</Label>
          <Input id="config-name" value={document.name ?? ""} onChange={(event) => setName(event.target.value)} />
        </div>
        <div className="space-y-1.5 min-w-0">
          <p className="text-xs text-muted-foreground flex items-center gap-1.5"><Monitor className="h-3.5 w-3.5" /> Editing environment</p>
          <EnvironmentSelector />
        </div>
        <div className="flex items-center gap-2">
          <Switch id="disable-caps" checked={config.disableCapsLock ?? false} onCheckedChange={setDisableCapsLock} />
          <Label htmlFor="disable-caps" className="text-sm">Disable Caps Lock</Label>
        </div>
      </div>
      <div className="border-t pt-3 flex flex-wrap items-center gap-3">
        <p className="text-xs text-muted-foreground flex-1 min-w-48">
          {platform === "shared"
            ? "Shared edits apply to every platform unless overridden. JSON export keeps all environments."
            : `${PLATFORMS[platform]} preview + editing. Remaps merge with Shared; editing an array replaces the whole section for this platform.`}
        </p>
        {platform !== "shared" && (
          <>
            <Button variant="outline" size="sm" onClick={applyPlatformPreset}>Apply {PLATFORMS[platform]} preset</Button>
            <Button variant="ghost" size="sm" disabled={!override} onClick={() => {
              if (confirm(`Remove all ${PLATFORMS[platform]} overrides and inherit Shared?`)) resetOverride();
            }}><RotateCcw className="h-3.5 w-3.5 mr-1" />Reset environment</Button>
          </>
        )}
      </div>
      {platform !== "shared" && (
        <div className="flex flex-wrap gap-2">
          {OVERRIDE_FIELDS.map((field) => (
            <span key={field} className="inline-flex items-center gap-2 text-xs rounded-md border px-2.5 py-1.5">
              {FIELD_LABELS[field]} <span className="text-muted-foreground">{override?.[field] !== undefined ? "overridden" : "inherited"}</span>
              {override?.[field] !== undefined && <button type="button" onClick={() => resetSection(field)}
                aria-label={`Inherit ${FIELD_LABELS[field]}`} className="text-primary hover:underline">Use Shared</button>}
            </span>
          ))}
          <p className="w-full text-xs text-muted-foreground">Preset: PrintScreen → {platform === "windows" ? "RightCtrl" : "LeftWin (Super / Command)"}. Other mappings stay unchanged.</p>
        </div>
      )}
    </section>
  );
}

export function EnvironmentSelector() {
  const { config, platform, setPlatform } = useConfigStore();
  return (
    <div data-tour="environment" className="flex flex-wrap gap-1" role="group" aria-label="Editing environment">
      {(["shared", ...Object.keys(PLATFORMS)] as EditPlatform[]).map((value) => (
        <Button key={value} size="sm" variant={platform === value ? "secondary" : "ghost"}
          aria-label={value === "shared" ? "Shared" : PLATFORMS[value]}
          aria-pressed={platform === value} onClick={() => setPlatform(value)}>
          {value === "shared" ? "Shared" : PLATFORMS[value]}
          {value !== "shared" && config.platformOverrides?.[value] && <span aria-hidden title="Has overrides" className="h-1.5 w-1.5 rounded-full bg-current" />}
        </Button>
      ))}
    </div>
  );
}
