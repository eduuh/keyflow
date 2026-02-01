"use client";

import { Header } from "@/components/Header";
import { Keyboard } from "@/components/Keyboard/Keyboard";
import { LayerTabs } from "@/components/Keyboard/LayerTabs";
import { KeyPicker } from "@/components/KeyPicker/KeyPicker";
import { ViewModeToggle } from "@/components/ViewModeToggle";
import { BaseModifierSetup } from "@/components/BaseModifierSetup";

export default function Home() {
  return (
    <div className="h-screen flex flex-col overflow-hidden">
      <Header />

      <main className="flex-1 flex flex-col overflow-auto px-6 py-4 gap-3">
        {/* Quick Setup Banner */}
        <BaseModifierSetup />

        <div className="flex items-center justify-between">
          {/* Layer Tabs */}
          <LayerTabs />

          {/* Strict Mode Toggle */}
          <ViewModeToggle />
        </div>

        {/* Keyboard Display - Full Width */}
        <Keyboard />

        {/* Key Picker */}
        <KeyPicker />
      </main>
    </div>
  );
}
