"use client";

import { useState } from "react";
import { ChevronDown, Settings2 } from "lucide-react";
import { Card, CardContent } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Badge } from "@/components/ui/badge";
import { useConfigStore } from "@/lib/store";
import { CombosEditor } from "./Editors/CombosEditor";
import { CustomModifiersEditor } from "./Editors/CustomModifiersEditor";
import { cn } from "@/lib/utils";

// Collapsible "Advanced" section holding the two power-user editors:
// noModCombos (number-row → symbol without Shift) and customModifiers
// (any key promoted to a layer trigger). Both are off the critical path
// for most users, so they default to collapsed — counts in the header
// make it obvious there's something to see when you do have data.

export function Advanced() {
  const [open, setOpen] = useState(false);
  const { config } = useConfigStore();

  const comboCount = config.noModCombos?.length ?? 0;
  const modCount = config.customModifiers?.length ?? 0;
  const total = comboCount + modCount;

  return (
    <Card className="w-full">
      <Button
        variant="ghost"
        onClick={() => setOpen(!open)}
        className="w-full justify-between h-12 px-4 rounded-none rounded-t-lg hover:bg-accent/30"
        aria-expanded={open}
      >
        <span className="flex items-center gap-3">
          <Settings2 className="h-4 w-4" />
          <span className="font-medium">Advanced settings</span>
          {total > 0 && (
            <Badge variant="secondary" className="text-xs">
              {total}
            </Badge>
          )}
        </span>
        <ChevronDown className={cn("h-4 w-4 transition-transform", open && "rotate-180")} />
      </Button>

      {open && (
        <CardContent className="pt-4 space-y-4 border-t">
          <CombosEditor />
          <CustomModifiersEditor />
        </CardContent>
      )}
    </Card>
  );
}
