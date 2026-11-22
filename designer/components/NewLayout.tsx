"use client";

import { useState } from "react";
import { LAYOUT_TEMPLATES, createLayoutTemplate, type LayoutTemplateId } from "@/lib/layoutTemplates";
import { PLATFORMS, resolveConfig } from "@/lib/config";
import { useConfigStore } from "@/lib/store";
import type { EditPlatform } from "@/lib/types";
import { Dialog, DialogContent, DialogDescription, DialogFooter, DialogHeader, DialogTitle } from "./ui/dialog";
import { Button } from "./ui/button";
import { Input } from "./ui/input";
import { Label } from "./ui/label";
import { Switch } from "./ui/switch";
import { cn } from "@/lib/utils";

export function NewLayout({ onClose }: { onClose: () => void }) {
  const { config, setConfig, setPlatform, setDisableCapsLock } = useConfigStore();
  const [template, setTemplate] = useState<LayoutTemplateId>("blank");
  const [name, setName] = useState("My Layout");
  const [platform, selectPlatform] = useState<EditPlatform>("shared");
  const [disableCapsLock, selectDisableCapsLock] = useState<boolean | undefined>();
  const templateCapsLock = resolveConfig(createLayoutTemplate(template), platform).disableCapsLock ?? false;

  const chooseTemplate = (id: LayoutTemplateId) => {
    const next = createLayoutTemplate(id);
    if (name === createLayoutTemplate(template).name) setName(next.name ?? "My Layout");
    setTemplate(id);
    selectDisableCapsLock(undefined);
  };

  const handleCreate = (event: React.FormEvent) => {
    event.preventDefault();
    if (!name.trim()) return;
    if (JSON.stringify(config) !== JSON.stringify(createLayoutTemplate("blank")) &&
      !confirm("Replace the current layout? Save its config first if you want to keep it.")) return;
    const next = createLayoutTemplate(template, name);
    setConfig(next);
    setPlatform(platform);
    if (disableCapsLock !== undefined && disableCapsLock !== (resolveConfig(next, platform).disableCapsLock ?? false)) {
      setDisableCapsLock(disableCapsLock);
    }
    onClose();
  };

  return (
    <Dialog open onOpenChange={(open) => !open && onClose()}>
      <DialogContent className="sm:max-w-xl max-h-[90vh] overflow-y-auto">
        <DialogHeader>
          <DialogTitle>New layout</DialogTitle>
          <DialogDescription>Choose a starting keyboard, then make it yours.</DialogDescription>
        </DialogHeader>
        <form onSubmit={handleCreate} className="space-y-5">
          <div className="space-y-1.5">
            <Label htmlFor="new-layout-name">Layout name</Label>
            <Input id="new-layout-name" value={name} onChange={(event) => setName(event.target.value)} autoFocus />
          </div>
          <fieldset className="space-y-2">
            <legend className="text-sm font-medium">Start from</legend>
            <div className="grid grid-cols-1 sm:grid-cols-2 gap-2">
              {LAYOUT_TEMPLATES.map((entry) => (
                <label key={entry.id} className={cn("flex items-start gap-2.5 rounded-lg border p-3 cursor-pointer",
                  template === entry.id ? "border-primary bg-accent" : "hover:bg-muted")}>
                  <input type="radio" name="layout-template" value={entry.id} checked={template === entry.id}
                    onChange={() => chooseTemplate(entry.id)} className="mt-1 accent-primary" />
                  <span>
                    <span className="block text-sm font-medium">{entry.name}</span>
                    <span className="block text-xs text-muted-foreground">{entry.description}</span>
                  </span>
                </label>
              ))}
            </div>
          </fieldset>
          <details className="rounded-lg border p-3">
            <summary className="text-sm font-medium cursor-pointer">Optional settings</summary>
            <div className="pt-3 space-y-3">
              <div className="space-y-1.5">
                <Label htmlFor="new-layout-environment">Start editing in</Label>
                <select id="new-layout-environment" value={platform} className="flex h-9 w-full rounded-md border bg-background px-3 text-sm"
                  onChange={(event) => {
                    const next = event.target.value as EditPlatform;
                    selectPlatform(next);
                  }}>
                  <option value="shared">Shared (all platforms)</option>
                  {Object.entries(PLATFORMS).map(([value, label]) => <option key={value} value={value}>{label}</option>)}
                </select>
              </div>
              <div className="flex items-center gap-2">
                <Switch id="new-layout-caps" checked={disableCapsLock ?? templateCapsLock} onCheckedChange={selectDisableCapsLock} />
                <Label htmlFor="new-layout-caps">Disable Caps Lock</Label>
              </div>
              <p className="text-xs text-muted-foreground">Templates keep every environment. These settings choose where your next edits apply.</p>
            </div>
          </details>
          <DialogFooter>
            <Button type="button" variant="outline" onClick={onClose}>Cancel</Button>
            <Button type="submit" disabled={!name.trim()}>Create layout</Button>
          </DialogFooter>
        </form>
      </DialogContent>
    </Dialog>
  );
}
