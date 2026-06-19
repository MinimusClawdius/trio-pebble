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
    // Robust vertical centering with small bottom bias for Emery (200x228)
    // Ensures text has breathing room from bottom bezel
    int available = subcol.size.h;
    int pad = (available - text_h) / 2;
    if (pad < 0) pad = 0;
    
    // Small consistent bottom bias (pushes text ~1px up from exact center)
    if (pad >= 3) pad -= 1; else if (pad == 2) pad = 1;
    
    GRect result = GRect(subcol.origin.x, subcol.origin.y + pad, subcol.size.w, text_h);
    
    // ADD LOGGING
    APP_LOG(APP_LOG_LEVEL_DEBUG, "[COMP] footer_text_band_vcenter: subcol=(%d,%d %dx%d) text_h=%d -> result=(%d,%d %dx%d)",
            subcol.origin.x, subcol.origin.y, subcol.size.w, subcol.size.h,
            text_h, result.origin.x, result.origin.y, result.size.w, result.size.h);
    
    return result;
}

/** Match icon + text vertical center using full slot height (battery + %, etc.). */
static GRect footer_text_band_cell_mid(GRect cell, GRect text_col, int text_h) {
    int mid_y = cell.origin.y + cell.size.h / 2;
    int y = mid_y - text_h / 2;
    // Add minimum 2px padding when text is large
    int min_pad = 2;
    if (y < min_pad) y = min_pad;
    if (y + text_h > cell.size.h - min_pad) y = cell.size.h - text_h - min_pad;
    GRect result = GRect(text_col.origin.x, y, text_col.size.w, text_h);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "[COMP] footer_text_band_cell_mid: cell=(%d,%d %dx%d) text_h=%d -> result=(%d,%d %dx%d)",
            cell.origin.x, cell.origin.y, cell.size.w, cell.size.h, text_h, 
            result.origin.x, result.origin.y, result.size.w, result.size.h);
    return result;
}

static void draw_one_slot(GContext *ctx, GRect cell, ComplicationSlotKind kind, AppState *state, TrioConfig *config,
                          GColor fg) {
    char buf[24];
    int hs = config ? config->header_size : 0;
    int text_h = (hs == 0) ? 14 : (hs == 1) ? 18 : 24;
    APP_LOG(APP_LOG_LEVEL_INFO, "[COMP] draw_one_slot kind=%d cell=(%d,%d %dx%d) hs=%d",
            (int)kind, cell.origin.x, cell.origin.y, cell.size.w, cell.size.h, hs);
    GFont font_footer;
    if (hs == 0) font_footer = fonts_get_system_font(FONT_KEY_GOTHIC_14);
    else if (hs == 1) font_footer = fonts_get_system_font(FONT_KEY_GOTHIC_18);
    else font_footer = fonts_get_system_font(FONT_KEY_GOTHIC_24);
    APP_LOG(APP_LOG_LEVEL_INFO, "[COMPLICATIONS] footer font size selected for header_size=%d", hs);

    graphics_context_set_text_color(ctx, fg);
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "[COMP] draw_one_slot FINAL cell=(%d,%d %dx%d) kind=%d",
            cell.origin.x, cell.origin.y, cell.size.w, cell.size.h, (int)kind);

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
                GRect tb = footer_text_band_cell_mid(cell, tr, text_h);
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
            graphics_draw_text(ctx, buf, font_footer, footer_text_band_cell_mid(cell, cell, text_h),
                               GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
            return;
        case COMP_SLOT_STEPS: {
            int32_t st = state->comp.steps;
            if (st >= 10000) {
                snprintf(buf, sizeof(buf), "%dk", (int)(st / 1000));
            } else {
                snprintf(buf, sizeof(buf), "%d", (int)st);
            }
            graphics_draw_text(ctx, buf, font_footer, footer_text_band_cell_mid(cell, cell, text_h),
                               GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
            return;
        }
        case COMP_SLOT_HEART_RATE: {
            GRect ir, tr;
            slot_icon_text_split_pct(cell, &ir, &tr, true, 30);
            
            // Draw custom heart icon
            GColor heart_color = PBL_IF_COLOR_ELSE(GColorRed, fg);
            draw_heart_icon(ctx, ir, heart_color);
            
            if (state->comp.heart_rate <= 0) {
                snprintf(buf, sizeof(buf), "--");
            } else {
                snprintf(buf, sizeof(buf), "%d", state->comp.heart_rate);
            }
            GRect tb = footer_text_band_vcenter(tr, 18);
            graphics_draw_text(ctx, buf, font_footer, tb, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
            return;
        }
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
                // Slightly inset the text rect to prevent degree symbol cutoff
                GRect tb = footer_text_band_vcenter(tr, 18);
                tb.size.w -= 4;
                tb.origin.x -= 2;
                graphics_draw_text(ctx, buf, font_footer, tb, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
            }
            return;
        }
        default:
            return;
    }
}


// Heart icon resource ID - uses TRIO_HEART_ICON from package.json
#define HEART_ICON_RESOURCE_ID RESOURCE_ID_TRIO_HEART_ICON

void draw_heart_icon(GContext *ctx, GRect rect, GColor color) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "[COMP] draw_heart_icon called: rect=(%d,%d %dx%d)",
            rect.origin.x, rect.origin.y, rect.size.w, rect.size.h);
    
    // Try loading the image resource first (preferred)
    GBitmap *bmp = gbitmap_create_with_resource(HEART_ICON_RESOURCE_ID);
    if (bmp) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "[COMP] draw_heart_icon: using TRIO_HEART_ICON resource");
        
        // Invert bitmap when drawing on dark background (color == black)
        if (gcolor_equal(color, GColorBlack)) {
            graphics_context_set_compositing_mode(ctx, GCompOpAssignInverted);
        } else {
            graphics_context_set_compositing_mode(ctx, GCompOpAssign);
        }
        
        graphics_draw_bitmap_in_rect(ctx, bmp, rect);
        gbitmap_destroy(bmp);
        return;
    }
    
    APP_LOG(APP_LOG_LEVEL_WARNING, "[COMP] draw_heart_icon: TRIO_HEART_ICON not found, using fallback");
    
    // Fallback: draw a simple heart using circles + triangle
    //graphics_context_set_fill_color(ctx, color);
    
    //int cx = rect.origin.x + rect.size.w / 2;
    //int cy = rect.origin.y + rect.size.h / 2;
    //int r = rect.size.w / 3;
    
    //graphics_fill_circle(ctx, GPoint(cx - r/2, cy - r/3), r);
    //graphics_fill_circle(ctx, GPoint(cx + r/2, cy - r/3), r);
    
    // Fallback heart (two circles)

    //graphics_fill_triangle(ctx, points[0], points[1], points[2]);
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
        if (k > COMP_SLOT_WEATHER) {
            k = COMP_SLOT_NONE;
        }
        GRect cell = GRect(x + i * slot_w, y, slot_w, row_h);
        draw_one_slot(ctx, cell, k, state, config, fg);
    }
}
