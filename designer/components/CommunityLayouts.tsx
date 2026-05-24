"use client";

import { useState } from "react";
import { ExternalLink, Github, Loader2, Users } from "lucide-react";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogHeader,
  DialogTitle,
} from "@/components/ui/dialog";
import { Button } from "@/components/ui/button";
import { Badge } from "@/components/ui/badge";
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from "@/components/ui/card";
import { useConfigStore } from "@/lib/store";
import { COMMUNITY_LAYOUTS, CommunityLayout } from "@/lib/community-layouts";

// Three surfaces share a single load action and a single LayoutCard
// component: the header button + dialog (CommunityLayoutsButton), the
// inline section on the page (CommunityLayoutsSection), and any future
// place that wants to list layouts.

function useLoadLayout(): {
  loadingId: string | null;
  load: (layout: CommunityLayout) => Promise<void>;
} {
  const { setConfig } = useConfigStore();
  const [loadingId, setLoadingId] = useState<string | null>(null);

  const load = async (layout: CommunityLayout) => {
    if (
      !confirm(
        `Replace your current config with "${layout.name}"?\n\nYour current layout will be lost.`,
      )
    )
      return;

    setLoadingId(layout.id);
    try {
      const res = await fetch(layout.rawUrl);
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const config = await res.json();
      if (typeof config !== "object" || !config?.version) {
        throw new Error("Response wasn't a valid Keyflow config");
      }
      setConfig(config);
    } catch (err) {
      alert(
        `Failed to load "${layout.name}":\n${
          err instanceof Error ? err.message : "Unknown error"
        }`,
      );
    } finally {
      setLoadingId(null);
    }
  };

  return { loadingId, load };
}

type LayoutCardProps = {
  layout: CommunityLayout;
  loading: boolean;
  onLoad: () => void;
};

function LayoutCard({ layout, loading, onLoad }: LayoutCardProps) {
  return (
    <Card className="flex flex-col">
      <CardContent className="p-4 space-y-3 flex-1">
        <div className="flex items-start justify-between gap-3">
          <div className="min-w-0 flex-1">
            <div className="flex items-center gap-2 flex-wrap">
              <h3 className="font-semibold text-lg">{layout.name}</h3>
              {layout.badge && (
                <Badge variant="secondary" className="text-[10px] uppercase tracking-wider">
                  {layout.badge}
                </Badge>
              )}
            </div>
            <a
              href={layout.authorUrl}
              target="_blank"
              rel="noopener noreferrer"
              className="text-xs text-muted-foreground hover:text-foreground inline-flex items-center gap-1 mt-0.5"
            >
              by {layout.author}
              <ExternalLink className="h-3 w-3" />
            </a>
          </div>
          <Button
            size="sm"
            onClick={onLoad}
            disabled={loading}
            className="shrink-0"
          >
            {loading ? (
              <>
                <Loader2 className="h-4 w-4 mr-1.5 animate-spin" />
                Loading...
              </>
            ) : (
              "Load"
            )}
          </Button>
        </div>

        <p className="text-sm text-muted-foreground leading-relaxed">
          {layout.description}
        </p>

        <div className="flex items-center gap-2 pt-1">
          <Button asChild variant="outline" size="sm">
            <a href={layout.sourceUrl} target="_blank" rel="noopener noreferrer">
              <Github className="h-3.5 w-3.5 mr-1.5" />
              View on GitHub
            </a>
          </Button>
        </div>
      </CardContent>
    </Card>
  );
}

// ─── Modal (header button) ───────────────────────────────────────────────────

type CommunityLayoutsModalProps = {
  isOpen: boolean;
  onClose: () => void;
};

function CommunityLayoutsModal({ isOpen, onClose }: CommunityLayoutsModalProps) {
  const { loadingId, load } = useLoadLayout();

  return (
    <Dialog open={isOpen} onOpenChange={(o) => !o && onClose()}>
      <DialogContent className="max-w-2xl">
        <DialogHeader>
          <DialogTitle>Community layouts</DialogTitle>
          <DialogDescription>
            Browse layouts shared by other users. Click "Load" to import one
            into the designer. To submit yours, open a PR adding an entry to{" "}
            <code className="text-xs">designer/lib/community-layouts.ts</code>.
          </DialogDescription>
        </DialogHeader>

        <div className="space-y-3 max-h-[60vh] overflow-y-auto -mx-1 px-1">
          {COMMUNITY_LAYOUTS.map((layout) => (
            <LayoutCard
              key={layout.id}
              layout={layout}
              loading={loadingId === layout.id}
              onLoad={() => {
                load(layout).then(() => onClose());
              }}
            />
          ))}
        </div>
      </DialogContent>
    </Dialog>
  );
}

export function CommunityLayoutsButton() {
  const [open, setOpen] = useState(false);
  return (
    <>
      <Button
        variant="outline"
        size="sm"
        onClick={() => setOpen(true)}
        title="Browse community layouts"
        data-tour="community-layouts"
      >
        <Users className="h-4 w-4" />
        <span className="hidden md:inline ml-2">Layouts</span>
      </Button>
      <CommunityLayoutsModal isOpen={open} onClose={() => setOpen(false)} />
    </>
  );
}

// ─── Inline section on the page ──────────────────────────────────────────────

export function CommunityLayoutsSection() {
  const { loadingId, load } = useLoadLayout();

  return (
    <Card data-tour="community-section">
      <CardHeader>
        <div className="flex items-center gap-2">
          <Users className="h-4 w-4" />
          <CardTitle className="text-base">Community layouts</CardTitle>
        </div>
        <CardDescription>
          Layouts shared by the community. Click "Load" to import one as a starting point,
          or "View on GitHub" to see its source. Submit yours via PR.
        </CardDescription>
      </CardHeader>
      <CardContent>
        <div className="grid gap-3 grid-cols-1 md:grid-cols-2 lg:grid-cols-3">
          {COMMUNITY_LAYOUTS.map((layout) => (
            <LayoutCard
              key={layout.id}
              layout={layout}
              loading={loadingId === layout.id}
              onLoad={() => load(layout)}
            />
          ))}
        </div>
      </CardContent>
    </Card>
  );
}
