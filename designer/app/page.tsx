"use client";

import { Header } from "@/components/Header";
import { Keyboard } from "@/components/Keyboard/Keyboard";
import { LayerTabs } from "@/components/Keyboard/LayerTabs";
import { KeyPicker } from "@/components/KeyPicker/KeyPicker";
import { BaseModifierSetup } from "@/components/BaseModifierSetup";
import { CombosEditor } from "@/components/Editors/CombosEditor";
import { CustomModifiersEditor } from "@/components/Editors/CustomModifiersEditor";

export default function Home() {
  return (
    <div className="h-screen flex flex-col overflow-hidden">
      <Header />

      <main className="flex-1 flex flex-col overflow-auto px-6 py-4 gap-3">
        <BaseModifierSetup />
        <LayerTabs />
        <Keyboard />
        <KeyPicker />

        {/* JSON-only-before-now editors. Collapsed by default so they don't
            crowd the keyboard view; expand when you want to edit the number
            row or define a Space-Cadet-style custom modifier. */}
        <div className="space-y-2">
          <CombosEditor />
          <CustomModifiersEditor />
        </div>
      </main>
    </div>
  );
}
