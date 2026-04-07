# Rebble / Pebble App Store visuals

## What the store actually uses

- **Listing screenshots** — You upload **PNG or JPEG** images in the [Rebble App Store](https://apps.rebble.io/) (or publisher) submission UI. They are **not** embedded inside the `.pbw`; they are separate marketing assets.
- **App list / phone preview tile** — Comes from the **`menuIcon`** resource in `package.json` (this repo: `resources/images/menu_icon.png`, max **25×25** px). CloudPebble’s project preview uses the same.

So: commit **screenshot PNGs here** (optional) for version control and designer handoff, then attach those files when you publish the watchface.

## Simulated graph + BG for screenshots

The firmware can show **fake but realistic** CGM data (graph, glucose, IOB/COB, complications) **without a phone**:

1. Open `src/modules/demo_preview.h`.
2. Set **`#define TRIO_ENABLE_DEMO_GRAPH 1`** (and keep **`0`** for every real-user release).
3. Build in **CloudPebble** or locally (`pebble build`).
4. Run the **emulator** (Basalt = color rectangle is a good default), press **UP/DOWN** to cycle all six faces, **capture** each layout you want in the listing.
5. Set **`TRIO_ENABLE_DEMO_GRAPH`** back to **`0`**, rebuild, and ship that PBW.

Suggested emulator targets (native resolution = crisp crops):

| Platform   | Resolution (typical) |
|-----------|------------------------|
| Aplite    | 144 × 168              |
| Basalt    | 144 × 168              |
| Chalk     | 180 × 180 (round)      |
| Diorite   | 144 × 168              |
| Emery     | 200 × 228              |

Rebble’s uploader may accept various sizes; crop consistently (e.g. full watch LCD, no bezel) so the gallery looks coherent.

## Naming pattern (recommended)

Use **`{platform}-{face-slug}.{ext}`** so filenames sort by platform then layout:

| Segment | Examples |
|--------|-----------|
| **platform** | `aplite`, `basalt`, `chalk`, `diorite`, `emery` |
| **face-slug** | `classic`, `graph-focus`, `compact`, `dashboard`, `minimal`, `retro` |
| **ext** | `png` (preferred) or `jpg` |

Examples: `basalt-classic.png`, `chalk-minimal.png`, `emery-dashboard.png`.

## Putting screenshots in this folder

`.gitignore` in this directory **only** excludes OS junk (`.DS_Store`, `Thumbs.db`, etc.); **PNG/JPEG listing shots are tracked** — commit them whenever you like.

Example layout:

```
store-assets/
  .gitignore
  README.md
  basalt-classic.png
  basalt-graph-focus.png
  chalk-minimal.png
  …
```

## CloudPebble “Import from GitHub”

Yes — this repo is structured for that flow:

- **Root manifest:** `package.json` only (see `docs/CLOUDPEBBLE_AND_DEPLOY.md` — avoid a competing root `appinfo.json` or the importer may pick the wrong file).
- **Sources:** `src/**/*.c` picked up by `wscript`.
- **New files:** `src/modules/demo_preview.c` is included automatically via the glob; no `wscript` change needed.
- After push to GitHub: CloudPebble → **Import** / **Pull** → **Compile** → emulator.

Enable demo mode only on a **branch** or remember to flip **`TRIO_ENABLE_DEMO_GRAPH`** back before tagging a release.
