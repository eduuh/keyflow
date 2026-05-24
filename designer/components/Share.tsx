"use client";

import { useEffect, useState } from "react";
import {
  Check, Copy, ExternalLink, GitPullRequest, Loader2, Share2, Upload,
} from "lucide-react";
import {
  Dialog, DialogContent, DialogDescription, DialogHeader, DialogTitle,
} from "@/components/ui/dialog";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { Textarea } from "@/components/ui/textarea";
import { useConfigStore } from "@/lib/store";
import {
  addRecentUrl, assertRawUrl, getGistToken, getRecentUrls, setGistToken,
} from "@/lib/share-storage";

// Single Share entry point exposed in the header — opens a 3-tab dialog:
//   Upload to Gist  → POST api.github.com/gists, returns a raw URL.
//   Load from URL   → fetch + validate + setConfig (replaces local layout).
//   Publish via PR  → builds a CommunityLayout snippet + opens the GitHub
//                     editor for designer/lib/community-layouts.ts so the
//                     user can paste it and open a PR.

const COMMUNITY_LAYOUTS_EDIT_URL =
  "https://github.com/eduuh/keyflow/edit/main/designer/lib/community-layouts.ts";

export function ShareButton() {
  const [open, setOpen] = useState(false);
  return (
    <>
      <Button
        variant="outline"
        size="sm"
        onClick={() => setOpen(true)}
        title="Share or load layouts"
        data-tour="share"
      >
        <Share2 className="h-4 w-4" />
        <span className="hidden md:inline ml-2">Share</span>
      </Button>
      <ShareDialog isOpen={open} onClose={() => setOpen(false)} />
    </>
  );
}

function ShareDialog({ isOpen, onClose }: { isOpen: boolean; onClose: () => void }) {
  return (
    <Dialog open={isOpen} onOpenChange={(o) => !o && onClose()}>
      <DialogContent className="max-w-xl">
        <DialogHeader>
          <DialogTitle>Share or load a layout</DialogTitle>
          <DialogDescription>
            Upload your current config as a public Gist, load any layout from a
            raw URL, or publish your layout to the community via PR.
          </DialogDescription>
        </DialogHeader>
        <Tabs defaultValue="upload">
          <TabsList className="grid grid-cols-3 w-full">
            <TabsTrigger value="upload">
              <Upload className="h-3.5 w-3.5 mr-1.5" />
              Upload
            </TabsTrigger>
            <TabsTrigger value="load">
              <ExternalLink className="h-3.5 w-3.5 mr-1.5" />
              Load URL
            </TabsTrigger>
            <TabsTrigger value="publish">
              <GitPullRequest className="h-3.5 w-3.5 mr-1.5" />
              Publish PR
            </TabsTrigger>
          </TabsList>
          <TabsContent value="upload" className="mt-4">
            <UploadPanel onClose={onClose} />
          </TabsContent>
          <TabsContent value="load" className="mt-4">
            <LoadPanel onClose={onClose} />
          </TabsContent>
          <TabsContent value="publish" className="mt-4">
            <PublishPanel />
          </TabsContent>
        </Tabs>
      </DialogContent>
    </Dialog>
  );
}

// ─── Upload to Gist ──────────────────────────────────────────────────────────

