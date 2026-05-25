// Copies the bundled Keyflow config from the parent repo into designer/public/
// so the screenshot route (and any future doc-rendering routes) can fetch it
// at runtime. Wired as `predev` / `prebuild` in package.json — Turbopack
// refuses to resolve imports outside the designer/ project root, hence the
// build-time sync instead of a direct ../../src import.

import { copyFileSync, mkdirSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = dirname(fileURLToPath(import.meta.url));
const src = resolve(__dirname, "../../src/config.json");
const dest = resolve(__dirname, "../public/eduuh-dh-config.json");

mkdirSync(dirname(dest), { recursive: true });
copyFileSync(src, dest);
console.log(`copy-config: ${src} → ${dest}`);
