# Trio local Pebble API on iOS

The HTTP server listens on `127.0.0.1` inside the Trio process. **Rebble** is a separate app: when it is in the foreground, Trio is usually backgrounded and iOS may suspend it, so `http://127.0.0.1:8080` can fail intermittently.

Trio now starts a **short background task** (~25s) each time the Pebble server accepts a connection, which improves (but does not guarantee) reliability when switching between Rebble and Trio.

**Apple Health:** PebbleKit JS cannot read HealthKit. If CGM data flows into **Apple Health** and Trio uses that as its glucose source, choose **“Apple Health & CGM via Trio”** in watchface settings — it uses the same local Trio HTTP API as “Trio (local HTTP)”; only the on-phone label/help text differs.

For the most stable CGM feed to the watch without Trio staying active, use **Nightscout** or **Dexcom Share (direct)** in the watchface settings.
