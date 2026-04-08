# CloudPebble & Pebble SDK: comprehensive watchface & watch app reference

**Purpose:** One markdown file you can **load into an AI context window** (or keep open locally) while building Pebble **watchfaces** or **watch apps** in **[CloudPebble (Rebble)](https://cloudpebble.repebble.com)** or a matching **SDK 3** repo layout. It merges **IDE / import / build** behavior (from the open-source CloudPebble codebase) with **product-level** guidance for C-native projects.

**Repo:** This file lives in the **trio-pebble** project under `docs/`. For **Trio-specific** CloudPebble import quirks (menu icon, `package.json` vs `appinfo.json`, capabilities), see **[CLOUDPEBBLE_AND_DEPLOY.md](CLOUDPEBBLE_AND_DEPLOY.md)** in the same folder.

**Primary links**

| What | URL |
|------|-----|
| Live IDE | [cloudpebble.repebble.com](https://cloudpebble.repebble.com) |
| CloudPebble source | [github.com/coredevices/cloudpebble](https://github.com/coredevices/cloudpebble) |
| Official API / design docs | [developer.repebble.com/guides](https://developer.repebble.com/guides/) |
| **Learning C with Pebble** (book) | [pebble.gitbooks.io/learning-c-with-pebble](https://pebble.gitbooks.io/learning-c-with-pebble/) |

Mike Jipping’s open **GitBook** teaches C in the Pebble context: variables through file I/O, UI, graphics, communication, and debugging—with **project exercises** aimed at **CloudPebble** and the emulator/hardware. It predates Rebble but remains a strong complement to the official guides above.

---

## Table of contents

1. [Watchface vs watch app (decision + behavior)](#1-watchface-vs-watch-app-decision--behavior)  
2. [CloudPebble architecture & account setup](#2-cloudpebble-architecture--account-setup)  
3. [CloudPebble IDE: end-to-end workflow](#3-cloudpebble-ide-end-to-end-workflow)  
4. [Manifest (`package.json` / `appinfo.json`) reference](#4-manifest-packagejson--appinfojson-reference)  
5. [Target platforms & hardware matrix](#5-target-platforms--hardware-matrix)  
6. [Resources (images, fonts, raw, menu icon)](#6-resources-images-fonts-raw-menu-icon)  
7. [Build system (`wscript`, defines, `pebble build`)](#7-build-system-wscript-defines-pebble-build)  
8. [C project anatomy: init, window, main loop](#8-c-project-anatomy-init-window-main-loop)  
9. [Graphics, layers, and drawing](#9-graphics-layers-and-drawing)  
10. [Time, ticks, and scheduling](#10-time-ticks-and-scheduling)  
11. [Text and fonts](#11-text-and-fonts)  
12. [Bitmaps and palettes](#12-bitmaps-and-palettes)  
13. [Animation](#13-animation)  
14. [Input: buttons and click recognizers](#14-input-buttons-and-click-recognizers)  
15. [Windows, navigation, and common UI patterns](#15-windows-navigation-and-common-ui-patterns)  
16. [Vibration, backlight, battery-conscious design](#16-vibration-backlight-battery-conscious-design)  
17. [Persistence (`persist_*`)](#17-persistence-persist_)  
18. [AppMessage, AppSync, and PebbleKit JS](#18-appmessage-appsync-and-pebblekit-js)  
19. [Capabilities: configurable, health, location](#19-capabilities-configurable-health-location)  
20. [Background worker (overview)](#20-background-worker-overview)  
21. [Timeline pins (overview)](#21-timeline-pins-overview)  
22. [GitHub layout, import, and CloudPebble finder rules](#22-github-layout-import-and-cloudpebble-finder-rules)  
23. [Troubleshooting](#23-troubleshooting)  
24. [Checklists: new watchface, new app, pre-release](#24-checklists-new-watchface-new-app-pre-release)  
25. [Upstream CloudPebble source map](#25-upstream-cloudpebble-source-map)  

**Appendices (extra depth for context retrieval)**  
[A. Preprocessor symbols, memory, teardown](#appendix-a-preprocessor-symbols-memory-teardown) · [B. Pebble.js, Rocky, packages](#appendix-b-pebblejs-rocky-packages) · [C. Sensors & system services](#appendix-c-sensors--system-services) · [D. i18n & locale](#appendix-d-i18n--locale) · [E. Anti-patterns & sharp edges](#appendix-e-anti-patterns--sharp-edges)  

---

## 1. Watchface vs watch app (decision + behavior)

### 1.1 Manifest flag

- In `package.json` → `pebble.watchapp.watchface`: **`true`** = watchface, **`false`** = application.  
- CloudPebble **Settings** exposes the same concept; it must stay consistent with the manifest you commit on GitHub.

### 1.2 User-visible difference

| Aspect | Watchface | Watch app |
|--------|-----------|-----------|
| **Primary role** | Replaces the **clock screen**; user usually returns here by timeout or system gesture. | Lives in the **app launcher**; user **exits** back to watchface. |
| **Expectation** | Glanceable, **low interaction**, updates on **minute/second** ticks common. | Can use **menus**, **multi-screen flows**, heavier interaction. |
| **Buttons** | Often **reserved** for system (e.g. switching faces, timeline); usable button behavior is **platform- and design-dependent**—test on real hardware. | Typically uses **`ClickRecognizer`** / `window_set_click_config_provider` for UP/DOWN/SELECT/BACK. |
| **Menu icon** | Still used where the system shows a **config** or **settings** entry; launcher tile rules depend on platform and how the face is launched. | **Menu icon** (e.g. 25×25 PNG) is standard for launcher grid/list. |

### 1.3 Engineering implications

- **Watchfaces** usually subscribe to **`tick_timer_service`** (minute or second) and redraw via **`layer_mark_dirty`** on one root `Layer` or a few child layers.  
- **Apps** usually push a **`Window`** (or stack of windows), subscribe to **input**, and may use **`MenuLayer`**, **`ActionBarLayer`**, **`ScrollLayer`**, etc.  
- **Do not** assume the same button map works on **round** vs **rectangular** displays; use bounds-driven layout (`layer_get_bounds`, `GRect`).

### 1.4 When to choose which

- **Choose a watchface** for always-on time, weather-at-a-glance, minimal config, step count on the clock screen, etc.  
- **Choose an app** for multi-step tasks, lists, games, settings-heavy tools, anything that should not own the clock screen 24/7.

---

## 2. CloudPebble architecture & account setup

### 2.1 What CloudPebble is

From [coredevices/cloudpebble README](https://github.com/coredevices/cloudpebble/blob/main/README.md):

| Piece | Role |
|--------|------|
| **Browser SPA** | Editor (e.g. CodeMirror), project UI, compile triggers, emulator hooks |
| **Django API** | Projects, files, builds |
| **Celery** | Runs **`pebble build`**, zip/GitHub import, export |
| **Object storage** | Sources + **`.pbw`** artifacts |
| **QEMU profile** (optional) | In-browser emulator; not guaranteed on every hosted instance |

**Build pipeline (simplified):** Compile in UI → `BuildResult` → worker assembles tree → `npm install` if `dependencies` → `pebble sdk activate` → **`pebble build -v`** → store `.pbw`.

### 2.2 Sign in

Use a Rebble/Pebble account or linked identity (Google, GitHub, Apple) as offered on the site.

### 2.3 GitHub Repo Sync

Production ([cloudpebble.repebble.com](https://cloudpebble.repebble.com)) expects **GitHub App install** + **linked account** so Pull/Push work. Self-hosted instances may need extra steps if `PUBLIC_URL` differs from production callbacks—see upstream README **GitHub Repo Sync** section.

---

## 3. CloudPebble IDE: end-to-end workflow

### 3.1 Create project

1. **Create** / new project → choose **Pebble C SDK** for native C watchfaces/apps (unless you intentionally use Pebble.js, Rocky, or a package template).  
2. Set **name**; CloudPebble seeds **`src/`**, **`resources/`**, **`package.json`**, **`wscript`**.

### 3.2 Settings (maps to manifest)

- **Watchface vs app**, **UUID**, **version**, **display name**, **company/author**.  
- **Target platforms** (subset of aplite, basalt, chalk, diorite, emery, …).  
- **Capabilities** (`configurable`, `health`, `location`, …)—must match code.  
- **Message keys** for AppMessage—must match C + any PKJS.

### 3.3 Source

- Edit **`src/*.c`** / headers; optional **`worker_src/`**.  
- If **multi-JS** enabled, maintain **`src/js/`** (or template paths) for PebbleKit JS.

### 3.4 Resources

- Add files under **`resources/`**; keep **`pebble.resources.media`** in sync.  
- **Never duplicate** the same logical resource in **both** GitHub manifest and CloudPebble-only UI rows on linked repos (duplicate `RESOURCE_ID_*` / duplicate media entries break builds).

### 3.5 Compilation

- **Run build** → status **Pending** → **Succeeded** / **Failed**.  
- **Build log** on failure is mandatory reading.  
- **Download `.pbw`** from the success row for sideloading.

### 3.6 Emulator (if enabled)

- Install last good build to **QEMU** target matching a selected platform.  
- Sessions are **short-lived** without activity (order of minutes per upstream docs).

### 3.7 Phone / watch

- **Developer connection** (or current Rebble cloud dev path) in phone app + **Install on phone** from CloudPebble, **or** download **`.pbw`** and **Open in Rebble** on the phone.  
- **Logs:** **Phone logs** / **Emulator logs** in the compile UI when connected.

### 3.8 Screenshots

- Use IDE **screenshot** action when connected to emulator or phone for store/marketing PNGs; keep high-res copies in repo (e.g. `store-assets/`) if you version them.

---

## 4. Manifest (`package.json` / `appinfo.json`) reference

### 4.1 Preferred layout

- **New native projects:** single root **`package.json`** with top-level **`"pebble": { ... }`**.  
- **Legacy:** `appinfo.json` + `src` with `.c`/`.js` still supported by tooling; avoid **two competing manifests** at the same root.

### 4.2 Common `pebble` keys (native)

| Key | Meaning |
|-----|---------|
| `displayName` | User-visible name |
| `uuid` | **Stable** UUID string for this product slot |
| `sdkVersion` | `"3"` for SDK 3 |
| `projectType` | `"native"` (typical) |
| `enableMultiJS` | `true` if using separate PKJS file(s) |
| `watchapp.watchface` | `true` / `false` |
| `watchapp.hidden` | Hide from launcher (rare) |
| `targetPlatforms` | Array of platform strings |
| `capabilities` | e.g. `["configurable","health"]` |
| `messageKeys` | Map of name → integer id (or ordered list per template) |
| `resources.media` | Array of resource descriptors |

### 4.3 Version rules (CloudPebble validation)

- **Django model / UI:** CloudPebble’s `Project.clean()` historically enforced Pebble-style **`major`** or **`major.minor`** for many **native** projects. In practice, **GitHub import** and npm-style workflows sometimes behave more reliably with a **three-part** top-level **`"version": "2.6.0"`** in `package.json` (semver). If import or version display fails with `2.6`, try **`2.6.0`**.  
- **Packages:** semver **`x.y.z`** is required.  
- **`pebble.sdkVersion`:** CloudPebble pins a worker SDK (see `ide/models/project.py` on the server, often a string like **`4.9.148`**). If import fails with **`"sdkVersion": "3"`**, set **`sdkVersion`** to the **same value your CloudPebble instance expects** (as shown in a freshly created project’s JSON on that host).

### 4.4 Minimal native example (watchface-oriented)

```json
{
  "name": "my-watchface",
  "version": "1.0",
  "author": "You",
  "pebble": {
    "displayName": "My Watchface",
    "uuid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
    "sdkVersion": "3",
    "projectType": "native",
    "enableMultiJS": false,
    "watchapp": { "watchface": true },
    "targetPlatforms": ["basalt", "chalk", "diorite", "emery"],
    "capabilities": [],
    "resources": {
      "media": [
        {
          "type": "png",
          "name": "IMAGE_MENU_ICON",
          "file": "images/menu_icon.png",
          "menuIcon": true
        }
      ]
    }
  }
}
```

---

## 5. Target platforms & hardware matrix

Use this when choosing **`targetPlatforms`** and when **branching** layout/drawing code.

| Platform | Typical device class | Display | Notes |
|----------|----------------------|---------|--------|
| **aplite** | Original Pebble, Steel | 1-bit B&W, 144×168 | Sharp contrast; limited anti-aliasing |
| **basalt** | Pebble Time | Color rectangular | `GColor` full UI |
| **chalk** | Pebble Time Round | Round color | Clip to circle; different corner geometry |
| **diorite** | Pebble 2 | B&W rectangular | Similar res to classic line in spirit |
| **emery** | Pebble Time 2 | Color, larger | More pixels—test layouts |

**Practices**

- Use **`PBL_PLATFORM_*`** / **`PBL_BW`** / **`PBL_COLOR`** / **`PBL_ROUND`** in C preprocessor where you must diverge.  
- Prefer **bounds-driven** layout over hard-coded `144`/`168` constants when possible.  
- **Round:** center content; watch for **corners** outside the visible circle.

---

## 6. Resources (images, fonts, raw, menu icon)

### 6.1 Declaring media

Each `media[]` entry needs at least:

- **`type`**: `png`, `font`, `raw`, etc.  
- **`name`**: becomes **`RESOURCE_ID_*`** in generated headers.  
- **`file`**: path relative to **`resources/`** (native layout).

Optional: **`menuIcon`**, **`targetPlatforms`** on an asset, font variant flags per SDK docs.

### 6.2 Menu icon

- Typically **25×25** PNG for launcher/menu contexts.  
- **Exactly one** `menuIcon: true` entry for standard native apps (unless you know you intentionally omit it).

### 6.3 PNG on aplite

- Monochrome platforms use **palette** PNGs; follow Pebble asset guidelines (limited palette, 1-bit semantics)—see Rebble **App Resources** guide.

### 6.4 Fonts

- Custom fonts declared as resources; load with **`fonts_load_custom_font`** and unload with **`fonts_unload_custom_font`** when done (avoid leaking GFont handles across reloads).

### 6.5 Raw

- Binary blobs (e.g. precomputed data); access via resource APIs appropriate to your use case.

---

## 7. Build system (`wscript`, defines, `pebble build`)

- **`wscript`** describes how `waf` builds your app; CloudPebble usually keeps this in sync with pushes.  
- Build generates **`build`** output and **resource headers** (e.g. `resource_ids.auto.h`).  
- **`#include <pebble.h>`** and generated IDs **`RESOURCE_ID_*`** must match manifest names.  
- **`APP_LOG`** enabled via `pebble_app_logging` / debug build where supported—use for development, strip noisy logs for release if desired.

---

## 8. C project anatomy: init, window, main loop

Typical pattern:

1. **`static Window *s_main_window;`** (+ layers as globals or heap).  
2. **`main_window_load` / `main_window_unload`** to create/destroy **child layers** tied to the window’s **root layer**.  
3. **`window_set_window_handlers`** with `.load = main_window_load`, `.unload = main_window_unload`.  
4. **`window_stack_push`** the window (apps); watchfaces often use a single full-screen window similarly.  
5. **`app_event_loop()`** at end of `main`.

**Deinit:** unsubscribe services (`tick_timer_service_unsubscribe`, `accel_data_service_unsubscribe`, etc.) in unload or before exit to avoid callbacks after teardown.

---

## 9. Graphics, layers, and drawing

### 9.1 Layer model

- **`Layer`** has **bounds**, **update_proc**, optional **children**.  
- **`layer_add_child(parent, child)`** builds a tree under the window root.  
- **`layer_mark_dirty(layer)`** schedules **`update_proc`** to run before next composite.

### 9.2 Drawing in `update_proc`

- Use **`GContext *ctx`** from the callback.  
- Set **fill** / **stroke** colors on color platforms (`graphics_context_set_fill_color`, etc.).  
- Draw primitives: **`graphics_fill_rect`**, **`graphics_draw_rect`**, **`graphics_draw_line`**, **`graphics_draw_circle`**, **`graphics_fill_circle`**, **`graphics_draw_round_rect`**, etc.  
- **Text** is often delegated to **`TextLayer`** / **`graphics_draw_text`** (see §11).

### 9.3 Clipping and transparency

- Use **`graphics_context_set_clip_rect`** (and variants) to limit drawing.  
- **BitmapLayer** / compositing: mind **alpha** and platform support.

---

## 10. Time, ticks, and scheduling

- **`tick_timer_service_subscribe(SECOND_UNIT | MINUTE_UNIT | ... , tick_handler)`** for periodic updates.  
- In handler, **update state** (e.g. `static` time struct) and **`layer_mark_dirty`** on affected layers.  
- Use **`time_*`** APIs (`time_now`, `localtime`, `strftime`) for clock strings.  
- For **non-tick** work, **`AppTimer`** can fire once or repeat (use sparingly for battery).

---

## 11. Text and fonts

- **`TextLayer`**: set **frame**, **text**, **font**, **colors**, **alignment** (`GTextAlignmentLeft`, etc.), **overflow mode** (`GTextOverflowModeTrailingEllipsis`, …).  
- **`graphics_draw_text`** for custom drawing paths with **`GFont`**.  
- **System fonts:** `fonts_get_system_font(FONT_KEY_*)`.  
- **Custom fonts:** from resources (§6.4).

---

## 12. Bitmaps and palettes

- Create from resource: **`gbitmap_create_with_resource(RESOURCE_ID_*)`**.  
- **`BitmapLayer`** displays a **`GBitmap`**; position with **`layer_set_frame`**.  
- For **rotated** or custom drawing, **`RotBitmapLayer`** or manual draw in **`LayerUpdateProc`**.  
- **`gbitmap_destroy`** when the bitmap is no longer needed.

---

## 13. Animation

- **`PropertyAnimation`** (and related) animates properties (e.g. **`layer` frame** via **`GRect`**) with **duration** and **curve** (`AnimationCurveEaseInOut`, …).  
- Set **`animation_set_handlers`** for **started/stopped** callbacks.  
- **`animation_schedule`** to run; **`animation_unschedule`** to cancel.

---

## 14. Input: buttons and click recognizers

### 14.1 Apps

- Implement **`click_config_provider`**; register with **`window_set_click_config_provider(window, click_config_provider)`**.  
- Inside provider: **`window_single_click_subscribe`**, **`window_multi_click_subscribe`**, **`window_long_click_subscribe`**, **`window_raw_click_subscribe`** as needed.  
- **BACK** often pops the window stack or exits the app—match platform UX expectations.

### 14.2 Watchfaces

- Button availability varies; **test on device**. Some faces use clicks for **config** launch or subtle interactions—always verify on target hardware.

---

## 15. Windows, navigation, and common UI patterns

- **`window_stack_push` / `window_stack_pop` / `window_stack_remove`**.  
- **`MenuLayer`** for scrollable lists; **`SimpleMenuLayer`** for quick static menus.  
- **`ActionBarLayer`** for UP/DOWN icon strip patterns.  
- **`ScrollLayer`** for long vertical content.  
- **`StatusBarLayer`** for system status strip when appropriate.

---

## 16. Vibration, backlight, battery-conscious design

- **`vibes_*`** for short/long/double pulses—use **judiciously**.  
- **`light_enable`** for temporary backlight.  
- Reduce **per-second** redraws unless needed; prefer **minute** ticks for static watchfaces.  
- Avoid **busy loops**; keep work in callbacks **short**.

---

## 17. Persistence (`persist_*`)

- **`persist_write_int` / `persist_read_int`**, **`persist_write_string`**, **`persist_write_data`**, etc., with **defined keys** (avoid collisions).  
- **`persist_exists`** before read when migrating versions.  
- **Size limits** apply—store small configuration, not large blobs (use resources or phone-side storage for big data).

---

## 18. AppMessage, AppSync, and PebbleKit JS

### 18.1 Concept

- **AppMessage** sends **small dictionaries** (tuples) between **watch C** and **phone JavaScript** (PebbleKit JS).  
- **`messageKeys`** in manifest must **match** indices/names used in C (`Tuplet`, `Dict_*`) and JS.

### 18.2 Patterns

- Inbound: register **`app_message_register_inbox_received`**, call **`app_message_open`** with **inbox/outbox sizes** (bytes) at least as large as your largest `Dict`/`Tuplet` payload.  
- Outbound: **`dict_merge`**, **`app_message_outbox_send`**.  
- **AppSync** (legacy pattern) still appears in older samples—prefer current guide patterns from Rebble docs for new code.

### 18.3 JS runtime

- JS runs in the **phone app** context; iOS/Android lifecycle can **suspend** the app—design for **reconnect** and **timeouts**.

---

## 19. Capabilities: configurable, health, location

| Capability | Typical use |
|------------|-------------|
| **`configurable`** | Web **config page** / Clay / settings via phone |
| **`health`** | **`health_service_*`** step/heart APIs on supported hardware |
| **`location`** | Phone JS geolocation for weather, etc. |

**Mismatch** (capability off but code calls API, or inverse) yields **runtime failure** or review issues.

---

## 20. Background worker (overview)

- Optional **C worker** in **`worker_src/`** for small periodic tasks; **tight limits** on CPU/memory.  
- Communicate with foreground app via **AppMessage** / worker APIs per SDK.  
- Declare and implement only if you truly need background behavior—complexity and policy constraints apply.

---

## 21. Timeline pins (overview)

- For apps that publish **timeline pins**, manifest and server-side pieces extend beyond basic faces—see Rebble **Pebble Timeline** guide.  
- Distinct from **store listing screenshots**.

---

## 22. GitHub layout, import, and CloudPebble finder rules

### 22.1 Recommended tree (native)

```
repo-root/
  package.json
  wscript
  .gitignore          # build/, node_modules/
  src/
    *.c
    js/               # if enableMultiJS
  resources/
    images/
      menu_icon.png
  worker_src/         # optional
```

### 22.2 How CloudPebble finds the project

Logic: `ide/utils/project.py` → **`find_project_root_and_manifest()`**:

- **`package.json`** qualifies if JSON parses and has top-level **`"pebble"`**.  
- **`appinfo.json`** qualifies if valid JSON **and** there is at least one **`src/**/*.c` or `*.js`** under the same tree prefix.  
- Paths under **`/build/`** or **`/node_modules/`** are **ignored** during search.  
- **First valid manifest wins** along iteration order—**do not** place two manifests at the same root.

### 22.3 GitHub Pull/Push

- **Pull** replaces CloudPebble’s `SourceFile`/`ResourceFile` from the branch zip—**paths must exist** for all declared resources.  
- **Push** may regenerate **`wscript`** / manifest files; reconcile conflicts with **409** by pulling first.

### 22.4 Import checklist (short)

- [ ] Single manifest root; no duplicates.  
- [ ] Resource **`file`** paths exist under **`resources/`**.  
- [ ] One **`menuIcon`** if using standard launcher icon.  
- [ ] **UUID** stable; **version** valid for project type.  
- [ ] **Capabilities** match code.  
- [ ] Optional **`package-lock.json`** for reproducible npm deps.

---

## 23. Troubleshooting

| Symptom | Likely cause |
|---------|----------------|
| “No valid Pebble project root” | Missing/invalid manifest; manifest under `build/` or `node_modules/` |
| Duplicate `RESOURCE_ID_*` | Duplicate media in manifest + CloudPebble UI, or duplicate names |
| Build fails resource phase | PNG path wrong, wrong `type`, aplite palette issue |
| AppMessage silent failure | `messageKeys` mismatch; inbox/outbox sizes too small |
| White screen on round | Drawing outside visible circle; check clipping |
| Import 404 | Wrong branch, private repo auth, renamed default branch |
| Emulator missing on CloudPebble | Host without QEMU profile |

---

## 24. Checklists: new watchface, new app, pre-release

### 24.1 New watchface

- [ ] `watchface: true`  
- [ ] Minute vs second tick chosen deliberately  
- [ ] Layout tested on **smallest** and **largest** target in your `targetPlatforms`  
- [ ] Round platform: circular clip / centering  
- [ ] Aplite: palette-compliant assets  
- [ ] Optional: config flow if user-selectable options  

### 24.2 New app

- [ ] `watchface: false`  
- [ ] Window stack and **BACK** behavior defined  
- [ ] Click config on each interactive window  
- [ ] Menu icon 25×25 (if shown in launcher)  
- [ ] Multi-window memory: unload destroys child layers  

### 24.3 Pre-release (both)

- [ ] UUID/version policy set  
- [ ] Logs trimmed or gated  
- [ ] `pbw` tested on **real watch** for each enabled platform  
- [ ] Store text + screenshots prepared (outside or alongside manifest)

---

## 25. Upstream CloudPebble source map

| Topic | Path under [coredevices/cloudpebble](https://github.com/coredevices/cloudpebble) |
|-------|-------------------------------------------------------------------------------------|
| Manifest discovery | `cloudpebble/ide/utils/project.py` |
| Manifest mapping | `cloudpebble/ide/utils/sdk/manifest.py` |
| Zip / import | `cloudpebble/ide/tasks/archive.py` |
| GitHub | `cloudpebble/ide/tasks/git.py` |
| Build worker | `cloudpebble/ide/tasks/build.py` |
| Project model | `cloudpebble/ide/models/project.py` |
| Compile UI (install, logs) | `cloudpebble/ide/static/ide/js/compile.js` |

---

## Appendix A. Preprocessor symbols, memory, teardown

### A.1 Common compile-time tests

Use **`#ifdef`** / **`#if defined(...)`** with SDK-provided symbols rather than inventing your own where possible:

| Symbol | Use |
|--------|-----|
| `PBL_BW` | Monochrome draw paths (aplite, diorite, …) |
| `PBL_COLOR` | Color APIs (`GColor*`) |
| `PBL_ROUND` | Chalk round layout (corners invisible) |
| `PBL_RECT` | Rectangular color/B&W |
| `PBL_PLATFORM_APLITE` / `BASALT` / `CHALK` / `DIORITE` / `EMERY` | Per-platform tweaks |

Combine with **`#ifndef NDEBUG`** or app logging flags for debug-only UI.

### A.2 Memory discipline

- **Global/static** `Window`, `Layer`, `TextLayer`, `BitmapLayer` pointers: create in **`window_load`**, **`destroy`** in **`window_unload`** (or symmetric helper).  
- **`malloc`**: every `free` on all paths (including early return); watchfaces rarely need heap—prefer static buffers with max sizes.  
- **Bitmaps**: `gbitmap_destroy` when swapping images (weather icons, themes).  
- **Fonts**: unload custom fonts when the screen that owns them is torn down.  
- **Animations**: unschedule on unload to avoid callbacks into destroyed layers.

### A.3 Teardown order (rule of thumb)

1. Unsubscribe **services** (tick, accel, health, …).  
2. **Cancel** timers and animations.  
3. Destroy **leaf layers** → remove from parent → destroy **window**.  
4. Release **GBitmap** / fonts last used by those layers.

---

## Appendix B. Pebble.js, Rocky, packages

| Stack | When to use | Tradeoff |
|-------|-------------|----------|
| **Native C** | Maximum control, smallest predictable binary, watchfaces, tight graphics | Steeper learning curve |
| **Pebble.js** | Rapid UI with JS on watch (legacy ecosystem) | Different model; check current Rebble support/docs |
| **Rocky** | JS-oriented graphics path where supported | See **Alloy** / Rocky guides on Rebble |
| **Pebble Package** | Shared deps, npm, modular apps | Stricter semver, different resource roots (`src/resources/`) |

CloudPebble project type must **match** the repo; importing a **native** layout into a **package** template (or vice versa) causes confusing build errors.

---

## Appendix C. Sensors & system services

Brief map (see Rebble **Events and Services** guide for signatures):

| API family | Typical use |
|------------|-------------|
| **Accelerometer** | Shake, step-like motion, simple games |
| **Compass** | Heading-aware UI |
| **Health** | Steps, sleep (capability `health`) |
| **Battery** | `battery_state_service_subscribe` for low-battery UI |
| **Bluetooth** | Connection lost icon / reconnect messaging |

Always **subscribe in init**, **unsubscribe in deinit**, and assume **callbacks on system thread**.

---

## Appendix D. i18n & locale

- Use **`i18n_get_locale()`** (and related APIs per SDK) to pick strings/resources.  
- Keep **English fallback** in code; optional **extra resource sets** or string tables per locale depending on your approach.  
- **Text width** differs by language—use **`GTextOverflowMode`** and dynamic layout, not fixed pixel widths from English only.

---

## Appendix E. Anti-patterns & sharp edges

1. **Second tick on battery-critical face** — redrawing full screen every second without need.  
2. **Heavy work in `tick_handler`** — parse JSON, allocate aggressively; move work to AppTimer batching or phone.  
3. **Ignoring `PBL_ROUND`** — text clipped or unreadable in corners.  
4. **AppMessage keys reordered** between releases — breaks compatibility with installed phone JS.  
5. **Persist schema changes** without migration — guard with version key in persist.  
6. **Click config on wrong window** — provider must match the window receiving events.  
7. **Duplicate resources** in CloudPebble UI + GitHub manifest on linked repos.  
8. **Three-part version** on native projects in CloudPebble — model rejects `1.0.0`-style labels for non-package types.  
9. **Assuming emulator ≡ device** — test **Bluetooth**, **performance**, and **button** behavior on hardware.

---

*This document is descriptive and combines CloudPebble repository behavior with general Pebble SDK 3 / Rebble practice. UI labels and hosted features may differ slightly in production. For API details and evolving patterns, prefer the [Rebble Developer Guides](https://developer.repebble.com/guides/). For CloudPebble bugs/features, use the [CloudPebble GitHub](https://github.com/coredevices/cloudpebble) and the [Rebble community](https://rebble.io/).*