function UploadPanel({ onClose }: { onClose: () => void }) {
  const { config } = useConfigStore();
  const [token, setToken] = useState("");
  const [description, setDescription] = useState("");
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [result, setResult] = useState<{ rawUrl: string; htmlUrl: string } | null>(null);

  useEffect(() => {
    setToken(getGistToken());
  }, []);

  const upload = async () => {
    setError(null);
    setResult(null);
    const trimmedToken = token.trim();
    if (!trimmedToken) {
      setError("Paste a GitHub personal access token with the 'gist' scope.");
      return;
    }
    setBusy(true);
    try {
      const res = await fetch("https://api.github.com/gists", {
        method: "POST",
        headers: {
          Authorization: `Bearer ${trimmedToken}`,
          Accept: "application/vnd.github+json",
          "X-GitHub-Api-Version": "2022-11-28",
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          description:
            description.trim() ||
            `Keyflow layout: ${config.name ?? "Untitled"}`,
          public: true,
          files: {
            "config.json": { content: JSON.stringify(config, null, 2) },
          },
        }),
      });
      if (!res.ok) {
        const body = await res.text();
        throw new Error(`GitHub API ${res.status}: ${body.slice(0, 200)}`);
      }
      const data = await res.json();
      const rawUrl: string | undefined = data?.files?.["config.json"]?.raw_url;
      const htmlUrl: string | undefined = data?.html_url;
      if (!rawUrl || !htmlUrl) {
        throw new Error("Gist created but raw_url / html_url missing from response.");
      }
      setGistToken(trimmedToken);
      addRecentUrl(rawUrl);
      setResult({ rawUrl, htmlUrl });
    } catch (err) {
      setError(err instanceof Error ? err.message : "Unknown error");
    } finally {
      setBusy(false);
    }
  };

  if (result) {
    return <UploadResult result={result} onClose={onClose} />;
  }

  return (
    <div className="space-y-3">
      <div className="space-y-1.5">
        <Label htmlFor="gist-token">GitHub token (gist scope)</Label>
        <Input
          id="gist-token"
          type="password"
          value={token}
          onChange={(e) => setToken(e.target.value)}
          placeholder="ghp_..."
          className="font-mono text-xs"
        />
        <p className="text-xs text-muted-foreground">
          Create at{" "}
          <a
            href="https://github.com/settings/tokens/new?scopes=gist&description=Keyflow%20Designer"
            target="_blank"
            rel="noopener noreferrer"
            className="underline"
          >
            github.com/settings/tokens
          </a>{" "}
          with only the <code>gist</code> scope. Stored in sessionStorage and
          cleared when you close this tab.
        </p>
      </div>
      <div className="space-y-1.5">
        <Label htmlFor="gist-desc">Description (optional)</Label>
        <Textarea
          id="gist-desc"
          value={description}
          onChange={(e) => setDescription(e.target.value)}
          placeholder="A short description of your layout"
          rows={2}
        />
      </div>
      {error && <p className="text-sm text-destructive break-all">{error}</p>}
      <div className="flex justify-end">
        <Button onClick={upload} disabled={busy}>
          {busy ? (
            <>
              <Loader2 className="h-4 w-4 mr-1.5 animate-spin" />
              Uploading...
            </>
          ) : (
            "Upload to Gist"
          )}
        </Button>
      </div>
    </div>
  );
}

function UploadResult({
  result,
  onClose,
}: {
  result: { rawUrl: string; htmlUrl: string };
  onClose: () => void;
}) {
  const [copied, setCopied] = useState(false);

  const copy = async () => {
    await navigator.clipboard.writeText(result.rawUrl);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  return (
    <div className="space-y-3">
      <p className="text-sm text-muted-foreground">
        Gist created. The raw URL below is what others paste into the "Load
        URL" tab — or what you use in the Publish PR tab to register this
        layout with the community.
      </p>
      <div className="flex gap-2">
        <Input readOnly value={result.rawUrl} className="font-mono text-xs" />
        <Button size="sm" variant="outline" onClick={copy} title="Copy raw URL">
          {copied ? <Check className="h-4 w-4" /> : <Copy className="h-4 w-4" />}
        </Button>
      </div>
      <div className="flex justify-between items-center">
        <Button asChild variant="ghost" size="sm">
          <a href={result.htmlUrl} target="_blank" rel="noopener noreferrer">
            <ExternalLink className="h-4 w-4 mr-1.5" />
            View on Gist
          </a>
        </Button>
        <Button size="sm" onClick={onClose}>
          Done
        </Button>
      </div>
    </div>
  );
}

// ─── Load from URL ───────────────────────────────────────────────────────────

function LoadPanel({ onClose }: { onClose: () => void }) {
  const { setConfig } = useConfigStore();
  const [url, setUrl] = useState("");
  const [recent, setRecent] = useState<string[]>([]);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    setRecent(getRecentUrls());
  }, []);

  const load = async (target: string) => {
    setError(null);
    const trimmed = target.trim();
    if (!trimmed) {
      setError("Paste a URL first.");
      return;
    }
    try {
      assertRawUrl(trimmed);
    } catch (err) {
      setError(err instanceof Error ? err.message : "Invalid URL");
      return;
    }
    if (
      !confirm(
        "Replace your current config with the one from this URL?\n\nYour current layout will be lost.",
      )
    ) {
      return;
    }
    setBusy(true);
    try {
      const res = await fetch(trimmed);
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const config = await res.json();
      if (typeof config !== "object" || !config?.version) {
        throw new Error("Response wasn't a valid Keyflow config (missing 'version').");
      }
      setConfig(config);
      addRecentUrl(trimmed);
      onClose();
    } catch (err) {
      setError(err instanceof Error ? err.message : "Unknown error");
    } finally {
      setBusy(false);
    }
  };

  return (
    <div className="space-y-3">
      <div className="space-y-1.5">
        <Label htmlFor="load-url">Raw URL</Label>
        <Input
          id="load-url"
          value={url}
          onChange={(e) => setUrl(e.target.value)}
          placeholder="https://raw.githubusercontent.com/..."
          className="font-mono text-xs"
        />
        <p className="text-xs text-muted-foreground">
          Accepts <code>raw.githubusercontent.com</code> and{" "}
          <code>gist.githubusercontent.com/.../raw/...</code> URLs.
        </p>
      </div>

      {recent.length > 0 && (
        <div className="space-y-1.5">
          <Label className="text-xs text-muted-foreground">Recent</Label>
          <div className="space-y-1 border rounded-md p-1">
            {recent.map((u) => (
              <button
                key={u}
                type="button"
                onClick={() => setUrl(u)}
                className="block w-full text-left text-xs font-mono truncate px-2 py-1 rounded hover:bg-muted"
                title={u}
              >
                {u}
              </button>
            ))}
          </div>
        </div>
      )}

      {error && <p className="text-sm text-destructive break-all">{error}</p>}

      <div className="flex justify-end">
        <Button onClick={() => load(url)} disabled={busy}>
          {busy ? (
            <>
              <Loader2 className="h-4 w-4 mr-1.5 animate-spin" />
              Loading...
            </>
          ) : (
            "Load"
          )}
        </Button>
      </div>
    </div>
  );
}

