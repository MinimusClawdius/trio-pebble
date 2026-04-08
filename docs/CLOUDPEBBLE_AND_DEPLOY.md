# CloudPebble settings, previews, and phone deployment

**See also:** General Pebble + CloudPebble reference (watchface vs app, SDK, IDE, manifest, troubleshooting): **[CLOUDPEBBLE_COMPREHENSIVE_GUIDE.md](CLOUDPEBBLE_COMPREHENSIVE_GUIDE.md)**.  
**Watch logs:** **[WATCH_APP_LOGS.md](WATCH_APP_LOGS.md)**.  
**Learn C on watch:** [Learning C with Pebble](https://pebble.gitbooks.io/learning-c-with-pebble/) (GitBook, project exercises with CloudPebble).

## `package.json`: version, `sdkVersion`, and GitHub import

- **`"version"`** — If CloudPebble import or version display misbehaves with a two-part npm version (e.g. `"2.6"`), use **three-part semver** (e.g. **`"2.7.0"`**).  
- **Release numbering (this repo)** — Iterate **`2.8.0` → `2.9.0` → `3.0.0`** (then `3.1.0`, …) for user-facing drops; the final **`.0`** is a fixed placeholder (patch digit unused unless we adopt patches later). Keep **`APP_VERSION`** in `trio_types.h`, **`package.json`** `"version"`, and the config page footer in sync.  
- **`pebble.sdkVersion`** — Must match what **your** CloudPebble host uses (check a **new empty project** on the same site). Example that has worked on Rebble-hosted CloudPebble: **`"4.9.148"`** instead of **`"3"`**.  
- **Trend / extra PNGs** — This repo lists **many** `pebble.resources.media` entries (trend arrows). Keep the **menu icon** only in **one** place (usually CloudPebble **Resources** UI with `pebble.resources.media` as `[]` for that icon **or** only in JSON—never both). Do **not** duplicate **trend** files in the CloudPebble Resources table if they are already in `package.json`.
- **`menuIcon` must be 25×25** — The launcher icon cannot use a trend arrow PNG. This repo declares **`"menuIcon": "IMAGE_MENU_ICON"`** and **`resources/images/menu_icon.png`** (25×25) in `package.json`. If CloudPebble still errors (e.g. `TRIO_TREND_NONE` exceeds 25×25), remove any **wrong** menu bitmap row from **CloudPebble → Resources** and **Pull from GitHub** so `package.json` wins.

## GitHub-linked CloudPebble: version and preview (why it looked “empty”)

Rebble’s CloudPebble import walks the GitHub zip and picks the **first** valid manifest it finds at the project root. If both `appinfo.json` and `package.json` exist, **whichever appears first in the zip** wins—not necessarily `package.json`.

- **Wrong or missing version in the CloudPebble UI** — For npm-style projects, **`package.json` top-level `"version"`** is what becomes the project semver (e.g. `2.2.5`). If CloudPebble imported **`appinfo.json` first**, it would use **`versionLabel`** from that file and **ignore** npm `version`, and it would **not** load `pebble.messageKeys` from `package.json`. This repo keeps **only `package.json`** at the root. Human-only legacy field reference: **`docs/pebble-legacy-manifest-reference.md`** (not a manifest; do not duplicate resources there).
- **`RESOURCE_ID_IMAGE_MENU_ICON` redefined / `menu_icon.reso` created twice** — The build fails when **`IMAGE_MENU_ICON`** is declared twice (e.g. **`menuIcon` + media entry in `package.json`** **and** the same resource row in **CloudPebble → Resources**). This repo ships **`menuIcon`** and **`IMAGE_MENU_ICON`** in **`package.json`** only. After **Pull from GitHub**, **remove** any duplicate **`IMAGE_MENU_ICON`** / menu-icon row from **CloudPebble → Resources** so only the manifest defines it.
- **No emulator / list preview image** — Same rule: **one** definition of the launcher icon. Repo default: **`resources/images/menu_icon.png`** via **`package.json`**; do not mirror it in the IDE **Resources** table.

**After changing any of this**, use **GitHub → Pull** in CloudPebble (or re-import the project) so metadata and resources refresh.

### Description and “release notes” in CloudPebble

GitHub sync only repopulates fields that come from the Pebble manifest (`package.json` → UUID, names, version, capabilities, message keys, resources, etc.). The CloudPebble **Project** model does not map `package.json` **`description`** or npm metadata into the IDE’s description / release-notes style fields, and **build logs** do not carry store-style release text. Keep long descriptions and per-build notes **in the CloudPebble project UI** (or your store listing), and use the repo **`description`** field for GitHub/npm readability only.

### Menu icon: PNG is supported; size is strict

The SDK enforces a **maximum 25×25** pixel size for **`menuIcon`** bitmaps. Larger PNGs (e.g. 28×28) fail at compile time with  
`menuIcon resource 'IMAGE_MENU_ICON' exceeds the maximum allowed dimensions of (25, 25)`.  
**GIF (and JPEG) are not valid Pebble app resource types** for bundled images—the resource pack expects **PNG** (or fonts / raw data). This project ships a **palettized** 25×25 PNG, which avoids some tools choking on full RGBA thumbnails. If the CloudPebble Resources preview still fails after a green build, treat it as an IDE quirk; the **phone launcher** icon comes from the same asset in the `.pbw`.

---

## The three checkboxes (Settings → your project)

These should match what the app actually uses. The repo’s **`package.json`** (`pebble.capabilities`) declares:

| Checkbox in CloudPebble | Enable when | This project |
|-------------------------|-------------|--------------|
| **Configurable** | You use a hosted settings page + `Pebble.openURL` / Clay | **Yes** – `config/index.html` |
| **Uses health** | C code uses `health_service_*` (steps, heart rate, etc.) | **Yes** – `complications.c` |
| **Uses location** | PebbleKit JS uses `navigator.geolocation` (or similar) | **Yes** – `fetchWeather()` in `index.js` |

After importing from GitHub, open **Settings** in CloudPebble and ensure all three match the table. Edit **`package.json`** in the repo and pull again if CloudPebble disagrees.

If a build ever complains about an unknown capability, remove **Uses location** from the checkbox first and from `pebble.capabilities` in `package.json`; weather will then only work when the phone still grants location at runtime (behavior depends on the Rebble app).

---

## Settings page: 404, raw HTML, or blank

**Settings** uses `Pebble.openURL(...)` in `src/pkjs/index.js` (`TRIO_CONFIG_PAGE_URL`).

### Raw HTML / “source code” instead of a form

CDNs such as **jsDelivr** and **raw.githubusercontent.com** often respond with:

- `Content-Type: text/plain; charset=utf-8`
- `X-Content-Type-Options: nosniff`

Rebble’s WebView then **does not render** the page; you see the HTML as plain text.

**Fix:** host `config/index.html` somewhere that serves it as **`text/html`**. The default URL is **GitHub Pages**:

`https://minimusclawdius.github.io/trio-pebble/config/index.html`

Enable it once per repo:

1. GitHub → **trio-pebble** → **Settings** → **Pages**
2. **Build and deployment** → Source: **Deploy from a branch**
3. Branch: **`main`**, folder: **`/ (root)`** → Save  
4. Wait a minute, then open that URL in **Safari/Chrome**; you should see the styled form (not angle-bracket soup).

**If you fork the repo**, change `TRIO_CONFIG_PAGE_URL` in `src/pkjs/index.js` to:

`https://<your-username>.github.io/<your-repo>/config/index.html`

(and enable Pages on that fork).

### 404 on the Pages URL

Pages is not enabled yet, or the site is still building. Confirm the **Pages** section shows a green check and the published URL.

### Private repo / custom host

Host `config/index.html` on any HTTPS server that returns **`Content-Type: text/html`** and set `TRIO_CONFIG_PAGE_URL` accordingly.

Rebuild / reinstall the `.pbw` after changing the URL.

---

## Trio `http://127.0.0.1:8080` — Safari shows nothing / connection failed

The Pebble API runs **inside the Trio iOS app**, bound to **loopback on that iPhone only**.

### iOS suspends Trio when you leave the app

If you open **Safari** (or **Rebble**) as the frontmost app, **Trio moves to the background**. iOS soon **suspends** it; the socket server **stops accepting connections**. Hitting `http://127.0.0.1:8080` from Safari then often shows **nothing**, **connection refused**, or a spinner.

**How to sanity-check the server**

1. **Trio** in **foreground** (don’t switch away yet).
2. **Watch → Pebble** → enable Pebble; confirm status shows **Running** (or check Trio logs for `Pebble: API server started`).
3. On **iPad**: open **Split View** / Slide Over with **Safari** + **Trio** so Trio stays eligible to run while you load `http://127.0.0.1:8080/api/health` or `/api/all`.
4. On **iPhone** (no split screen): you usually **cannot** keep both Safari and Trio fully active; expect localhost checks from Safari to **fail** unless you switch back to Trio quickly.

### Rebble + Trio local API

PebbleKit JS also runs in the **Rebble** app. When Rebble is foreground, Trio is typically **background** → local HTTP to `127.0.0.1` is **unreliable** on iOS. For day-to-day use, prefer **Nightscout** or **Dexcom Share** in the watchface settings when the phone won’t keep Trio alive, or open Trio briefly so a fetch can succeed.

### Requirements

- **Pebble integration** enabled in Trio; Trio has pushed at least one watch state (so the bridge has data).
- Port matches Trio (default **8080**).

---

## No preview image in the Pebble / Rebble app

Sideloaded and dev builds often show a **generic** tile until you add a **menu icon** resource.

1. Use a square PNG at **`resources/images/menu_icon.png`**, **exactly 25×25** (or smaller) for `menuIcon` (this repo ships one).
2. **CloudPebble (GitHub-linked):** In **Resources**, add **one** PNG resource: name **`IMAGE_MENU_ICON`**, file **`images/menu_icon.png`**, enable **menu image** / launcher icon. Do **not** also paste the same entry into `package.json` while that row exists in the IDE (duplicate → `RESOURCE_ID_IMAGE_MENU_ICON` redefined).
3. **Local `pebble build` (CLI) only:** If you are **not** merging CloudPebble Resources, add this single object to **`pebble.resources.media`** in `package.json` (and keep Resources empty / in sync):

```json
{
  "menuIcon": true,
  "type": "png",
  "name": "IMAGE_MENU_ICON",
  "file": "images/menu_icon.png"
}
```

**Store / listing screenshots** (for Rebble App Store later) are separate from the menu icon; you upload those in the store submission flow, not inside the `.pbw` alone.

### Store screenshots with simulated CGM (demo graph)

To capture **filled graphs and BG** in the emulator without Trio/phone data:

1. Set **`TRIO_ENABLE_DEMO_GRAPH`** to **`1`** in `src/modules/demo_preview.h` (must be **`0`** for production PBWs).
2. Build and run the **CloudPebble emulator** (or local SDK); use **UP/DOWN** to show each face layout.
3. Export PNGs and upload them in the **Rebble App Store** listing; optionally commit copies under **`store-assets/`** for the repo.

Details, resolution table, and CloudPebble import notes: **`store-assets/README.md`**.

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

## Watchface + Trio Remote (two PBWs, two UUIDs)

### Why installing the “watchapp” removed the watchface

On Pebble / Rebble, each installed item is keyed by its **`uuid`** in `package.json` (same idea as Android `applicationId`). **Installing a `.pbw` replaces whatever is already installed with that exact UUID** — it is not “watchface slot” vs “app slot” at the UUID level. Two different UUIDs = **two separate installs** (one can be a watchface, one a watchapp).

If both builds used the **same** manifest (same UUID), the second install would **overwrite** the first. That usually happens on CloudPebble when:

1. **One CloudPebble project** is linked to this GitHub repo’s **root** (see [GitHub-linked CloudPebble](#github-linked-cloudpebble-version-and-preview-why-it-looked-empty) — the import uses the **root** `package.json`, UUID `a1b2c3d4-e5f6-7890-abcd-ef1234567890`).
2. You then change **`watchface`** to `false`, swap source files toward `remote-app/`, and **Compile** again — the project still **Pull**s the repo’s **root** `package.json` on sync, so the UUID often stays the watchface UUID unless you maintain a **fork** or never Pull.
3. You install that PBW → it **replaces** the previous install with that UUID (your watchface).

So the conflict is **not** “watchface vs app container” in the abstract — it is **same UUID = same install slot**.

### Fix: two CloudPebble projects (recommended)

| Project | Source | `package.json` | UUID (this repo) |
|---------|--------|----------------|------------------|
| **A – Trio watchface** | GitHub: `trio-pebble` **root** | Repo root `package.json` | `a1b2c3d4-e5f6-7890-abcd-ef1234567890` |
| **B – Trio Remote** | **Not** the same root-only import | Must be **`remote-app/package.json`** at the **CloudPebble project root** | See `remote-app/package.json` (`fdd1d4b6-…` in current tree) |

**Project B setup (typical):**

1. In CloudPebble: **Create project** → native **watchapp** (not watchface), or import a **minimal** repo/zip whose **root** is the contents of `remote-app/` (i.e. `package.json` + `wscript` + `src/` from `remote-app/` only).
2. Paste **`remote-app/package.json`** as the project manifest (or maintain a small fork / branch where `remote-app/` is the repository root).
3. Copy **`remote-app/wscript`** and **`remote-app/src/**`** into that project (same paths as in this repo: `src/main.c`, `src/remote_menu.c`, `src/pkjs/index.js`).
4. **Resources:** ensure **`IMAGE_MENU_ICON`** exists (this repo references `../images/menu_icon.png` from `remote-app/`; on CloudPebble you may need a **Resources** row or copy the PNG into `remote-app/resources/` and point `file` at it).
5. **Compile** → install PBW **B**. Confirm in Rebble / app list you now have **both** “Trio Pebble” (watchface) and “Trio Remote” (app).

**Verify:** Open each project’s **Settings** in CloudPebble and confirm the **UUID** field differs between project A and project B before trusting installs.

### Alternative: local CLI (no CloudPebble ambiguity)

From a machine with the Pebble SDK:

```bash
# Watchface
cd /path/to/trio-pebble
pebble build && pebble install --phone <ip>

# Remote app (different folder → different package.json → different UUID)
cd /path/to/trio-pebble/remote-app
pebble build && pebble install --phone <ip>
```

---

## Quick checklist

- [ ] **Trio Remote:** separate CloudPebble project (or local build from `remote-app/`) so its **UUID ≠** watchface UUID; see [Watchface + Trio Remote](#watchface--trio-remote-two-pbws-two-uuids).
- [ ] **Configurable** + **Uses health** + **Uses location** enabled (or match trimmed capabilities if you dropped location).
- [ ] Settings URL loads in **Safari/Chrome on the phone** before blaming the watch.
- [ ] Optional: add `IMAGE_MENU_ICON` once (CloudPebble **Resources** *or* `package.json` media—not both).
- [ ] Trio API: only works from the phone when Trio’s local server is on the same device (`127.0.0.1`).
