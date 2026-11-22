"use client";

import { useState } from "react";
import { useConfigStore } from "@/lib/store";
import type { Platform } from "@/lib/types";
import { PLATFORMS } from "@/lib/config";
import { Dialog, DialogContent, DialogDescription, DialogHeader, DialogTitle } from "./ui/dialog";
import { Button } from "./ui/button";

const REPO = "https://github.com/eduuh/keyflow";

export function SetupInstructions({ onClose }: { onClose: () => void }) {
  const platform = useConfigStore((state) => state.platform);
  const [target, setTarget] = useState<Platform>(platform === "shared" ? "windows" : platform);
  return (
    <Dialog open onOpenChange={(open) => !open && onClose()}>
      <DialogContent className="sm:max-w-2xl max-h-[90vh] overflow-y-auto">
        <DialogHeader>
          <DialogTitle>Use your config</DialogTitle>
          <DialogDescription>Save your JSON, validate it, then start Keyflow. One file keeps every platform override.</DialogDescription>
        </DialogHeader>
        <div role="group" aria-label="Setup platform" className="flex gap-2">
          {Object.entries(PLATFORMS).map(([value, label]) => (
            <Button key={value} size="sm" variant={target === value ? "secondary" : "outline"}
              aria-pressed={target === value} onClick={() => setTarget(value as Platform)}>{label}</Button>
          ))}
        </div>
        {target === "mac" ? (
          <p role="status" className="rounded-lg border p-4 text-sm">
            macOS configs can be prepared and exported, but Keyflow has no macOS remapping runtime yet.
            The JSON will not remap keys on macOS.
          </p>
        ) : (
          <div className="space-y-4 text-sm">
            <ol className="list-decimal pl-5 space-y-2">
              <li>Use <strong>Save config</strong> and keep the downloaded JSON. Replace the example path below with its full path.</li>
              {target === "windows" ? (
                <li>Extract the <a className="text-primary underline" href={`${REPO}/releases/latest`} target="_blank" rel="noopener noreferrer">Windows release</a>.
                  Install the <a className="text-primary underline" href="https://github.com/oblitum/Interception/releases" target="_blank" rel="noopener noreferrer">Interception driver</a> once as administrator and reboot.
                  Open PowerShell in the folder containing keyflow.exe.</li>
              ) : (
                <li>Get or <a className="text-primary underline" href={`${REPO}/blob/main/BUILD.md`} target="_blank" rel="noopener noreferrer">build the experimental Linux runtime</a>.
                  Running it requires explicitly authorized access to your selected keyboard and /dev/uinput.
                  Do not grant broad input-device access. Keep an unaffected way to stop it.</li>
              )}
              <li>Validate without capturing any keyboard input:</li>
            </ol>
            <Command text={target === "windows"
              ? '.\\keyflow.exe --validate "C:\\path\\your-layout.json"'
              : './keyflow --validate "/path/to/your-layout.json"'} />
            <p>After validation succeeds, start with the same file{target === "linux" ? " and your verified keyboard device path" : ""}:</p>
            <Command text={target === "windows"
              ? '.\\keyflow.exe "C:\\path\\your-layout.json"'
              : './keyflow --device "/dev/input/by-id/YOUR-KEYBOARD-event-kbd" "/path/to/your-layout.json"'} />
            <p className="text-muted-foreground">
              Stop with physical Ctrl+Escape{target === "linux" ? " or Ctrl+C in the terminal" : ""}.
              Restart Keyflow after changing the JSON. For the standard keyboard templates, use a US QWERTY OS keymap to avoid double-remapping.
            </p>
            {target === "linux" && <p className="text-muted-foreground">Already using a Keyflow service?
              Follow the <a className="text-primary underline" href={`${REPO}/blob/main/docs/LINUX_SERVICE.md`} target="_blank" rel="noopener noreferrer">service configuration and rollback guide</a> instead of reinstalling it.</p>}
          </div>
        )}
      </DialogContent>
    </Dialog>
  );
}

function Command({ text }: { text: string }) {
  const [copied, setCopied] = useState(false);
  const [error, setError] = useState(false);
  return (
    <div className="rounded-lg border bg-muted p-3 space-y-2">
      <pre className="whitespace-pre-wrap break-all text-xs font-mono"><code>{text}</code></pre>
      <Button variant="outline" size="sm" onClick={async () => {
        try {
          await navigator.clipboard.writeText(text);
          setCopied(true);
          setError(false);
        } catch {
          setError(true);
        }
      }}>{copied ? "Copied" : "Copy command"}</Button>
      {error && <p role="status" className="text-xs">Select the command above to copy it.</p>}
    </div>
  );
}
