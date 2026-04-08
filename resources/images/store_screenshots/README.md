# Store / listing screenshots (marketing)

These PNGs are **not** bundled into the `.pbw`. Commit them here (or under `store-assets/`) so GitHub + designers have the same files you upload to the [Rebble App Store](https://apps.rebble.io/) listing.

## How to capture

1. Enable **demo data** briefly (see `store-assets/README.md`: `TRIO_ENABLE_DEMO_GRAPH` in `demo_preview.h`), build, run **CloudPebble emulator** or hardware.
2. Use **CloudPebble → Compilation → Screenshot** (or OS capture) for each target.
3. Save into this folder with clear names, e.g.  
   `emery-classic.png`, `diorite-classic.png`, `basalt-classic.png`, …

## Sizes (typical native LCD)

| Platform | Resolution |
|----------|------------|
| aplite / basalt / diorite | 144×168 |
| chalk | 180×180 (round) |
| emery | 200×228 |

Crop **full LCD** (no phone bezel) for a consistent gallery.

## After capture

Turn **`TRIO_ENABLE_DEMO_GRAPH`** back to **`0`** before shipping a release PBW.
