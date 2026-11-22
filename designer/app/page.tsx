"use client";

import { useEffect, useState } from "react";
import { Header } from "@/components/Header";
import { Keyboard } from "@/components/Keyboard/Keyboard";
import { LayerTabs } from "@/components/Keyboard/LayerTabs";
import { KeyPicker } from "@/components/KeyPicker/KeyPicker";
import { EnvironmentSelector } from "@/components/EnvironmentSettings";

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
        <div className="flex flex-wrap items-center justify-between gap-3">
          <h2 className="text-lg font-semibold">Remapping</h2>
          <EnvironmentSelector />
        </div>
        <LayerTabs />
        <div className="grid grid-cols-1 xl:grid-cols-[minmax(0,1fr)_340px] gap-5 items-start">
          <Keyboard />
          <KeyPicker />
        </div>
      </main>
    </div>
  );
}
