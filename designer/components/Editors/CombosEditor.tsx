"use client";

import { Plus, Trash2 } from "lucide-react";
import { useConfigStore, useEditingConfig } from "@/lib/store";
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Switch } from "@/components/ui/switch";
import { Label } from "@/components/ui/label";
import {
  Select,
  SelectContent,
  SelectGroup,
  SelectItem,
  SelectLabel,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { ALL_KEY_CODES, KEY_GROUPS, canonicalKey } from "@/lib/allKeys";

// Editor for `noModCombos` — number row → symbols without holding Shift.
// Each combo: { key, output, shift?, description? }
// The C++ side requires zero modifiers held for these to fire.

export function CombosEditor() {
  const { setNoModCombo, removeNoModCombo } = useConfigStore();
  const config = useEditingConfig();
  const combos = config.noModCombos ?? [];

  const handleAdd = () => {
    const used = new Set(combos.map((c) => canonicalKey(c.key)));
    const candidates = ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"];
    const next = [...candidates, ...ALL_KEY_CODES].find((key) => key !== "NOP" && !used.has(key));
    if (!next) return;
    setNoModCombo({ key: next, output: next, shift: false, description: "" });
  };

  return (
    <Card>
      <CardHeader>
        <CardTitle className="text-base">No-modifier combos</CardTitle>
        <CardDescription>
          Remap the number row (or any key) to a symbol without holding Shift. Fires only when
          no modifier is held.
        </CardDescription>
      </CardHeader>
      <CardContent className="space-y-2">
        {combos.map((combo, i) => (
          <div
            key={i}
            className="grid grid-cols-2 sm:grid-cols-[1fr_auto_auto_1fr_2fr_auto] items-center gap-2 rounded-lg border p-3"
          >
            <KeyDropdown
              value={combo.key}
              label={`Combo ${i + 1} source`}
              disabledKeys={combos.filter((_, index) => index !== i).map((entry) => canonicalKey(entry.key) ?? entry.key)}
              onChange={(v) =>
                setNoModCombo({ ...combo, key: v, shift: combo.shift ?? false }, combo.key)
              }
            />
            <span className="text-muted-foreground text-sm">→</span>
            <div className="flex items-center gap-1.5">
              <Switch
                id={`shift-${i}`}
                checked={combo.shift ?? false}
                aria-label={`Combo ${i + 1} shift`}
                onCheckedChange={(checked) =>
                  setNoModCombo({ ...combo, shift: checked })
                }
              />
              <Label htmlFor={`shift-${i}`} className="text-xs cursor-pointer">
                ⇧
              </Label>
            </div>
            <KeyDropdown
              value={combo.output}
              label={`Combo ${i + 1} output`}
              onChange={(v) =>
                setNoModCombo({ ...combo, output: v, shift: combo.shift ?? false })
              }
            />
            <Input
              value={combo.description ?? ""}
              onChange={(e) =>
                setNoModCombo({
                  ...combo,
                  description: e.target.value,
                  shift: combo.shift ?? false,
                })
              }
              placeholder="Description (optional)"
              aria-label={`Combo ${i + 1} description`}
              className="text-sm"
            />
            <Button
              variant="ghost"
              size="icon"
              className="text-muted-foreground hover:text-destructive"
              onClick={() => removeNoModCombo(combo.key)}
              aria-label="Remove combo"
            >
              <Trash2 className="h-4 w-4" />
            </Button>
          </div>
        ))}

        <Button variant="outline" size="sm" onClick={handleAdd}>
          <Plus className="h-4 w-4 mr-1" />
          Add combo
        </Button>
      </CardContent>
    </Card>
  );
}

function KeyDropdown({ value, onChange, label, disabledKeys = [] }: {
  value: string; onChange: (v: string) => void; label: string; disabledKeys?: string[];
}) {
  return (
    <Select value={value} onValueChange={onChange}>
      <SelectTrigger className="h-9 font-mono text-sm" aria-label={label}>
        <SelectValue />
      </SelectTrigger>
      <SelectContent>
        {KEY_GROUPS.map((group) => (
          <SelectGroup key={group.group}>
            <SelectLabel>{group.group}</SelectLabel>
            {group.keys.map((k) => (
              <SelectItem key={k.code} value={k.code} className="font-mono" disabled={disabledKeys.includes(k.code)}>
                {k.label}
              </SelectItem>
            ))}
          </SelectGroup>
        ))}
      </SelectContent>
    </Select>
  );
}
