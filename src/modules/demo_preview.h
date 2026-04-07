#pragma once
#include "../trio_types.h"

/**
 * Store / emulator screenshots: set to 1 to seed plausible CGM + graph + complications
 * without a phone. MUST be 0 for real-world PBW builds (users expect live data only).
 *
 * Workflow: set 1 → build in CloudPebble or `pebble build` → run emulator →
 * capture each face (UP/DOWN) → upload PNGs to Rebble App Store listing → set back to 0.
 */
#define TRIO_ENABLE_DEMO_GRAPH 0

void trio_demo_preview_apply(AppState *state);
