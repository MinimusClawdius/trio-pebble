#pragma once
#include "../trio_types.h"

/**
 * Persist the mutable parts of AppState (CGM, loop, graph, complications)
 * to the Pebble watch's flash storage so data survives:
 *   - watchface restarts / face switches
 *   - BLE disconnections
 *   - phone backgrounding
 *
 * Config is persisted separately by config.c.
 * Only call state_persist_save() after receiving fresh data — not on every tick.
 */

void state_persist_save(const AppState *state);

/**
 * Restore persisted state into the provided AppState struct.
 * Returns true if valid data was loaded, false if no persisted state exists
 * or the version/size doesn't match (stale migration).
 */
bool state_persist_load(AppState *state);
