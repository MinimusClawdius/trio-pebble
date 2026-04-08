#include "complications.h"
#include "complication_icons.h"
#include "platform_compat.h"
#include <stdio.h>

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

static void slot_icon_text_split_pct(GRect cell, GRect *out_icon, GRect *out_text, bool with_icon, int icon_pct) {
    if (!with_icon) {
        *out_icon = GRect(cell.origin.x, cell.origin.y, 0, 0);
        *out_text = cell;
        return;
    }
    int iw = cell.size.w * icon_pct / 100;
    if (iw < 18) {
        iw = 18;
    }
    if (iw > cell.size.w - 14) {
        iw = cell.size.w - 14;
    }
    *out_icon = GRect(cell.origin.x, cell.origin.y, iw, cell.size.h);
    *out_text = GRect(cell.origin.x + iw, cell.origin.y, cell.size.w - iw, cell.size.h);
}

static void slot_icon_text_split(GRect cell, GRect *out_icon, GRect *out_text, bool with_icon) {
    slot_icon_text_split_pct(cell, out_icon, out_text, with_icon, 46);
}

/** Vertical center a single-line footer label within a sub-rect. */
static GRect footer_text_band_vcenter(GRect subcol, int text_h) {
    int pad = (subcol.size.h - text_h) / 2;
    if (pad < 0) {
        pad = 0;
    }
    return GRect(subcol.origin.x, subcol.origin.y + pad, subcol.size.w, text_h);
}

/** Match icon + text vertical center using full slot height (battery + %, etc.). */
static GRect footer_text_band_cell_mid(GRect cell, GRect text_col, int text_h) {
    int mid_y = cell.origin.y + cell.size.h / 2;
    int y = mid_y - text_h / 2;
    return GRect(text_col.origin.x, y, text_col.size.w, text_h);
}

static void draw_one_slot(GContext *ctx, GRect cell, ComplicationSlotKind kind, AppState *state, TrioConfig *config,
                          GColor fg) {
    char buf[24];
    GFont font_footer = fonts_get_system_font(FONT_KEY_GOTHIC_14);

    graphics_context_set_text_color(ctx, fg);

    switch (kind) {
        case COMP_SLOT_NONE:
            return;
        case COMP_SLOT_WATCH_BATTERY: {
            GRect ir, tr;
            slot_icon_text_split_pct(cell, &ir, &tr, true, 40);
            int mid_y = cell.origin.y + cell.size.h / 2;
            trio_draw_footer_battery_bar(ctx, ir, mid_y, state->comp.watch_battery, state->comp.watch_charging, fg,
                                         config);
            if (state->comp.watch_charging) {
                snprintf(buf, sizeof(buf), "%d+", state->comp.watch_battery);
            } else {
                snprintf(buf, sizeof(buf), "%d%%", state->comp.watch_battery);
            }
            {
                GRect tb = footer_text_band_cell_mid(cell, tr, 16);
                graphics_draw_text(ctx, buf, font_footer, tb, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
            }
            return;
        }
        case COMP_SLOT_PHONE_BATTERY:
            if (state->comp.phone_battery <= 0) {
                snprintf(buf, sizeof(buf), "--");
            } else {
                snprintf(buf, sizeof(buf), "P%d%%", state->comp.phone_battery);
            }
            graphics_draw_text(ctx, buf, font_footer, footer_text_band_vcenter(cell, 16),
                               GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
            return;
        case COMP_SLOT_STEPS: {
            int32_t st = state->comp.steps;
            if (st >= 10000) {
                snprintf(buf, sizeof(buf), "%dk", (int)(st / 1000));
            } else {
                snprintf(buf, sizeof(buf), "%d", (int)st);
            }
            graphics_draw_text(ctx, buf, font_footer, footer_text_band_vcenter(cell, 16),
                               GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
            return;
        }
        case COMP_SLOT_HEART_RATE:
            if (state->comp.heart_rate <= 0) {
                snprintf(buf, sizeof(buf), "--");
            } else {
                snprintf(buf, sizeof(buf), "%d", state->comp.heart_rate);
            }
            graphics_draw_text(ctx, buf, font_footer, footer_text_band_vcenter(cell, 16),
                               GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
            return;
        case COMP_SLOT_WEATHER: {
            GRect ir, tr;
            slot_icon_text_split(cell, &ir, &tr, true);
            if (config->weather_enabled && state->comp.weather_icon[0] && state->comp.weather_icon[0] != '\0') {
                trio_draw_footer_weather_icon(ctx, ir, state->comp.weather_icon, config);
            }
            if (!config->weather_enabled) {
                snprintf(buf, sizeof(buf), "off");
            } else if (state->comp.weather_temp == 0) {
                snprintf(buf, sizeof(buf), "--");
            } else {
                snprintf(buf, sizeof(buf), "%d°", state->comp.weather_temp);
            }
            {
                GRect tb = footer_text_band_vcenter(tr, 16);
                graphics_draw_text(ctx, buf, font_footer, tb, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter,
                                   NULL);
            }
            return;
        }
        case COMP_SLOT_IOB: {
            GRect ir, tr;
            slot_icon_text_split(cell, &ir, &tr, true);
            trio_draw_footer_iob_icon(ctx, ir, fg);
            if (state->loop.iob[0] == '\0') {
                snprintf(buf, sizeof(buf), "--");
            } else {
                snprintf(buf, sizeof(buf), "%s", state->loop.iob);
            }
            {
                GRect tb = footer_text_band_vcenter(tr, 16);
                graphics_draw_text(ctx, buf, font_footer, tb, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter,
                                   NULL);
            }
            return;
        }
        default:
            return;
    }
}

void complications_draw_bar(GContext *ctx, GRect area, AppState *state, TrioConfig *config) {
    int x = area.origin.x;
    int y = area.origin.y;
    int row_h = area.size.h;
    int aw = area.size.w;
    int slot_w = aw / TRIO_COMP_BAR_COLUMNS;

    GColor fg;
#if TRIO_DISPLAY_COLOR
    if (trio_classic_footer_light_ink(config)) {
        fg = GColorWhite;
    } else if (trio_classic_chrome_active(config)) {
        fg = GColorBlack;
    } else if (config->color_scheme == COLOR_SCHEME_LIGHT) {
        fg = GColorDarkGray;
    } else {
        fg = GColorLightGray;
    }
#else
    if (trio_classic_footer_light_ink(config)) {
        fg = GColorWhite;
    } else if (trio_classic_chrome_active(config)) {
        fg = GColorBlack;
    } else if (config->color_scheme == COLOR_SCHEME_LIGHT) {
        fg = GColorBlack;
    } else {
        fg = GColorWhite;
    }
#endif

    for (int i = 0; i < TRIO_COMP_BAR_COLUMNS; i++) {
        ComplicationSlotKind k = (ComplicationSlotKind)config->comp_slot[i];
        if (k > COMP_SLOT_IOB) {
            k = COMP_SLOT_NONE;
        }
        GRect cell = GRect(x + i * slot_w, y, slot_w, row_h);
        draw_one_slot(ctx, cell, k, state, config, fg);
    }
}
