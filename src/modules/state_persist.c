#include "state_persist.h"
#include "trend_normalize.h"
#include <pebble.h>
#include <string.h>

/* ── Persist keys ────────────────────────────────────────────────
 * Using separate keys keeps each chunk under the 256-byte persist limit
 * and lets us version individual sections independently.
 * Key namespace: 0x50xx  ("P" prefix — Persist state data)
 */
#define PERSIST_KEY_CGM        0x5001
#define PERSIST_KEY_LOOP       0x5002
#define PERSIST_KEY_GRAPH      0x5003
#define PERSIST_KEY_COMP       0x5004
#define PERSIST_KEY_VERSION    0x5005

/* Bump when struct layout changes to invalidate stale blobs. */
/* v2: mmol/mg/dL CGM fix. v3: CGMState.trend_str widened + trend normalization.
 * v4: LoopState.trio_link for PebbleKit JS HTTP status. */
#define PERSIST_STATE_VERSION  4

/* ── Graph persistence ───────────────────────────────────────────
 * GraphData contains up to 48 × int16 values + 24 × int16 predictions +
 * two int counts.  Total worst-case:
 *   48*2 + 24*2 + 4 + 4 = 152 bytes — fits in one persist slot (256 max).
 */

void state_persist_save(const AppState *state) {
    persist_write_int(PERSIST_KEY_VERSION, PERSIST_STATE_VERSION);

    persist_write_data(PERSIST_KEY_CGM,  &state->cgm,  sizeof(CGMState));
    persist_write_data(PERSIST_KEY_LOOP, &state->loop, sizeof(LoopState));
    persist_write_data(PERSIST_KEY_COMP, &state->comp, sizeof(Complications));

    /* Graph: pack into a flat buffer so it fits reliably.
     * Layout: [count:int32][values:int16*48][pred_count:int32][predictions:int16*24]
     */
    uint8_t buf[sizeof(int32_t) + MAX_GRAPH_POINTS * sizeof(int16_t) +
                sizeof(int32_t) + MAX_PREDICTIONS * sizeof(int16_t)];
    int off = 0;

    int32_t cnt = (int32_t)state->graph.count;
    memcpy(buf + off, &cnt, sizeof(cnt)); off += sizeof(cnt);
    memcpy(buf + off, state->graph.values, MAX_GRAPH_POINTS * sizeof(int16_t));
    off += MAX_GRAPH_POINTS * sizeof(int16_t);

    int32_t pcnt = (int32_t)state->graph.prediction_count;
    memcpy(buf + off, &pcnt, sizeof(pcnt)); off += sizeof(pcnt);
    memcpy(buf + off, state->graph.predictions, MAX_PREDICTIONS * sizeof(int16_t));
    off += MAX_PREDICTIONS * sizeof(int16_t);

    persist_write_data(PERSIST_KEY_GRAPH, buf, off);
}

bool state_persist_load(AppState *state) {
    if (!persist_exists(PERSIST_KEY_VERSION)) return false;

    int version = persist_read_int(PERSIST_KEY_VERSION);
    if (version != PERSIST_STATE_VERSION) {
        /* Layout changed — discard stale data. */
        persist_delete(PERSIST_KEY_CGM);
        persist_delete(PERSIST_KEY_LOOP);
        persist_delete(PERSIST_KEY_GRAPH);
        persist_delete(PERSIST_KEY_COMP);
        persist_delete(PERSIST_KEY_VERSION);
        return false;
    }

    if (persist_exists(PERSIST_KEY_CGM)) {
        persist_read_data(PERSIST_KEY_CGM, &state->cgm, sizeof(CGMState));
        trio_normalize_trend_str(state->cgm.trend_str, sizeof(state->cgm.trend_str));
    }
    if (persist_exists(PERSIST_KEY_LOOP)) {
        persist_read_data(PERSIST_KEY_LOOP, &state->loop, sizeof(LoopState));
    }
    if (persist_exists(PERSIST_KEY_COMP)) {
        persist_read_data(PERSIST_KEY_COMP, &state->comp, sizeof(Complications));
    }

    if (persist_exists(PERSIST_KEY_GRAPH)) {
        uint8_t buf[sizeof(int32_t) + MAX_GRAPH_POINTS * sizeof(int16_t) +
                    sizeof(int32_t) + MAX_PREDICTIONS * sizeof(int16_t)];
        int read = persist_read_data(PERSIST_KEY_GRAPH, buf, sizeof(buf));
        if (read > 0) {
            int off = 0;
            int32_t cnt;
            memcpy(&cnt, buf + off, sizeof(cnt)); off += sizeof(cnt);
            state->graph.count = (cnt > MAX_GRAPH_POINTS) ? MAX_GRAPH_POINTS : cnt;
            memcpy(state->graph.values, buf + off, MAX_GRAPH_POINTS * sizeof(int16_t));
            off += MAX_GRAPH_POINTS * sizeof(int16_t);

            int32_t pcnt;
            memcpy(&pcnt, buf + off, sizeof(pcnt)); off += sizeof(pcnt);
            state->graph.prediction_count = (pcnt > MAX_PREDICTIONS) ? MAX_PREDICTIONS : pcnt;
            memcpy(state->graph.predictions, buf + off, MAX_PREDICTIONS * sizeof(int16_t));
        }
    }

    /* Mark data as stale if the reading is older than 15 minutes.
     * We still display it — better than an empty screen — but the
     * face can gray-out or show a stale indicator.
     */
    time_t now = time(NULL);
    if (state->cgm.last_reading_time > 0 &&
        (now - state->cgm.last_reading_time) > 15 * 60) {
        state->cgm.is_stale = true;
    }

    return true;
}
