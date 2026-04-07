#include "demo_preview.h"
#include "trend_normalize.h"
#include <string.h>
#include <time.h>

void trio_demo_preview_apply(AppState *state) {
#if !TRIO_ENABLE_DEMO_GRAPH
    (void)state;
#else
    if (!state) {
        return;
    }

    const int n = 48;
    int base = 118;
    for (int i = 0; i < n; i++) {
        int wobble = (i * 13) % 31 - 15;
        int v = base + wobble + (i * 18 / n);
        if (v < 95) {
            v = 95;
        }
        if (v > 168) {
            v = 168;
        }
        state->graph.values[i] = (int16_t)v;
    }
    state->graph.count = n;
    state->graph.prediction_count = 0;

    state->cgm.glucose = 118;
    /* UTF-8 RIGHTWARDS ARROW U+2192 (flat trend) */
    state->cgm.trend_str[0] = (char)0xe2;
    state->cgm.trend_str[1] = (char)0x86;
    state->cgm.trend_str[2] = (char)0x92;
    state->cgm.trend_str[3] = '\0';
    trio_normalize_trend_str(state->cgm.trend_str, sizeof(state->cgm.trend_str));

    strncpy(state->cgm.delta_str, "+2", sizeof(state->cgm.delta_str) - 1);
    state->cgm.delta_str[sizeof(state->cgm.delta_str) - 1] = '\0';
    state->cgm.is_stale = false;
    state->cgm.last_reading_time = time(NULL);

    strncpy(state->loop.iob, "1.8U", sizeof(state->loop.iob) - 1);
    strncpy(state->loop.cob, "12g", sizeof(state->loop.cob) - 1);
    strncpy(state->loop.last_loop_time, "Loop 2m", sizeof(state->loop.last_loop_time) - 1);
    strncpy(state->loop.loop_status, "Enacted", sizeof(state->loop.loop_status) - 1);
    strncpy(state->loop.pump_status, "Pod", sizeof(state->loop.pump_status) - 1);
    state->loop.reservoir = 72;
    state->loop.pump_battery = 88;
    strncpy(state->loop.sensor_age, "4d", sizeof(state->loop.sensor_age) - 1);

    state->comp.phone_battery = 85;
    state->comp.weather_temp = 72;
    strncpy(state->comp.weather_icon, "partly", sizeof(state->comp.weather_icon) - 1);
    state->comp.steps = 8420;
    state->comp.heart_rate = 72;
    state->comp.watch_battery = 80;
    state->comp.watch_charging = false;
#endif
}
