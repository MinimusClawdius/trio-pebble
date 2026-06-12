#include "tap_framework.h"
#include "config.h"
#include <string.h>

#define MAX_TAP_ZONES 8

static TapZone s_zones[MAX_TAP_ZONES];
static int s_zone_count = 0;

#if defined(PBL_TOUCH)
static bool s_tracking = false;
static int16_t s_start_x = 0;
static int16_t s_start_y = 0;
static GRect s_graph_bounds = {{0, 0}, {0, 0}};
#endif

void tap_framework_init(void) {
    s_zone_count = 0;
    memset(s_zones, 0, sizeof(s_zones));
}

void tap_framework_register_zone(GRect zone, TapAction action, const char *label) {
    if (s_zone_count >= MAX_TAP_ZONES) return;
    s_zones[s_zone_count].zone = zone;
    s_zones[s_zone_count].action = action;
    s_zones[s_zone_count].label = label;
    s_zone_count++;
}

void tap_framework_handle_tap(AccelAxisType axis, int32_t direction) {
    (void)axis; (void)direction;
    tap_framework_send_action(TAP_ACTION_REFRESH);
}

TapZone *tap_framework_find_zone(GPoint point) {
    for (int i = 0; i < s_zone_count; i++) {
        if (grect_contains_point(&s_zones[i].zone, &point)) {
            return &s_zones[i];
        }
    }
    return NULL;
}

TapAction tap_framework_resolve(GPoint touch_point) {
    TapZone *zone = tap_framework_find_zone(touch_point);
    return zone ? zone->action : TAP_ACTION_NONE;
}

void tap_framework_send_action(TapAction action) {
    if (action == TAP_ACTION_NONE) return;

    if (action == TAP_ACTION_CYCLE_GRAPH_TIME) {
        tap_framework_cycle_graph_time_range();
        return;
    }

    DictionaryIterator *iter;
    AppMessageResult result = app_message_outbox_begin(&iter);
    if (result == APP_MSG_OK) {
        dict_write_int32(iter, KEY_TAP_ACTION, (int32_t)action);
        app_message_outbox_send();
    }
}

void tap_framework_cycle_graph_time_range(void) {
    TrioConfig *cfg = config_get();
    if (!cfg) return;

    uint8_t current = cfg->graph_time_range;
    uint8_t next = (current + 1) % 4;

    cfg->graph_time_range = next;
    config_save();

    APP_LOG(APP_LOG_LEVEL_INFO, "[TAP] Cycled graph time range to %d", next);

    // Trigger a refresh so the phone sends data for the new range
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
        dict_write_uint8(iter, KEY_TAP_ACTION, TAP_ACTION_REFRESH);
        app_message_outbox_send();
    }
}

#if defined(PBL_TOUCH)
void tap_framework_set_graph_bounds(GRect bounds) {
    s_graph_bounds = bounds;
}

void tap_framework_handle_touch_event(const TouchEvent *event, void *context) {
    (void)context;
    if (!event) return;

    switch (event->type) {
        case TouchEvent_Touchdown:
            s_tracking = true;
            s_start_x = event->x;
            s_start_y = event->y;
            break;

        case TouchEvent_Liftoff: {
            if (!s_tracking) break;

            int16_t dx = event->x - s_start_x;
            int16_t dy = event->y - s_start_y;
            int16_t adx = dx < 0 ? -dx : dx;
            int16_t ady = dy < 0 ? -dy : dy;

            const int16_t HSWIPE_THRESHOLD = 30;

            bool in_graph = grect_contains_point(&s_graph_bounds,
                &(GPoint){ .x = event->x, .y = event->y });

            if (adx > HSWIPE_THRESHOLD && adx > ady && in_graph) {
                tap_framework_send_action(TAP_ACTION_CYCLE_GRAPH_TIME);
            }

            s_tracking = false;
            break;
        }
        default:
            break;
    }
}
#endif
