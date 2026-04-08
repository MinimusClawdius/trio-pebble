# T1000-style trend arrow assets

Trio bundles the CGM **trend direction** PNGs from **[andrewchilds/t1000-pebble-cgm](https://github.com/andrewchilds/t1000-pebble-cgm)** under `resources/images/`:

- `trend_*.png` — intended for **dark** watchface backgrounds  
- `trend_*_black.png` — intended for **light** (e.g. white) backgrounds  

They are declared in **`package.json`** → `pebble.resources.media` as `TRIO_TREND_*` resources.  
Do **not** re-add the same files again in **CloudPebble → Resources** (duplicate `RESOURCE_ID_*` breaks the build).

Check the upstream repo for its license before redistributing forks.
