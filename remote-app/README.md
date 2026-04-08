# Trio Remote (inside `trio-pebble` monorepo)

This folder is the **Trio Remote** watch app for **local `pebble build`** next to the watchface.

**CloudPebble:** there is no way to point a project at this subfolder. Use a **second GitHub repository** — **`trio-pebble-remote`** — and import **that** repo in CloudPebble. See the parent [README.md](../README.md) and [docs/CLOUDPEBBLE_AND_DEPLOY.md](../docs/CLOUDPEBBLE_AND_DEPLOY.md).

After editing here, sync the standalone repo (sibling folder):

`pwsh -File ../trio-pebble/scripts/sync-trio-pebble-remote.ps1`

Then add **`resources/images/menu_icon.png`** in `trio-pebble-remote` if you only synced sources.

## Local build

```bash
pebble build
pebble install --phone <ip>
```