// ─── Publish via PR ──────────────────────────────────────────────────────────

function PublishPanel() {
  const { config } = useConfigStore();
  const [name, setName] = useState(config.name ?? "");
  const [description, setDescription] = useState("");
  const [author, setAuthor] = useState("");
  const [authorUrl, setAuthorUrl] = useState("");
  const [badge, setBadge] = useState("");
  const [rawUrl, setRawUrl] = useState("");
  const [snippet, setSnippet] = useState<string | null>(null);
  const [copied, setCopied] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const build = () => {
    setError(null);
    if (!name.trim() || !description.trim() || !author.trim() || !authorUrl.trim() || !rawUrl.trim()) {
      setError("Name, description, author, author URL, and raw URL are required.");
      return;
    }
    try {
      assertRawUrl(rawUrl.trim());
    } catch (err) {
      setError(err instanceof Error ? err.message : "Invalid raw URL");
      return;
    }
    setSnippet(formatCommunityEntry({
      name: name.trim(),
      description: description.trim(),
      author: author.trim(),
      authorUrl: authorUrl.trim(),
      rawUrl: rawUrl.trim(),
      badge: badge.trim() || undefined,
    }));
  };

  const copy = async () => {
    if (!snippet) return;
    await navigator.clipboard.writeText(snippet);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  if (snippet) {
    return (
      <div className="space-y-3">
        <p className="text-sm text-muted-foreground">
          1. Click <strong>Copy</strong> below. 2. Click <strong>Open editor</strong> to
          open <code>community-layouts.ts</code> on GitHub. 3. Paste the snippet
          inside the <code>COMMUNITY_LAYOUTS</code> array, then GitHub will guide
          you through forking and opening a PR.
        </p>
        <Textarea readOnly value={snippet} rows={10} className="font-mono text-xs" />
        <div className="flex gap-2 justify-end">
          <Button variant="outline" size="sm" onClick={() => setSnippet(null)}>
            Edit fields
          </Button>
          <Button variant="outline" size="sm" onClick={copy}>
            {copied ? <Check className="h-4 w-4 mr-1.5" /> : <Copy className="h-4 w-4 mr-1.5" />}
            {copied ? "Copied" : "Copy"}
          </Button>
          <Button asChild size="sm">
            <a href={COMMUNITY_LAYOUTS_EDIT_URL} target="_blank" rel="noopener noreferrer">
              <ExternalLink className="h-4 w-4 mr-1.5" />
              Open editor
            </a>
          </Button>
        </div>
      </div>
    );
  }

  return (
    <div className="space-y-3">
      <p className="text-xs text-muted-foreground">
        Submit your layout to the community gallery. You'll need a hosted{" "}
        <strong>raw URL</strong> first — use the <em>Upload</em> tab to make one,
        or point at a file in your own repo.
      </p>
      <div className="grid grid-cols-2 gap-3">
        <div className="space-y-1.5">
          <Label htmlFor="pub-name">Layout name</Label>
          <Input id="pub-name" value={name} onChange={(e) => setName(e.target.value)} placeholder="My Layout" />
        </div>
        <div className="space-y-1.5">
          <Label htmlFor="pub-badge">Badge (optional)</Label>
          <Input id="pub-badge" value={badge} onChange={(e) => setBadge(e.target.value)} placeholder="Colemak-inspired" />
        </div>
      </div>
      <div className="space-y-1.5">
        <Label htmlFor="pub-desc">Description</Label>
        <Textarea
          id="pub-desc"
          value={description}
          onChange={(e) => setDescription(e.target.value)}
          rows={2}
          placeholder="What makes your layout interesting?"
        />
      </div>
      <div className="grid grid-cols-2 gap-3">
        <div className="space-y-1.5">
          <Label htmlFor="pub-author">Author (GitHub handle)</Label>
          <Input id="pub-author" value={author} onChange={(e) => setAuthor(e.target.value)} placeholder="octocat" />
        </div>
        <div className="space-y-1.5">
          <Label htmlFor="pub-author-url">Author URL</Label>
          <Input
            id="pub-author-url"
            value={authorUrl}
            onChange={(e) => setAuthorUrl(e.target.value)}
            placeholder="https://github.com/octocat"
            className="font-mono text-xs"
          />
        </div>
      </div>
      <div className="space-y-1.5">
        <Label htmlFor="pub-raw">Raw URL (the layout's config.json)</Label>
        <Input
          id="pub-raw"
          value={rawUrl}
          onChange={(e) => setRawUrl(e.target.value)}
          placeholder="https://raw.githubusercontent.com/..."
          className="font-mono text-xs"
        />
      </div>
      {error && <p className="text-sm text-destructive">{error}</p>}
      <div className="flex justify-end">
        <Button onClick={build}>
          <GitPullRequest className="h-4 w-4 mr-1.5" />
          Build snippet
        </Button>
      </div>
    </div>
  );
}

// Produces the literal TS source the user pastes inside COMMUNITY_LAYOUTS[].
// Matches the indentation and field order of the existing eduuh-dh entry in
// designer/lib/community-layouts.ts so the PR diff stays clean.
function formatCommunityEntry(entry: {
  name: string;
  description: string;
  author: string;
  authorUrl: string;
  rawUrl: string;
  badge?: string;
}): string {
  const sourceUrl = deriveSourceUrlFromRaw(entry.rawUrl);
  const id = slugify(entry.name);
  const escape = (s: string) => s.replace(/\\/g, "\\\\").replace(/"/g, '\\"');
  const lines = [
    "  {",
    `    id: "${escape(id)}",`,
    `    name: "${escape(entry.name)}",`,
    `    description:`,
    `      "${escape(entry.description)}",`,
    `    author: "${escape(entry.author)}",`,
    `    authorUrl: "${escape(entry.authorUrl)}",`,
    `    rawUrl: "${escape(entry.rawUrl)}",`,
    `    sourceUrl: "${escape(sourceUrl)}",`,
  ];
  if (entry.badge) lines.push(`    badge: "${escape(entry.badge)}",`);
  lines.push("  },");
  return lines.join("\n");
}

// Best-effort: convert a raw URL to a human-friendly source link.
//   raw.githubusercontent.com/u/r/ref/path  → github.com/u/r/blob/ref/path
//   gist.githubusercontent.com/u/id/raw/... → gist.github.com/u/id
//   anything else                           → return as-is
function deriveSourceUrlFromRaw(rawUrl: string): string {
  try {
    const u = new URL(rawUrl);
    if (u.hostname === "raw.githubusercontent.com") {
      const parts = u.pathname.split("/").filter(Boolean);
      if (parts.length >= 4) {
        const [user, repo, ref, ...rest] = parts;
        return `https://github.com/${user}/${repo}/blob/${ref}/${rest.join("/")}`;
      }
    }
    if (u.hostname === "gist.githubusercontent.com") {
      const parts = u.pathname.split("/").filter(Boolean);
      if (parts.length >= 2) {
        const [user, id] = parts;
        return `https://gist.github.com/${user}/${id}`;
      }
    }
  } catch {
    // fall through
  }
  return rawUrl;
}

function slugify(s: string): string {
  return s
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, "-")
    .replace(/^-+|-+$/g, "")
    || "layout";
}

