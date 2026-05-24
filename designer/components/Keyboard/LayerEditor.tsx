"use client";

import { useState } from "react";
import { Plus, Trash2, X } from "lucide-react";
import { useConfigStore } from "@/lib/store";
import {
  Dialog,
  DialogContent,
  DialogFooter,
  DialogHeader,
  DialogTitle,
} from "@/components/ui/dialog";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { Badge } from "@/components/ui/badge";

type LayerEditorProps = {
  layerIndex: number;
  onClose: () => void;
};

const MODIFIER_KEYS = [
  "RALT",
  "LALT",
  "RCTRL",
  "LCTRL",
  "RSHIFT",
  "LSHIFT",
  "RWIN",
  "LWIN",
  "CAPS",
];

export function LayerEditor({ layerIndex, onClose }: LayerEditorProps) {
  const { config, updateLayer, deleteLayer } = useConfigStore();
  const layer = config.layers?.[layerIndex];

  const [name, setName] = useState(layer?.name || "");
  const [triggers, setTriggers] = useState<string[]>(layer?.triggers ?? []);

  if (!layer) return null;

  const baseRemapping = config.remapping || {};
  const availableModifiers = Array.from(
    new Set(Object.values(baseRemapping).filter((t) => MODIFIER_KEYS.includes(t))),
  );

  const layers = config.layers || [];
  const usedByOthers = new Set<string>();
  layers.forEach((l, i) => {
    if (i === layerIndex) return;
    (l.triggers || []).forEach((t) => usedByOthers.add(t));
  });

  const addable = availableModifiers.filter(
    (m) => !triggers.includes(m) && !usedByOthers.has(m),
  );

  const handleSave = () => {
    if (triggers.length === 0) {
      alert("Layer needs at least one trigger modifier.");
      return;
    }
    updateLayer(layerIndex, { name, triggers });
    onClose();
  };

  const handleDelete = () => {
    if (confirm(`Delete "${layer.name}"? All mappings will be lost.`)) {
      deleteLayer(layerIndex);
      onClose();
    }
  };

  return (
    <Dialog open onOpenChange={(open) => !open && onClose()}>
      <DialogContent className="sm:max-w-md">
        <DialogHeader>
          <DialogTitle>Edit Layer</DialogTitle>
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
                  {t}
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
                    onClick={() => setTriggers([...triggers, mod])}
                  >
                    <Plus className="h-3 w-3 mr-1" />
                    {mod}
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
        </div>

        <DialogFooter className="flex-row justify-between sm:justify-between">
          <Button variant="destructive" size="sm" onClick={handleDelete}>
            <Trash2 className="h-4 w-4 mr-1" />
            Delete
          </Button>
          <div className="flex gap-2">
            <Button variant="outline" size="sm" onClick={onClose}>
              Cancel
            </Button>
            <Button size="sm" onClick={handleSave}>
              Save
            </Button>
          </div>
        </DialogFooter>
      </DialogContent>
    </Dialog>
  );
}
