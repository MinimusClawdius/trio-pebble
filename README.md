# Trio Pebble v2.2

A premium, configurable CGM watchface for Pebble smartwatches. Supports **Trio**, **Dexcom Share**, and **Nightscout** data sources.

## Features

### Multiple Watchface Layouts
Switch between **six** face designs via buttons or the config page:

| Face | Description |
|------|-------------|
| **Classic** | Glucose + graph + IOB/COB/loop + complications bar |
| **Graph Focus** | Full-screen graph with overlaid glucose and minimal text |
| **Compact** | T1000-inspired: large glucose + trend, reading age, full graph |
| **Dashboard** | Information-dense quadrant layout with date, pump, sensor data |
| **Minimal** | Elegant time-forward design with sparkline - for everyday wear |
| **Retro** | LCD-style frame (inspired by [91 Dub](https://apps.repebble.com/91-dub-v2-0_554e08d40136b0e6e200001b) layout); Bitham time + bordered readout |

The bottom bar has **four configurable slots** (phone settings page → Complications). Defaults match the old layout: watch battery, steps, heart rate, weather. Set any slot to **Empty** to skip it.

### Data Sources (Configurable)
- **Trio** - Polls local HTTP API on `127.0.0.1:8080` (requires Trio iOS app with Pebble integration enabled)
- **Dexcom Share** - Direct connection to Dexcom Share servers (US and international)
- **Nightscout** - Connects to any Nightscout instance with optional API token auth

### Dynamic Color-Coded Graph
- Green line segments when glucose is in range
- Orange when above high threshold
- Red when below low threshold or above high+60
- Target range band with dashed threshold lines
- Urgent low line in red
- CGM trace and optional prediction line without extra grid clutter (readable on small screens)
- Black outline on the glucose trace when a weather sky background is active
- Prediction line overlay (when available from Trio/Nightscout)
- Configurable thresholds via settings page

### Weather sky background (behind the graph)
When **weather** is enabled in settings, the graph area draws a simple sky scene from Open-Meteo’s WMO code: **clear** (sun by day, moon + stars at night), **mainly clear / partly cloudy / overcast**, **fog**, **rain**, **snow**, **storm** (rain + lightning). Day vs night follows the **watch’s local time** (~6:00–20:59 day). Temperature still appears in the complications bar.

### Configurable Alerts
- **High glucose alert** - vibration when above threshold
- **Low glucose alert** - distinct vibration pattern when below threshold
- **Urgent low alert** - aggressive vibration, always active regardless of settings
- **Snooze** - press SELECT to snooze all alerts (configurable duration: 5-60 min)
- Minimum 60-second re-alert interval to prevent vibration spam

### Complications (four slots, left → right)
Configure each slot on the settings page: **Empty**, **Watch battery**, **Phone battery** (from the phone when provided), **Steps**, **Heart rate** (Pebble Health), **Weather** (needs the weather fetch toggle on). Slots are equal width; **Weather** shows `off` if fetching is disabled.

### Color Schemes
- **Dark** - Black background, green/red/orange glucose colors
- **Light** - White background, adapted colors for readability
- **High Contrast** - Maximum visibility for outdoor use

### Touch/Tap Framework
Built-in framework for future touch screen support. When Pebble touch capabilities become available:
- Tap graph area to open carbs entry
- Tap data area to request bolus
- Tap and hold for temp basal adjustment
- Currently uses accelerometer tap for data refresh

## Quick Start

### Option A: CloudPebble (Recommended)
1. Go to [CloudPebble](https://cloudpebble.net/)
2. Import from GitHub: `https://github.com/MinimusClawdius/trio-pebble`
3. Build and install to your Pebble

### Option B: Local Build
```bash
pebble build
pebble install --phone <your-phone-ip>
```

### Configuration
1. Open the Pebble/Rebble app on your phone
2. Find "Trio Pebble" in your watchfaces
3. Tap **Settings** to open the configuration page (hosted URL; see below if you see **404**)
4. Select your data source, thresholds, alerts, and face type

**If Settings shows raw HTML or 404:** CDNs like jsDelivr serve `.html` as `text/plain`, so Rebble shows source instead of a UI. Enable **GitHub Pages** on this repo and use the default `TRIO_CONFIG_PAGE_URL` (see [docs/CLOUDPEBBLE_AND_DEPLOY.md](docs/CLOUDPEBBLE_AND_DEPLOY.md)). Forks should point the constant at `https://<you>.github.io/<repo>/config/index.html`.

### CloudPebble: capability checkboxes
In CloudPebble project **Settings**, enable **Configurable**, **Uses health**, and **Uses location** so they match this project (config page, `health_service_*` on the watch, `navigator.geolocation` for weather). Details: [docs/CLOUDPEBBLE_AND_DEPLOY.md](docs/CLOUDPEBBLE_AND_DEPLOY.md).

## Architecture

```
Pebble Watch (C)
├── main.c              - App lifecycle, face management, message routing
├── trio_types.h        - Shared types, enums, state structures
├── modules/
│   ├── config.c/.h     - Persistent configuration (persist API)
│   ├── graph.c/.h      - Color-coded graph with thresholds & predictions
│   ├── alerts.c/.h     - BG alerts with vibration patterns & snooze
│   ├── complications.c/.h - Battery, weather, steps, heart rate
│   └── tap_framework.c/.h - Future touch action zone system
├── faces/
│   ├── face_classic.c/.h
│   ├── face_graph_focus.c/.h
│   ├── face_compact.c/.h
│   ├── face_dashboard.c/.h
│   └── face_minimal.c/.h
└── pkjs/
    └── index.js        - Multi-source data fetching, weather, config bridge

Phone (HTML)
└── config/
    └── index.html      - Settings page (dark theme, mobile-optimized)
```

### Data Flow
```
[CGM Sensor] → [Trio App / Dexcom / Nightscout]
                           ↓
                  PebbleKit JS (index.js)
                  - Fetches from selected source
                  - Normalizes to common format
                  - Fetches weather from Open-Meteo
                           ↓
                     AppMessage
                           ↓
                  Pebble Watch (main.c)
                  - Routes to active face
                  - Updates graph, text, complications
                  - Checks alert thresholds
```

## Button Controls

| Button | Watchface Mode | Alert Active |
|--------|---------------|--------------|
| **UP** | Previous face layout | Previous face layout |
| **UP (hold ~½s)** | Remote bolus / carbs menu (Trio source) | Same |
| **DOWN** | Next face layout | Next face layout |
| **DOWN (hold ~½s)** | Remote bolus / carbs menu (Trio source) | Same |
| **SELECT (hold ~½s)** | Same menu *if* system Quick Launch does not steal Select (see below) | Same |
| **SELECT (short)** | — | Snooze alerts |
| **BACK** | Exit watchface | Exit watchface |

**Quick Launch (Pebble Time 2, etc.):** A long press on **Select** often triggers Pebble’s **Quick Launch** instead of this watchface. Use **long Up** or **long Down** for the remote menu, or disable/move Quick Launch for the middle button under **Settings → Quick Launch** on the watch. If the menu never appears with Trio as the data source, update to the latest build (older code required the top window pointer to match exactly, which failed on some firmware paths).

## Safety

- All bolus/carb commands (Trio source only) require iPhone confirmation
- Commands are not available when using Dexcom Share or Nightscout sources
- Urgent low alerts cannot be disabled
- This watchface is not FDA approved and should not be used for medical decisions

## Documentation

| Document | Purpose |
|----------|---------|
| [docs/CLOUDPEBBLE_COMPREHENSIVE_GUIDE.md](docs/CLOUDPEBBLE_COMPREHENSIVE_GUIDE.md) | **CloudPebble + Pebble SDK** reference (IDE, manifest, watchface vs app, graphics, AppMessage, checklists)—good for AI context while building. |
| [docs/CLOUDPEBBLE_AND_DEPLOY.md](docs/CLOUDPEBBLE_AND_DEPLOY.md) | **Trio-specific** CloudPebble import, menu icon, capabilities, phone deploy. |
| [docs/WATCH_APP_LOGS.md](docs/WATCH_APP_LOGS.md) | How to capture **watchface logs** (`APP_LOG`) for debugging. |
| [docs/pebble-legacy-manifest-reference.md](docs/pebble-legacy-manifest-reference.md) | Human reference for legacy manifest fields (not authoritative for this repo’s `package.json`). |

**Learn C on Pebble:** [Learning C with Pebble](https://pebble.gitbooks.io/learning-c-with-pebble/) (GitBook)—chapters and CloudPebble-oriented project exercises from variables through UI, graphics, and messaging.

**Trend arrow PNGs** are adapted from the [t1000-pebble-cgm](https://github.com/andrewchilds/t1000-pebble-cgm) project (`resources/images/trend_*.png`); see [docs/T1000_TREND_ASSETS.md](docs/T1000_TREND_ASSETS.md).

## Requirements

- Pebble / Pebble 2 (Aplite)
- Pebble Time / Time Steel (Basalt) - color support
- Pebble Time Round (Chalk)
- Pebble 2 HR (Diorite)
- Pebble Time 2 (Emery)
- Rebble app on iOS or Android

## License

MIT
