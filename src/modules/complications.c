#include "complications.h"
#include "platform_compat.h"

void complications_init(void) {
    complications_update_battery();
    complications_update_health();
}

void complications_update_battery(void) {
    AppState *state = app_state_get();
    BatteryChargeState batt = battery_state_service_peek();
    state->comp.watch_battery = batt.charge_percent;
    state->comp.watch_charging = batt.is_charging;
}

void complications_update_health(void) {
#if defined(PBL_HEALTH)
    AppState *state = app_state_get();
    state->comp.steps = (int32_t)health_service_sum_today(HealthMetricStepCount);
    HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
    state->comp.heart_rate = (hr > 0) ? (int16_t)hr : 0;
#endif
}

void complications_apply_message(DictionaryIterator *iter, AppState *state) {
    Tuple *t;

    t = dict_find(iter, KEY_BATTERY_PHONE);
    if (t) state->comp.phone_battery = (int8_t)t->value->int32;

    t = dict_find(iter, KEY_WEATHER_TEMP);
    if (t) state->comp.weather_temp = (int16_t)t->value->int32;

    t = dict_find(iter, KEY_WEATHER_ICON);
    if (t) {
        strncpy(state->comp.weather_icon, t->value->cstring, sizeof(state->comp.weather_icon) - 1);
    }

    t = dict_find(iter, KEY_STEPS);
    if (t) state->comp.steps = t->value->int32;

    t = dict_find(iter, KEY_HEART_RATE);
    if (t) state->comp.heart_rate = (int16_t)t->value->int32;
}

static void draw_one_slot(GContext *ctx, GRect cell, ComplicationSlotKind kind, AppState *state, TrioConfig *config,
                          GColor fg) {
    char buf[24];
    GFont font_main = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    GFont font_aux = fonts_get_system_font(FONT_KEY_GOTHIC_14);

    graphics_context_set_text_color(ctx, fg);

    switch (kind) {
        case COMP_SLOT_NONE:
            return;
        case COMP_SLOT_WATCH_BATTERY:
            if (state->comp.watch_charging) {
                snprintf(buf, sizeof(buf), "%d%%+", state->comp.watch_battery);
            } else {
                snprintf(buf, sizeof(buf), "%d%%", state->comp.watch_battery);
            }
            graphics_draw_text(ctx, buf, font_main, cell, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
            return;
        case COMP_SLOT_PHONE_BATTERY:
            if (state->comp.phone_battery <= 0) {
                snprintf(buf, sizeof(buf), "--");
            } else {
                snprintf(buf, sizeof(buf), "P%d%%", state->comp.phone_battery);
            }
            graphics_draw_text(ctx, buf, font_aux, cell, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
            return;
        case COMP_SLOT_STEPS: {
            int32_t st = state->comp.steps;
            if (st >= 10000) {
                snprintf(buf, sizeof(buf), "%dk", (int)(st / 1000));
            } else {
                snprintf(buf, sizeof(buf), "%d", (int)st);
            }
            graphics_draw_text(ctx, buf, font_main, cell, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
            return;
        }
        case COMP_SLOT_HEART_RATE:
            if (state->comp.heart_rate <= 0) {
                snprintf(buf, sizeof(buf), "--");
            } else {
                snprintf(buf, sizeof(buf), "%d", state->comp.heart_rate);
            }
            graphics_draw_text(ctx, buf, font_main, cell, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
            return;
        case COMP_SLOT_WEATHER:
            if (!config->weather_enabled) {
                snprintf(buf, sizeof(buf), "off");
            } else if (state->comp.weather_temp == 0) {
                snprintf(buf, sizeof(buf), "--");
            } else {
                snprintf(buf, sizeof(buf), "%d°", state->comp.weather_temp);
            }
            graphics_draw_text(ctx, buf, font_main, cell, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
            return;
        case COMP_SLOT_IOB:
            if (state->loop.iob[0] == '\0') {
                snprintf(buf, sizeof(buf), "--");
            } else {
                snprintf(buf, sizeof(buf), "%s", state->loop.iob);
            }
            graphics_draw_text(ctx, buf, font_main, cell, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
            return;
        default:
            return;
    }
}

/*
 * Four equal columns; each maps to comp_slot[0..3] (see phone config HTML).
 * Data: watch battery, phone % (from phone), steps, HR (Health), weather (when enabled + fetched).
 */
void complications_draw_bar(GContext *ctx, GRect area, AppState *state, TrioConfig *config) {
    int x = area.origin.x + 2;
    int y = area.origin.y;
    int row_h = area.size.h;
    int aw = area.size.w - 4;
    int slot_w = aw / TRIO_COMP_SLOT_COUNT;

#if TRIO_DISPLAY_COLOR
    GColor fg = (config->color_scheme == COLOR_SCHEME_LIGHT) ? GColorDarkGray : GColorLightGray;
#else
    GColor fg = (config->color_scheme == COLOR_SCHEME_LIGHT) ? GColorBlack : GColorWhite;
#endif

    for (int i = 0; i < TRIO_COMP_SLOT_COUNT; i++) {
        ComplicationSlotKind k = (ComplicationSlotKind)config->comp_slot[i];
        if (k > COMP_SLOT_IOB) {
            k = COMP_SLOT_NONE;
        }
        GRect cell = GRect(x + i * slot_w, y - 1, slot_w, row_h + 2);
        draw_one_slot(ctx, cell, k, state, config, fg);
    }
}
