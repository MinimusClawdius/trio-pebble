#include "complication_icons.h"
#include <string.h>

static bool wx_icon_is(const char *icon, const char *tag) {
    return icon && strncmp(icon, tag, 16) == 0;
}

void trio_draw_footer_battery_bar(GContext *ctx, GRect icon_area, int align_mid_y, int pct, bool charging,
                                  GColor border_ink, const TrioConfig *config) {
    (void)config;
    (void)charging;
    if (pct < 0) {
        pct = 0;
    }
    if (pct > 100) {
        pct = 100;
    }

    int bar_h = 10;
    if (bar_h > icon_area.size.h - 2) {
        bar_h = icon_area.size.h - 2;
    }
    int bar_w = icon_area.size.w - 2;
    if (bar_w < 8) {
        return;
    }
    int bx = icon_area.origin.x + 1;
    int by;
    if (align_mid_y >= 0) {
        by = align_mid_y - bar_h / 2;
        if (by < icon_area.origin.y) {
            by = icon_area.origin.y;
        }
        if (by + bar_h > icon_area.origin.y + icon_area.size.h) {
            by = icon_area.origin.y + icon_area.size.h - bar_h;
        }
    } else {
        by = icon_area.origin.y + (icon_area.size.h - bar_h) / 2;
    }
    GRect bar = GRect(bx, by, bar_w, bar_h);

    graphics_context_set_stroke_color(ctx, border_ink);
    graphics_draw_round_rect(ctx, bar, 2);

    int inner_max = bar.size.w - 4;
    int fill_w = (pct * inner_max) / 100;
    if (fill_w < 1 && pct > 0) {
        fill_w = 1;
    }

    GColor fill;
#ifdef PBL_COLOR
    if (pct <= 20) {
        fill = GColorRed;
    } else if (pct <= 40) {
        fill = GColorChromeYellow;
    } else {
        fill = GColorGreen;
    }
#else
    fill = border_ink;
#endif

    graphics_context_set_fill_color(ctx, fill);
    if (fill_w > 0) {
        graphics_fill_rect(ctx, GRect(bar.origin.x + 2, bar.origin.y + 2, fill_w, bar.size.h - 4), 1, GCornerNone);
    }
}

#ifdef PBL_COLOR
static void mini_cloud(GContext *ctx, int ax, int ay, GColor c) {
    graphics_context_set_fill_color(ctx, c);
    graphics_fill_circle(ctx, GPoint(ax, ay), 4);
    graphics_fill_circle(ctx, GPoint(ax + 5, ay + 1), 5);
    graphics_fill_circle(ctx, GPoint(ax + 10, ay), 4);
}

static void mini_sun(GContext *ctx, int cx, int cy) {
    graphics_context_set_fill_color(ctx, GColorYellow);
    graphics_fill_circle(ctx, GPoint(cx, cy), 5);
    graphics_context_set_stroke_color(ctx, GColorOrange);
    graphics_context_set_stroke_width(ctx, 1);
    static const int8_t rdx[] = { 1, 1, 0, -1, -1, -1, 0, 1 };
    static const int8_t rdy[] = { 0, -1, -1, -1, 0, 1, 1, 1 };
    for (int i = 0; i < 8; i++) {
        graphics_draw_line(ctx, GPoint(cx + 6 * rdx[i], cy + 6 * rdy[i]), GPoint(cx + 10 * rdx[i], cy + 10 * rdy[i]));
    }
}
#endif

void trio_draw_footer_weather_icon(GContext *ctx, GRect icon_area, const char *weather_icon,
                                   const TrioConfig *config) {
    if (!weather_icon || !weather_icon[0] || wx_icon_is(weather_icon, "off")) {
        return;
    }

    int cx = icon_area.origin.x + icon_area.size.w / 2;
    int cy = icon_area.origin.y + icon_area.size.h / 2;
    bool light = config && config->color_scheme == COLOR_SCHEME_LIGHT;

#ifdef PBL_COLOR
    if (wx_icon_is(weather_icon, "clear") || wx_icon_is(weather_icon, "mainly_clr")) {
        mini_sun(ctx, cx, cy);
        if (wx_icon_is(weather_icon, "mainly_clr")) {
            mini_cloud(ctx, cx - 8, cy + 2, light ? GColorDarkGray : GColorLightGray);
        }
    } else if (wx_icon_is(weather_icon, "partly")) {
        mini_sun(ctx, cx + 4, cy - 2);
        mini_cloud(ctx, cx - 8, cy + 2, GColorLightGray);
    } else if (wx_icon_is(weather_icon, "overcast") || wx_icon_is(weather_icon, "cloudy")) {
        mini_cloud(ctx, cx - 6, cy - 2, GColorDarkGray);
        mini_cloud(ctx, cx + 2, cy, GColorLightGray);
    } else if (wx_icon_is(weather_icon, "fog")) {
        graphics_context_set_fill_color(ctx, GColorLightGray);
        for (int i = -1; i <= 1; i++) {
            graphics_fill_rect(ctx, GRect(cx - 10, cy + i * 3, 20, 2), 0, GCornerNone);
        }
    } else if (wx_icon_is(weather_icon, "rain") || wx_icon_is(weather_icon, "storm")) {
        mini_cloud(ctx, cx - 6, cy - 4, GColorLightGray);
        graphics_context_set_stroke_color(ctx, GColorCyan);
        graphics_context_set_stroke_width(ctx, 1);
        for (int i = -1; i <= 1; i++) {
            graphics_draw_line(ctx, GPoint(cx + i * 5, cy + 2), GPoint(cx + i * 5 + 2, cy + 10));
        }
        if (wx_icon_is(weather_icon, "storm")) {
            graphics_context_set_stroke_color(ctx, GColorYellow);
            graphics_draw_line(ctx, GPoint(cx, cy + 2), GPoint(cx + 2, cy + 8));
            graphics_draw_line(ctx, GPoint(cx + 2, cy + 8), GPoint(cx - 1, cy + 10));
        }
    } else if (wx_icon_is(weather_icon, "snow")) {
        mini_cloud(ctx, cx - 6, cy - 4, GColorLightGray);
        graphics_context_set_fill_color(ctx, GColorWhite);
        for (int i = -1; i <= 1; i++) {
            graphics_fill_circle(ctx, GPoint(cx + i * 5, cy + 6), 2);
        }
    } else {
        mini_cloud(ctx, cx - 4, cy - 1, GColorLightGray);
    }
#else
    graphics_context_set_stroke_color(ctx, light ? GColorBlack : GColorWhite);
    graphics_context_set_stroke_width(ctx, 1);
    if (wx_icon_is(weather_icon, "rain") || wx_icon_is(weather_icon, "storm")) {
        for (int i = -1; i <= 1; i++) {
            graphics_draw_line(ctx, GPoint(cx + i * 4, cy - 2), GPoint(cx + i * 4 + 2, cy + 8));
        }
    } else {
        graphics_draw_circle(ctx, GPoint(cx, cy), 6);
    }
#endif
}

void trio_draw_footer_iob_icon(GContext *ctx, GRect icon_area, GColor ink) {
    int cx = icon_area.origin.x + icon_area.size.w / 2;
    int cy = icon_area.origin.y + icon_area.size.h / 2;
    graphics_context_set_fill_color(ctx, ink);
    graphics_fill_circle(ctx, GPoint(cx, cy - 1), 4);
    graphics_fill_rect(ctx, GRect(cx - 2, cy + 2, 4, 6), 0, GCornerNone);
    graphics_fill_circle(ctx, GPoint(cx, cy + 9), 3);
}
