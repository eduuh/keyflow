"use client";

import { useRef, useState } from "react";
import Link from "next/link";
import {
  Moon, Sun, Download, Upload, FileJson, Github,
  HardDriveDownload, Compass, ChevronDown, Plus, BookOpen,
} from "lucide-react";
import { useTheme } from "./ThemeProvider";
import { useConfigStore } from "@/lib/store";
import { CategoryExport } from "./Export/CategoryExport";
import { CommunityLayoutsButton } from "./CommunityLayouts";
import { JsonEditor } from "./JsonEditor";
import { ShareButton } from "./Share";
import { Button, buttonVariants } from "@/components/ui/button";
import { serializeConfig } from "@/lib/config";
import { startTour } from "./Tour";
import { NewLayout } from "./NewLayout";
import { SetupInstructions } from "./SetupInstructions";

const RELEASE_DOWNLOAD_URL =
  "https://github.com/eduuh/keyflow/releases/latest";
const REPO_URL = "https://github.com/eduuh/keyflow";

export function Header({ settings = false }: { settings?: boolean }) {
  const { theme, toggleTheme } = useTheme();
  const { config, setConfig } = useConfigStore();
  const [showExportModal, setShowExportModal] = useState(false);
  const [showJsonEditor, setShowJsonEditor] = useState(false);
  const [showNewLayout, setShowNewLayout] = useState(false);
  const [showSetup, setShowSetup] = useState(false);
  const [downloaded, setDownloaded] = useState(false);
  const tools = useRef<HTMLDetailsElement>(null);

  const closeTools = () => {
    if (!tools.current) return;
    tools.current.open = false;
    tools.current.querySelector("summary")?.focus();
  };

  const handleSave = () => {
    try {
      const url = URL.createObjectURL(new Blob([serializeConfig(config)], { type: "application/json" }));
      const anchor = document.createElement("a");
      anchor.href = url;
      anchor.download = `${config.name || "config"}.json`;
      anchor.click();
      URL.revokeObjectURL(url);
      setDownloaded(true);
    } catch (error) {
      alert("Cannot save config: " + (error instanceof Error ? error.message : "Invalid configuration"));
    }
  };

  const handleImport = () => {
    const input = document.createElement("input");
    input.type = "file";
    input.accept = ".json";
    input.onchange = (e) => {
      const file = (e.target as HTMLInputElement).files?.[0];
      if (!file) return;
      const reader = new FileReader();
      reader.onload = (ev) => {
        try {
          const json = JSON.parse(ev.target?.result as string);
          if (!json.version) {
            alert("Invalid config: missing 'version' field");
            return;
          }
          setConfig(json);
        } catch (err) {
          alert("Invalid JSON: " + (err instanceof Error ? err.message : "unknown"));
        }
      };
      reader.readAsText(file);
    };
    input.click();
  };

  return (
    <header
      data-tour="header"
      className="border-b sticky top-0 z-30 bg-background/95 backdrop-blur supports-[backdrop-filter]:bg-background/80"
    >
      <div className="px-4 lg:px-8 py-3 flex flex-wrap items-center justify-between gap-3 max-w-[1600px] mx-auto">
        <div className="flex items-center gap-3 min-w-0">
          <div className="leading-tight min-w-0">
            <h1 aria-label="Keyflow Designer" className="text-sm font-semibold truncate">Keyflow<span className="hidden sm:inline"> Designer</span></h1>
            <p data-layout-name className="text-xs text-muted-foreground truncate max-w-40">
              {config.name || "Untitled"}
            </p>
          </div>
        </div>

        <div className="flex items-center gap-1.5 flex-wrap justify-end">
          <Button variant="outline" size="sm" onClick={() => setShowNewLayout(true)}>
            <Plus className="h-4 w-4" />New layout
          </Button>
          <Button asChild variant="ghost" size="sm">
            <Link href={settings ? "/" : "/settings"} data-tour="settings-link">
              {settings ? "Remapping" : "Settings"}
            </Link>
          </Button>
          <Button variant="outline" size="sm" onClick={handleImport} aria-label="Import">
            <Upload className="h-4 w-4" />
            <span>Import</span>
          </Button>
          <Button size="sm" onClick={handleSave} data-tour="save-config">
            <Download className="h-4 w-4" />
            Save config
          </Button>
          <details ref={tools} className="relative" onKeyDown={(event) => {
            if (event.key === "Escape" && tools.current?.open) {
              event.preventDefault();
              closeTools();
            }
          }} onBlur={(event) => {
            if (tools.current && !event.currentTarget.contains(event.relatedTarget)) tools.current.open = false;
          }}>
            <summary data-tour="more-tools" aria-label="More tools"
              className={buttonVariants({ variant: "ghost", size: "sm", className: "cursor-pointer list-none [&::-webkit-details-marker]:hidden" })}>
              More <ChevronDown className="h-4 w-4" />
            </summary>
            <div className="absolute right-0 mt-2 w-56 max-w-[calc(100vw-2rem)] rounded-lg border bg-popover p-2 shadow-lg flex flex-col gap-1 [&_button]:w-full [&_button]:justify-start [&_a]:w-full [&_a]:justify-start"
              onClickCapture={(event) => {
                if (tools.current?.open && event.target instanceof Element && event.target.closest("button, a")) closeTools();
              }}>
              <Button variant="ghost" size="sm" onClick={() => setShowJsonEditor(true)} aria-label="JSON">
                <FileJson className="h-4 w-4" />JSON
              </Button>
              <Button variant="ghost" size="sm" onClick={() => setShowExportModal(true)} aria-label="Export image">
                <Download className="h-4 w-4" />Export image
              </Button>
              <ShareButton />
              <CommunityLayoutsButton />
              <Button variant="ghost" size="sm" onClick={() => setShowSetup(true)}>
                <BookOpen className="h-4 w-4" />Setup instructions
              </Button>
              <Button asChild variant="ghost" size="sm">
                <a href={RELEASE_DOWNLOAD_URL} target="_blank" rel="noopener noreferrer">
                  <HardDriveDownload className="h-4 w-4" />Get Keyflow
                </a>
              </Button>
              <Button variant="ghost" size="sm" onClick={() => startTour(settings)} aria-label="Take a tour">
                <Compass className="h-4 w-4" />Take a tour
              </Button>
              <Button asChild variant="ghost" size="sm" aria-label="View on GitHub">
                <a href={REPO_URL} target="_blank" rel="noopener noreferrer">
                  <Github className="h-4 w-4" />View on GitHub
                </a>
              </Button>
            </div>
          </details>
          <Button variant="ghost" size="icon" className="h-9 w-9" onClick={toggleTheme} aria-label="Toggle theme">
            {theme === "dark" ? <Sun className="h-4 w-4" /> : <Moon className="h-4 w-4" />}
          </Button>
        </div>
      </div>

      {downloaded && <p className="px-4 lg:px-8 pb-2 max-w-[1600px] mx-auto text-xs text-muted-foreground" role="status">
        Config downloaded. <button type="button" className="text-primary underline" onClick={() => setShowSetup(true)}>How to use your JSON</button>
      </p>}
      <CategoryExport isOpen={showExportModal} onClose={() => setShowExportModal(false)} />
      <JsonEditor isOpen={showJsonEditor} onClose={() => setShowJsonEditor(false)} />
      {showNewLayout && <NewLayout onClose={() => setShowNewLayout(false)} />}
      {showSetup && <SetupInstructions onClose={() => setShowSetup(false)} />}
    </header>
  );
}
