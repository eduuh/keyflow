"use client";

import { Plus, Trash2 } from "lucide-react";
import { useConfigStore } from "@/lib/store";
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
import { KEY_GROUPS } from "@/lib/allKeys";

// Editor for `customModifiers` — promote any physical key to a layer trigger
// by giving it a name (e.g. Space → SPACE_MOD). C++ assigns each one a bit
// from Custom1..Custom23 in declaration order.

export function CustomModifiersEditor() {
  const { config, setCustomModifier, removeCustomModifier } = useConfigStore();
  const mods = config.customModifiers ?? [];

  const handleAdd = () => {
    const used = new Set(mods.map((m) => m.modifierName));
    const candidates = ["SPACE_MOD", "TAB_MOD", "CAPS_MOD", "ENTER_MOD"];
    const name = candidates.find((c) => !used.has(c)) ?? `MOD_${mods.length + 1}`;
    setCustomModifier({ key: "Space", modifierName: name, blockOutput: true });
  };

  return (
    <Card>
      <CardHeader>
        <CardTitle className="text-base">Custom modifiers</CardTitle>
        <CardDescription>
          Promote any key (Space, Tab, etc.) to a layer trigger. Used by Space-Cadet style
          layouts.
        </CardDescription>
      </CardHeader>
      <CardContent className="space-y-2">
        {mods.map((mod, i) => (
          <div
            key={`${mod.modifierName}-${i}`}
            className="grid grid-cols-[1fr_auto_2fr_auto_auto] items-center gap-2"
          >
            <Select
              value={mod.key}
              onValueChange={(v) =>
                setCustomModifier({ ...mod, key: v, blockOutput: mod.blockOutput ?? true })
              }
            >
              <SelectTrigger className="h-9 font-mono text-sm">
                <SelectValue />
              </SelectTrigger>
              <SelectContent>
                {KEY_GROUPS.map((group) => (
                  <SelectGroup key={group.group}>
                    <SelectLabel>{group.group}</SelectLabel>
                    {group.keys.map((k) => (
                      <SelectItem key={k.code} value={k.code} className="font-mono">
                        {k.label}
                      </SelectItem>
                    ))}
                  </SelectGroup>
                ))}
              </SelectContent>
            </Select>
            <span className="text-muted-foreground text-sm">→</span>
            <Input
              value={mod.modifierName}
              onChange={(e) =>
                setCustomModifier({
                  ...mod,
                  modifierName: e.target.value
                    .toUpperCase()
                    .replace(/[^A-Z0-9_]/g, "_"),
                  blockOutput: mod.blockOutput ?? true,
                })
              }
              placeholder="MOD_NAME"
              className="font-mono text-sm"
            />
            <div className="flex items-center gap-1.5">
              <Switch
                id={`block-${i}`}
                checked={mod.blockOutput ?? true}
                onCheckedChange={(checked) =>
                  setCustomModifier({ ...mod, blockOutput: checked })
                }
              />
              <Label htmlFor={`block-${i}`} className="text-xs cursor-pointer whitespace-nowrap">
                Block output
              </Label>
            </div>
            <Button
              variant="ghost"
              size="icon"
              className="text-muted-foreground hover:text-destructive"
              onClick={() => removeCustomModifier(mod.modifierName)}
              aria-label="Remove custom modifier"
            >
              <Trash2 className="h-4 w-4" />
            </Button>
          </div>
        ))}

        <Button variant="outline" size="sm" onClick={handleAdd}>
          <Plus className="h-4 w-4 mr-1" />
          Add custom modifier
        </Button>
      </CardContent>
    </Card>
  );
}
