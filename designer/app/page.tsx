"use client";

import { Header } from "@/components/Header";
import { Keyboard } from "@/components/Keyboard/Keyboard";
import { LayerTabs } from "@/components/Keyboard/LayerTabs";
import { KeyPicker } from "@/components/KeyPicker/KeyPicker";
import { BaseModifierSetup } from "@/components/BaseModifierSetup";
import { Advanced } from "@/components/Advanced";

export default function Home() {
  return (
    <div className="min-h-screen flex flex-col">
      <Header />
      <main className="flex-1 w-full px-4 lg:px-6 py-4 space-y-3">
        <BaseModifierSetup />
        <LayerTabs />
        <Keyboard />
        <KeyPicker />
        <Advanced />
      </main>
    </div>
  );
}
