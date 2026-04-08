# Capturing watchface logs (Trio / remote menu debugging)

Trio logs with **`APP_LOG(...)`** from native C (see `remote_cmds.c`, `main.c`, `trend_glyphs.c`). Filter for **`remote:`** when diagnosing the **UP/DOWN long-press menu**.

## CloudPebble (easiest)

1. Build and run on **emulator** or **phone** with the dev connection active.  
2. **Compilation** (or project) pane → **Show Phone Logs** / **Show Emulator Logs**.  
3. Reproduce the issue (e.g. long-press **UP** or **DOWN** for the remote menu).  
4. Copy a **20–40 s** slice of the log.

## Rebble app on Android

Logs usually appear in **logcat** when the phone is connected via USB with **USB debugging** enabled.

1. Install [adb](https://developer.android.com/studio/command-line/adb) or use Android Studio **Logcat**.  
2. Connect the phone, run:  
   `adb logcat | findstr /i "pebble rebble trio"`  
   (PowerShell: `adb logcat | Select-String -Pattern "pebble|rebble|trio"`.)  
3. Open the watchface, trigger the menu, then save the matching lines.

Exact process name varies by Rebble build; widening the filter to **`pebble`** helps.

## Rebble app on iOS

There is no logcat. Options:

1. **Xcode → Window → Devices and Simulators** → select the iPhone → **Open Console** while the Rebble app and watchface run.  
2. Or **macOS Console.app** (search for **Pebble** / **Rebble** / the app name).

Apple restricts low-level logs; if nothing appears, rely on **CloudPebble** logs when the project is sideloaded from there, or temporarily add more **`APP_LOG`** lines and rebuild.

## What the remote menu logs mean

From **`remote_cmds_try_open()`** (`remote_cmds.c`):

| Message | Meaning |
|--------|---------|
| `remote: s_watchface NULL` | Window pointer not set (init order bug). |
| `remote: already in menu/picker` | Menu or amount picker is already on top. |
| `remote: top=… != watch=…` | Another window is above the watchface (system UI, notification, etc.); menu is **not** pushed so we do not corrupt the stack. |
| `remote: wrong data_source=… (need Trio)` | Menu only works for **Trio** or **Apple Health via Trio**; other sources double-vibe by design. |
| `remote: pushing menu` | Menu should open. |

If you see **`top != watch`** while the face looks visible, note **watch model** and **firmware** — some stacks report a different top window than expected.

## Build with logging

`APP_LOG` is enabled in normal release builds. Do not strip logs while diagnosing.

## References

- Rebble C tutorial — **battery / Bluetooth** (patterns similar to our footer battery bar): [Part 3 — Battery Meter and Bluetooth](https://developer.repebble.com/tutorials/watchface-tutorial/part3/).  
- **Phone ↔ watch** flow and AppMessage: [Part 4 — Adding Weather](https://developer.repebble.com/tutorials/watchface-tutorial/part4/).
