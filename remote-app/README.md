# Trio Remote (Pebble watch app)

Companion **watch app** for bolus / carbs (full button support). Install **alongside** the **Trio** watchface from the repository root — they are **two different apps** and must have **two different UUIDs**.

## CloudPebble

GitHub import of **`trio-pebble` uses only the repo root** `package.json` (the watchface). **Do not** try to ship both targets from that single project without a separate manifest; you will get **one UUID** and the second install will **replace** the first.

Create a **second CloudPebble project** whose project root is the contents of this **`remote-app/`** folder (same layout: `package.json`, `wscript`, `src/`). See the parent repo **[docs/CLOUDPEBBLE_AND_DEPLOY.md](../docs/CLOUDPEBBLE_AND_DEPLOY.md)** section *Watchface + Trio Remote*.

## Local build

```bash
pebble build
pebble install --phone <ip>
```
