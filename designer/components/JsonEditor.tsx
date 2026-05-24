"use client";

import { useEffect, useState } from "react";
import { Check, Download, RefreshCw, Sparkles, X } from "lucide-react";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogHeader,
  DialogTitle,
} from "@/components/ui/dialog";
import { Button } from "@/components/ui/button";
import { Textarea } from "@/components/ui/textarea";
import { useConfigStore } from "@/lib/store";

// JSON view + edit modal.
//
// Opens with the current config formatted (2-space indent). User can:
//   - Read it (no action needed)
//   - Edit inline, then Save (validates + applies via setConfig)
//   - Format (re-indent / strip extra whitespace)
//   - Reset (revert to what the store currently has)
//   - Download (save as <name>.json)
//
// Validation rules match the importer: must parse as JSON, must have a
// `version` string. Everything else flows through normalizeImport so legacy
// shapes / unknown fields are handled the same way the file picker handles them.

type JsonEditorProps = {
  isOpen: boolean;
  onClose: () => void;
};

export function JsonEditor({ isOpen, onClose }: JsonEditorProps) {
  const { config, setConfig } = useConfigStore();
  const [text, setText] = useState("");
  const [error, setError] = useState<string | null>(null);
  const [dirty, setDirty] = useState(false);

  // Reset the editor whenever the modal opens, so it always reflects
  // the current store config (not stale text from a previous session).
  useEffect(() => {
    if (isOpen) {
      setText(JSON.stringify(config, null, 2));
      setError(null);
      setDirty(false);
    }
  }, [isOpen, config]);

  const tryParse = (): unknown | null => {
    try {
      const parsed = JSON.parse(text);
      if (typeof parsed !== "object" || parsed === null) {
        setError("Top-level value must be a JSON object");
        return null;
      }
      if (!("version" in parsed) || typeof (parsed as { version?: unknown }).version !== "string") {
        setError("Config must include a 'version' string field");
        return null;
      }
      setError(null);
      return parsed;
    } catch (e) {
      setError(e instanceof Error ? e.message : "Invalid JSON");
      return null;
    }
  };

  const handleSave = () => {
    const parsed = tryParse();
    if (parsed) {
      // setConfig calls normalizeImport which strips strictMode/debug
      // and coerces legacy `trigger` to canonical `triggers[]`.
      setConfig(parsed as Parameters<typeof setConfig>[0]);
      setDirty(false);
      onClose();
    }
  };

  const handleFormat = () => {
    const parsed = tryParse();
    if (parsed) {
      setText(JSON.stringify(parsed, null, 2));
    }
  };

  const handleReset = () => {
    if (dirty && !confirm("Discard changes and reload from current config?")) return;
    setText(JSON.stringify(config, null, 2));
    setError(null);
    setDirty(false);
  };

  const handleDownload = () => {
    // Download what's currently in the editor (which may differ from the
    // store if the user has unsaved edits — match the visible text).
    const blob = new Blob([text], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = `${config.name || "config"}.json`;
    a.click();
    URL.revokeObjectURL(url);
  };

  const handleChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    setText(e.target.value);
    setDirty(true);
    if (error) setError(null);
  };

  return (
    <Dialog open={isOpen} onOpenChange={(o) => !o && onClose()}>
      <DialogContent className="max-w-3xl max-h-[90vh] flex flex-col">
        <DialogHeader>
          <DialogTitle>JSON view & edit</DialogTitle>
          <DialogDescription>
            Direct access to your config as JSON. Edit, format, save back to the designer,
            or download. The same C++ schema applies — see{" "}
            <code className="text-xs">docs/EDUUH_DH.md</code> for the reference shape.
          </DialogDescription>
        </DialogHeader>

        <Textarea
          value={text}
          onChange={handleChange}
          spellCheck={false}
          className="font-mono text-sm flex-1 min-h-[40vh] resize-none"
          aria-invalid={!!error}
        />

        {error && (
          <div className="flex items-start gap-2 px-3 py-2 rounded-md bg-destructive/10 border border-destructive/40">
            <X className="h-4 w-4 text-destructive shrink-0 mt-0.5" />
            <p className="text-sm text-destructive font-mono">{error}</p>
          </div>
        )}

        <div className="flex flex-wrap items-center gap-2 justify-between pt-1">
          <div className="flex gap-2">
            <Button variant="outline" size="sm" onClick={handleFormat}>
              <Sparkles className="h-4 w-4 mr-1.5" />
              Format
            </Button>
            <Button variant="outline" size="sm" onClick={handleReset} disabled={!dirty}>
              <RefreshCw className="h-4 w-4 mr-1.5" />
              Reset
            </Button>
            <Button variant="outline" size="sm" onClick={handleDownload}>
              <Download className="h-4 w-4 mr-1.5" />
              Download
            </Button>
          </div>
          <div className="flex gap-2">
            <Button variant="outline" size="sm" onClick={onClose}>
              Cancel
            </Button>
            <Button size="sm" onClick={handleSave} disabled={!dirty}>
              <Check className="h-4 w-4 mr-1.5" />
              Save
            </Button>
          </div>
        </div>
      </DialogContent>
    </Dialog>
  );
}
