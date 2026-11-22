"use client";

import { useEffect, useState } from "react";
import { Header } from "@/components/Header";
import { EnvironmentSettings } from "@/components/EnvironmentSettings";
import { Advanced } from "@/components/Advanced";

export default function Settings() {
  const [ready, setReady] = useState(false);
  useEffect(() => setReady(true), []);
  if (!ready) {
    return <main className="min-h-screen grid place-items-center" aria-busy="true">
      <p role="status" className="text-sm text-muted-foreground">Loading settings...</p>
    </main>;
  }
  return (
    <div className="min-h-screen flex flex-col" data-designer-ready>
      <Header settings />
      <main className="flex-1 w-full max-w-4xl mx-auto px-4 py-6 space-y-5">
        <h2 className="text-lg font-semibold">Settings</h2>
        <EnvironmentSettings />
        <Advanced />
      </main>
    </div>
  );
}
