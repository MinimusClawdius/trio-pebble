# Trio Pebble Watchface

A Pebble smartwatch companion watchface for the [Trio](https://github.com/MinimusClawdius/Trio) diabetes management iOS app.

## Features

- **Real-time glucose display** with large, readable numbers
- **Glucose trend graph** with color-coded ranges (low=red, in-range=green, high=orange)
- **Trend arrow** showing glucose direction
- **Delta** showing rate of change
- **IOB** (Insulin on Board) display
- **COB** (Carbs on Board) display
- **Loop status** with last loop time
- **Bolus commands** with iPhone confirmation safety
- **Carb entry** with iPhone confirmation safety
- Compatible with all Pebble models (Aplite, Basalt, Chalk, Diorite, Emery)

## Architecture

```
Pebble Watch <-- AppMessage --> Rebble App (PebbleKit JS)
                                     |
                                HTTP (127.0.0.1:8080)
                                     |
                                Trio iOS App (PebbleLocalAPIServer)
```

The watchface communicates with Trio via:
1. **PebbleKit JS** bridge running inside the Rebble companion app
2. JS bridge polls a **local HTTP server** running inside the Trio iOS app
3. Data is formatted and sent to the watch via **AppMessage**

## Building with CloudPebble

1. Go to [CloudPebble](https://cloudpebble.net/) (Rebble-maintained)
2. Click "Import" and select "Import from GitHub"
3. Enter this repository URL: `https://github.com/MinimusClawdius/trio-pebble`
4. CloudPebble will import all source files
5. Click "Compilation" > "Run Build"
6. Install the resulting `.pbw` to your Pebble via the Rebble app

## Building Locally

Requires the [Pebble SDK](https://developer.rebble.io/developer.pebble.com/sdk/install/index.html).

```bash
pebble build
```

The output `.pbw` file will be in the `build/` directory.

## Setup in Trio

1. Open Trio on your iPhone
2. Go to **Settings** > **Watch** > **Pebble**
3. Toggle **Enable Pebble** on
4. Verify the status shows "Running"
5. Install this watchface on your Pebble
6. The watchface will automatically connect and display data

## Safety

- All bolus and carb commands from the Pebble **require confirmation on the iPhone**
- Commands expire after 5 minutes if not confirmed
- Maximum bolus and carb amounts are enforced by Trio's safety limits

## API Endpoints

The local HTTP server exposes these endpoints:

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/cgm` | GET | Current glucose, trend, delta |
| `/api/loop` | GET | IOB, COB, loop status, glucose history |
| `/api/pump` | GET | Pump status |
| `/api/all` | GET | All data combined |
| `/api/bolus` | POST | Request bolus (requires iPhone confirm) |
| `/api/carbs` | POST | Request carb entry (requires iPhone confirm) |
| `/health` | GET | Server health check |

## File Structure

```
src/
  main.c              - Main watchapp: UI, menus, AppMessage handling
  glucose_graph.c     - Glucose trend graph rendering
  glucose_graph.h     - Graph header
  js/
    pebble-js-app.js  - PebbleKit JS bridge (polls Trio HTTP server)
appinfo.json          - Pebble project metadata
package.json          - NPM metadata with AppMessage key definitions
wscript               - Pebble build script
```

## License

MIT
