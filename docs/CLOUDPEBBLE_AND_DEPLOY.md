# CloudPebble settings, previews, and phone deployment

## The three checkboxes (Settings → your project)

These should match what the app actually uses. The repo’s `appinfo.json` / `package.json` now declare:

| Checkbox in CloudPebble | Enable when | This project |
|-------------------------|-------------|--------------|
| **Configurable** | You use a hosted settings page + `Pebble.openURL` / Clay | **Yes** – `config/index.html` |
| **Uses health** | C code uses `health_service_*` (steps, heart rate, etc.) | **Yes** – `complications.c` |
| **Uses location** | PebbleKit JS uses `navigator.geolocation` (or similar) | **Yes** – `fetchWeather()` in `index.js` |

After importing from GitHub, open **Settings** in CloudPebble and ensure all three match the table. If CloudPebble ever disagrees with `appinfo.json`, prefer editing the repo and re-importing / syncing so the source of truth stays Git.

If a build ever complains about an unknown capability, remove **Uses location** from the checkbox first and from `capabilities` in `appinfo.json` / `package.json`; weather will then only work when the phone still grants location at runtime (behavior depends on the Rebble app).

---

## Why Settings showed 404

**Settings** opens a normal browser/WebView URL. The companion runs `showConfiguration` in `src/pkjs/index.js`, which calls `Pebble.openURL(...)` with that URL.

Previously the URL pointed at **GitHub Pages** (`minimusclawdius.github.io/...`). That returns **404** until you enable GitHub Pages on the repo and publish the `config/` folder.

The default URL is now **jsDelivr**, which serves the public GitHub file with a correct `text/html` MIME type, so Settings works **without** enabling Pages:

`https://cdn.jsdelivr.net/gh/MinimusClawdius/trio-pebble@main/config/index.html`

**If you fork the repo**, change `TRIO_CONFIG_PAGE_URL` in `src/pkjs/index.js` to your fork:

`https://cdn.jsdelivr.net/gh/<YOUR_USER>/<YOUR_REPO>@main/config/index.html`

**Alternatives:**

1. **GitHub Pages** – Repo → Settings → Pages → Branch `main`, folder `/ (root)`. Then you can set the URL to  
   `https://<user>.github.io/<repo>/config/index.html`
2. **Private repo** – jsDelivr/GitHub raw CDN won’t serve your config; host `config/index.html` on any HTTPS host you control and point `TRIO_CONFIG_PAGE_URL` there.

Rebuild the `.pbw` after changing the URL.

---

## No preview image in the Pebble / Rebble app

Sideloaded and dev builds often show a **generic** tile until you add a **menu icon** resource.

1. Create a square PNG (commonly **25×25** for legacy, or follow CloudPebble’s current hint for menu icons).
2. Put it in the project, e.g. `resources/images/menu_icon.png`.
3. In `appinfo.json`, under `resources.media`, add:

```json
{
  "type": "png",
  "name": "IMAGE_MENU_ICON",
  "file": "images/menu_icon.png"
}
```

Mirror the same entry under `pebble.resources.media` in `package.json` if CloudPebble reads from npm layout.

**Store / listing screenshots** (for Rebble App Store later) are separate from the menu icon; you upload those in the store submission flow, not inside the `.pbw` alone.

---

## Deploy path: CloudPebble → phone → watch

### A. Install from CloudPebble (recommended while iterating)

1. In CloudPebble: **Compile** (success).
2. **Phone** – Install **Rebble** (or legacy Pebble app with Rebble services), log in, Bluetooth on.
3. CloudPebble: **Install and run** / send to phone (exact label varies). The IDE talks to your account; the phone receives the app.
4. On the watch: select the new watchface if it doesn’t auto-switch.

### B. Download `.pbw` and install manually

1. CloudPebble: download the built `.pbw`.
2. Open the file on the phone (AirDrop, Files, email, etc.) and choose **Open in Rebble** (or Pebble).
3. Confirm install to the watch.

**Bluetooth proxy:** Some flows use the phone app as a proxy; keep the Pebble connected and the app in foreground for the first install.

### C. After changing Settings URL or capabilities

Always **rebuild** in CloudPebble and **reinstall** the `.pbw` to the watch so the new JavaScript and manifest are on the phone.

---

## Quick checklist

- [ ] **Configurable** + **Uses health** + **Uses location** enabled (or match trimmed capabilities if you dropped location).
- [ ] Settings URL loads in **Safari/Chrome on the phone** before blaming the watch.
- [ ] Optional: add `IMAGE_MENU_ICON` for a nicer list tile.
- [ ] Trio API: only works from the phone when Trio’s local server is on the same device (`127.0.0.1`).
