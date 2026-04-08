# Capturing watchface logs from Trio (for debugging)

Trio uses Pebble **`APP_LOG(...)`** in C (e.g. `main.c`, `remote_cmds.c`, `trend_glyphs.c`). To capture lines for analysis:

## CloudPebble (browser)

1. Build and install to a **phone** or **emulator** with the dev bridge active.  
2. Open the **Compilation** pane.  
3. Use **Show Phone Logs** (or **Show Emulator Logs** if using QEMU).  
4. Reproduce the issue (e.g. **long-press UP/DOWN** for the remote menu).  
5. Copy the log text (filter for `remote:` or `trend:` if you added recent builds).

Logs only appear when the app is running and the bridge is connected.

## Local Pebble tool + hardware

If you use the **Pebble CLI** with a physical watch:

```bash
pebble install --phone <ip>
pebble logs --phone <ip>
```

Or enable logging in the phone’s developer / debug UI for the connected app (varies by Rebble app version).

## What to send

- A **short window** around the action (e.g. 10–30 seconds).  
- Note **watch model** (e.g. Pebble Time 2), **Rebble app** platform (iOS/Android), and **data source** (Trio vs Dexcom vs Nightscout).  
- For **remote menu**: confirm data source is **Trio** (or Apple Health via Trio); other sources intentionally double-vibe and do not open the menu.

## Build with logging enabled

`APP_LOG` is included in normal debug-oriented builds; if you ever strip logs, revert that for diagnosis.
