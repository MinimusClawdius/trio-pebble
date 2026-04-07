#include "complications.h"
#include "platform_compat.h"

void complications_init(void) {
    // Register battery state handler
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

/*
 * Bottom bar, left → right: watch battery %, step count (today), heart rate, weather °C/°F.
 * Ellipsis on a value means the column was too narrow — widths are biased so "100%" fits.
 * The small centered number (e.g. 172) is step count from Health / phone sync, not glucose.
 */
void complications_draw_bar(GContext *ctx, GRect area, AppState *state, TrioConfig *config) {
    char buf[64];
    int x = area.origin.x + 2;
    int y = area.origin.y;
    int row_h = area.size.h;
    int aw = area.size.w - 4;
    /* Unequal columns: battery + weather use bold; steps/HR need readable width */
    int w_batt = aw * 38 / 100;
    int w_step = aw * 30 / 100;
    int w_hr = aw * 17 / 100;
    int w_wx = aw - w_batt - w_step - w_hr;

    GFont font_batt = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    GFont font_mid = fonts_get_system_font(FONT_KEY_GOTHIC_18);
    GFont font_hr = fonts_get_system_font(FONT_KEY_GOTHIC_14);

#if TRIO_DISPLAY_COLOR
    GColor fg = (config->color_scheme == COLOR_SCHEME_LIGHT) ? GColorDarkGray : GColorLightGray;
#else
    GColor fg = (config->color_scheme == COLOR_SCHEME_LIGHT) ? GColorBlack : GColorWhite;
#endif

    graphics_context_set_text_color(ctx, fg);
    if (state->comp.watch_charging) {
        snprintf(buf, sizeof(buf), "%d%%+", state->comp.watch_battery);
    } else {
        snprintf(buf, sizeof(buf), "%d%%", state->comp.watch_battery);
    }
    graphics_draw_text(ctx, buf, font_batt, GRect(x, y - 1, w_batt, row_h + 2),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

    graphics_context_set_text_color(ctx, fg);
    if (state->comp.steps > 0) {
        if (state->comp.steps >= 10000) {
            snprintf(buf, sizeof(buf), "%dk", (int)(state->comp.steps / 1000));
        } else {
            snprintf(buf, sizeof(buf), "%d", (int)state->comp.steps);
        }
        graphics_draw_text(ctx, buf, font_mid, GRect(x + w_batt, y - 1, w_step, row_h + 2),
                           GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }

    if (state->comp.heart_rate > 0) {
        snprintf(buf, sizeof(buf), "%d", state->comp.heart_rate);
        graphics_draw_text(ctx, buf, font_hr, GRect(x + w_batt + w_step, y, w_hr, row_h),
                           GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }

    if (config->weather_enabled && state->comp.weather_temp != 0) {
        snprintf(buf, sizeof(buf), "%d°", state->comp.weather_temp);
        graphics_draw_text(ctx, buf, font_batt, GRect(x + w_batt + w_step + w_hr, y - 1, w_wx, row_h + 2),
                           GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    }
}
