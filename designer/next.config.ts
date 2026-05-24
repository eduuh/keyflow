import type { NextConfig } from "next";

// Deployment notes
// ─────────────────
// The designer ships as a fully static site to GitHub Pages at
//   https://eduuh.github.io/keyflow/
// `npm run build` produces an /out/ directory the workflow uploads as a
// Pages artifact. basePath/assetPrefix are only applied when GITHUB_PAGES=1
// (set by the deploy workflow) so local `npm run dev` keeps serving at
// http://localhost:3000/ — important because the Playwright tests assume
// the root path.

const isPagesBuild = process.env.GITHUB_PAGES === "1";

const nextConfig: NextConfig = {
  reactStrictMode: true,

  // Static export — no Next.js server, no API routes, no Image
  // Optimization at runtime. Everything ships as plain HTML/CSS/JS.
  output: "export",

  // Pages serves /keyflow/path/ rather than /path/. trailingSlash makes
  // every route emit an index.html so static hosts (which don't rewrite)
  // resolve the URL.
  trailingSlash: true,

  // Tell Next.js to skip its built-in image optimization (which requires
  // a Node runtime). All <Image> sources are passed through unchanged.
  images: { unoptimized: true },

  ...(isPagesBuild
    ? {
        basePath: "/keyflow",
        assetPrefix: "/keyflow/",
      }
    : {}),
};

export default nextConfig;
