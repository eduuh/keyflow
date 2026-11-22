"use client";

import { useEffect, useState } from "react";
import { Header } from "@/components/Header";
import { Keyboard } from "@/components/Keyboard/Keyboard";
import { LayerTabs } from "@/components/Keyboard/LayerTabs";
import { KeyPicker } from "@/components/KeyPicker/KeyPicker";
import { BaseModifierSetup } from "@/components/BaseModifierSetup";
import { Advanced } from "@/components/Advanced";
import { CommunityLayoutsSection } from "@/components/CommunityLayouts";
import { TourAutoStart } from "@/components/Tour";
import { EnvironmentSettings } from "@/components/EnvironmentSettings";

export default function Home() {
  const [ready, setReady] = useState(false);
  useEffect(() => setReady(true), []);
  if (!ready) {
    return <main className="min-h-screen grid place-items-center" aria-busy="true">
      <p role="status" className="text-sm text-muted-foreground">Loading designer...</p>
    </main>;
  }
  return (
    <div className="min-h-screen flex flex-col" data-designer-ready>
      <Header />
      <main className="flex-1 w-full max-w-[1600px] mx-auto px-4 lg:px-8 py-6 space-y-5">
        <div className="flex flex-wrap items-end justify-between gap-3">
          <div>
            <p className="text-xs font-medium text-primary uppercase tracking-widest mb-1">Layout workspace</p>
            <h2 className="text-2xl font-semibold tracking-tight">A keyboard that works your way.</h2>
          </div>
          <p className="text-sm text-muted-foreground">Select a key. Pick an output. Make it yours.</p>
        </div>
        <EnvironmentSettings />
        <BaseModifierSetup />
        <LayerTabs />
        <div className="grid grid-cols-1 xl:grid-cols-[minmax(0,1fr)_340px] gap-5 items-start">
          <Keyboard />
          <KeyPicker />
        </div>
        <Advanced />
        <CommunityLayoutsSection />
      </main>
      <TourAutoStart />
    </div>
  );
}
