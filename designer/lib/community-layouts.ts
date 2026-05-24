// Community keyboard layouts. Each entry references a config.json hosted
// on GitHub (or anywhere with CORS-allowed raw access). Users can browse
// the gallery, click into a layout to see details, and "Load" it into
// the local designer state for inspection or as a starting point.
//
// To submit a new layout: open a PR adding an entry here.
//
// `rawUrl`: the URL that returns the raw JSON config (used by the Load
//   action — must be CORS-allowed, e.g. raw.githubusercontent.com works).
// `sourceUrl`: a human-friendly link to the file on its hosting page
//   (e.g. github.com/.../blob/...). Shown as "View on GitHub".
// `authorUrl`: link to the author's GitHub profile or homepage.

export type CommunityLayout = {
  id: string;
  name: string;
  description: string;
  author: string;
  authorUrl: string;
  rawUrl: string;
  sourceUrl: string;
  /** Optional one-line tag like "Colemak-DH-inspired" or "Space-Cadet". */
  badge?: string;
};

export const COMMUNITY_LAYOUTS: CommunityLayout[] = [
  {
    id: "eduuh-dh",
    name: "Eduuh-DH",
    description:
      "Personal Colemak-DH-inspired layout with right-hand letters shifted right, " +
      "apostrophe repurposed as a letter slot, and aggressive number-row symbol " +
      "remaps for programming. Ships as the default config in the Keyflow repo.",
    author: "Edwin Muraya",
    authorUrl: "https://github.com/eduuh",
    rawUrl: "https://raw.githubusercontent.com/eduuh/keyflow/main/src/config.json",
    sourceUrl: "https://github.com/eduuh/keyflow/blob/main/src/config.json",
    badge: "Colemak-DH-inspired",
  },
];
