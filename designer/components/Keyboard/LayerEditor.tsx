"use client";

import { useState } from "react";
import { Plus, Trash2, X } from "lucide-react";
import { useConfigStore, useEditingConfig } from "@/lib/store";
import { availableTriggers } from "@/lib/config";
import { TRIGGER_ALIAS } from "@/lib/keyDisplay";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
} from "@/components/ui/dialog";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { Badge } from "@/components/ui/badge";

const MODIFIER_LABELS: Record<string, string> = {
  LALT: "Left Alt", RALT: "Right Alt", LCTRL: "Left Ctrl", RCTRL: "Right Ctrl",
  LSHIFT: "Left Shift", RSHIFT: "Right Shift", LWIN: "Left Win / Super",
  RWIN: "Right Win / Super", PRINT: "Print Screen",
};

function modifierLabel(name: string): string {
  return Object.hasOwn(MODIFIER_LABELS, name) ? MODIFIER_LABELS[name] : name;
}

type LayerEditorProps = {
  layerIndex?: number;
  onClose: () => void;
};

export function LayerEditor({ layerIndex, onClose }: LayerEditorProps) {
  const { updateLayer, deleteLayer, addLayer, setCurrentLayer } = useConfigStore();
  const config = useEditingConfig();
  const layer = layerIndex !== undefined ? config.layers?.[layerIndex] : undefined;
  const creating = layerIndex === undefined;

  const [name, setName] = useState(layer?.name || `Layer ${(config.layers?.length ?? 0) + 1}`);
  const [triggers, setTriggers] = useState<string[]>(layer?.triggers ?? ["RALT"]);

  if (!creating && !layer) return null;

  const availableModifiers = Array.from(new Set([...Object.keys(TRIGGER_ALIAS).filter((name) => !name.startsWith("MOD")),
    ...availableTriggers(config)]));

  const addable = availableModifiers.filter(
    (m) => !triggers.includes(m),
  );

  const handleSave = () => {
    if (triggers.length === 0 || !name.trim()) {
      alert("Layer needs a name and at least one trigger modifier.");
      return;
    }
    if (layerIndex === undefined) {
      addLayer({ name: name.trim(), triggers, mappings: {} });
      setCurrentLayer(config.layers?.length ?? 0);
    } else {
      updateLayer(layerIndex, { name, triggers });
    }
    onClose();
  };

  const handleDelete = () => {
    if (layerIndex === undefined || !layer) return;
    if (confirm(`Delete "${layer.name}"? All mappings will be lost.`)) {
      deleteLayer(layerIndex);
      onClose();
    }
  };

  return (
    <Dialog open onOpenChange={(open) => !open && onClose()}>
      <DialogContent className="sm:max-w-md">
        <DialogHeader>
          <DialogTitle>{creating ? "New layer" : "Edit Layer"}</DialogTitle>
          <DialogDescription>Name the layer and choose the modifier you hold to activate it. Physical modifiers work without any setup in BASE.</DialogDescription>
        </DialogHeader>

        <div className="space-y-4">
          <div className="space-y-1.5">
            <Label htmlFor="layer-name">Name</Label>
            <Input
              id="layer-name"
              value={name}
              onChange={(e) => setName(e.target.value)}
              placeholder="e.g., Numpad, Arrows & Symbols"
            />
          </div>

          <div className="space-y-1.5">
            <Label>Triggers (any one activates)</Label>
            <div className="min-h-[2.5rem] p-2 rounded-md border bg-background flex flex-wrap gap-1.5">
              {triggers.length === 0 && (
                <span className="text-xs text-muted-foreground italic px-1 py-1">
                  No triggers — add at least one below
                </span>
              )}
              {triggers.map((t) => (
                <Badge key={t} variant="secondary" className="gap-1 pl-2 pr-1">
                  {modifierLabel(t)}
                  <button
                    type="button"
                    onClick={() => setTriggers(triggers.filter((x) => x !== t))}
                    className="hover:text-foreground rounded-sm"
                    aria-label={`Remove ${t}`}
                  >
                    <X className="h-3 w-3" />
                  </button>
                </Badge>
              ))}
            </div>

            {addable.length > 0 && (
              <div className="flex flex-wrap gap-1.5">
                {addable.map((mod) => (
                  <Button
                    key={mod}
                    type="button"
                    variant="outline"
                    size="sm"
                    className="h-7 border-dashed"
                    aria-label={`Add ${modifierLabel(mod)} (${mod})`}
                    onClick={() => setTriggers([...triggers, mod])}
                  >
                    <Plus className="h-3 w-3 mr-1" />
                    {modifierLabel(mod)}
                  </Button>
                ))}
              </div>
            )}

            {availableModifiers.length === 0 && (
              <p className="text-xs text-destructive">
                Map a modifier in BASE first — layers need a trigger.
              </p>
            )}
            {availableModifiers.length > 0 && addable.length === 0 && triggers.length === 0 && (
              <p className="text-xs text-destructive">
                All BASE modifiers are already claimed by other layers.
              </p>
            )}
          </div>

          {layer && (
            <div className="rounded-md border p-3 text-xs space-y-1">
              <div className="flex justify-between">
                <span className="text-muted-foreground">Mapped keys</span>
                <span className="font-medium">{Object.keys(layer.mappings || {}).length}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-muted-foreground">Shift mappings</span>
                <span className="font-medium">{(layer.shiftMappings || []).length}</span>
              </div>
            </div>
          )}
        </div>

        <DialogFooter className="flex-row justify-between sm:justify-between">
          {!creating && <Button variant="destructive" size="sm" onClick={handleDelete}>
            <Trash2 className="h-4 w-4 mr-1" />
            Delete
          </Button>}
          <div className="flex gap-2">
            <Button variant="outline" size="sm" onClick={onClose}>
              Cancel
            </Button>
            <Button size="sm" onClick={handleSave} disabled={!name.trim() || triggers.length === 0}>
              {creating ? "Create layer" : "Save"}
            </Button>
          </div>
        </DialogFooter>
      </DialogContent>
    </Dialog>
  );
}
