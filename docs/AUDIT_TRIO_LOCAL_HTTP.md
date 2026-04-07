# Audit: trio-pebble ↔ Trio `127.0.0.1` local HTTP

This document describes how data flows from Trio’s loopback API to the watch, common failure modes, and fixes (v2.2.5+).

## 1. HTTP surface (Trio iOS)

- **`GET /api/all`** — Combined JSON: `cgm`, `loop`, `pump`, `maxBolus`, `maxCarbs`, `blePushActive`.
- **CGM** — `glucose` is a **string** in the user’s units (`"4.7"` mmol/L or `"85"` mg/dL). `units` is e.g. `"mmol/L"` or `"mg/dL"`.
- **Loop** — `glucoseHistory` is a numeric array in the **same** units as `cgm` (mmol or mg/dL).
- **Pump** — `pump.reservoir` / `pump.battery` may be JSON `null` when unknown; pkjs must not treat null as `0` for display keys unless we intentionally clear (current code omits keys when 0).

## 2. PebbleKit JS (`src/pkjs/index.js`)

### 2.1 Normalization (`normalizeTrio`)

- **Wire contract:** `KEY_GLUCOSE` and graph points are always **mg/dL integers** (watch C + alerts + graph scale 40–400).
- **Conversion:** If `cgm.units` contains `mmol` **or** values look like mmol (e.g. current BG &lt; 35 with plausible mmol history), multiply by **18** and round.
- **Pump:** Reads `data.pump.reservoir` / `data.pump.battery` when present; passes through to `sendToWatch` (keys omitted when 0 so prior watch values are not overwritten with junk).

### 2.2 Display units (`KEY_UNITS`)

- Controlled by watchface settings (`glucoseUnits` / mmol vs mg/dL), **not** by copying Trio's `cgm.units` into every payload (that mixed “API units” with “display format”).

### 2.3 `sendToWatch`

- Sends integer `KEY_GLUCOSE`, packed uint16 LE graph, optional pump keys, `KEY_UNITS` for display formatting on C side.

### 2.4 Watch → phone messages (`appmessage`)

- **Must not** use **dictionary key `0`** for “poll” or handshake: key **0** is **`KEY_GLUCOSE`** in `package.json`. A `{0: 0}` ping collides with the CGM schema and has been a source of subtle bugs on some stacks.
- **Fix:** The watch **minute tick** sends **`KEY_TAP_ACTION` = `TAP_ACTION_REFRESH` (4)** instead of key 0.

## 3. Watch C (`src/main.c`)

### 3.1 `inbox_received`

- Applies `config_apply_message` then updates **`s_state.config = *config_get()`** so `alerts_check` uses current thresholds (not a stale copy from boot).

### 3.2 Face switch / `reload_face`

- **`window_load`** calls `graph_init()` which zeros the graph module’s buffers.
- **Fix:** After `graph_init()`, if `s_state.graph.count > 0`, call **`graph_restore_from_state(&s_state.graph)`** so changing faces (e.g. Compact / “T1000”) does not leave an empty graph while CGM text still shows stale numbers.

### 3.3 Persisted state

- **`PERSIST_STATE_VERSION`** was bumped when we invalidated blobs that could hold **bogus mg/dL** (e.g. `4`) from pre-conversion bugs.

## 4. Symptom → cause cheat sheet

| Symptom | Likely cause |
|--------|----------------|
| **0.2** mmol on display with real ~4.7 mmol | Internal mg/dL ≈ **4** (`format_glucose_display`: 4/18 ≈ 0.2). Often **mmol not ×18** on wire, or **persisted** bad value. |
| Vibrations every minute | **Urgent low** with bogus low mg/dL; fixed by correct conversion + alert cooldowns + clearing bad persist. |
| Wrong after switching face | **Graph buffer cleared** without restore from `s_state.graph`; fixed by `graph_restore_from_state` in `window_load`. |
| “OK at first then bad” | **Stale `s_state.config`** for alerts, or **key-0 tick** + race; fixed by config sync + TAP refresh ping. |

## 5. Verify after install

1. Build PBW from this repo (**v2.2.5+**).
2. Open Trio → confirm `/api/all` shows expected `cgm.glucose` / `units`.
3. On watch: main BG should match (within rounding) **either** mmol one-decimal **or** mg/dL integer per settings.
4. Change faces (Classic ↔ Compact); graph and BG should stay coherent.
5. Optional: Rebble logs / `console.log` in pkjs (if available) after `normalizeTrio` (temporary) to print `glucoseMgdl`.
