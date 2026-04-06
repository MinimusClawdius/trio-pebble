#include "weather_background.h"
#include <string.h>
#include <time.h>

static bool is_local_nighttime(void) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int h = t->tm_hour;
    /* ~6:00-20:59 treated as day (tunable). */
    return (h < 6 || h >= 21);
}

static bool icon_is(const char *icon, const char *tag) {
    return icon && strncmp(icon, tag, 16) == 0;
}

static void sky_base(GContext *ctx, GRect b, bool night, bool light_scheme) {
#ifdef PBL_COLOR
    /* Core palette only (Basalt/Chalk/Aplite+ color builds). */
    if (night) {
        graphics_context_set_fill_color(ctx, light_scheme ? GColorBlue : GColorDukeBlue);
    } else {
        graphics_context_set_fill_color(ctx, light_scheme ? GColorCyan : GColorBlue);
    }
#else
    (void)night;
    (void)light_scheme;
    graphics_context_set_fill_color(ctx, GColorBlack);
#endif
    graphics_fill_rect(ctx, b, 0, GCornerNone);
#ifdef PBL_COLOR
    /* Ground haze band — keeps graph area readable. */
    int gh = b.size.h / 5;
    GRect ground = GRect(b.origin.x, b.origin.y + b.size.h - gh, b.size.w, gh);
    if (night) {
        graphics_context_set_fill_color(ctx, light_scheme ? GColorDarkGray : GColorDukeBlue);
    } else {
        graphics_context_set_fill_color(ctx, light_scheme ? GColorGreen : GColorIslamicGreen);
    }
    graphics_fill_rect(ctx, ground, 0, GCornerNone);
#endif
}

#ifdef PBL_COLOR
static void draw_star(GContext *ctx, GPoint p, int max_w) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(p.x, p.y, 1, 1), 0, GCornerNone);
    if (p.x + 1 < max_w) {
        graphics_fill_rect(ctx, GRect(p.x + 1, p.y, 1, 1), 0, GCornerNone);
    }
}

static void draw_stars_field(GContext *ctx, GRect b) {
    /* Fixed pattern so it does not flicker each redraw. */
    const GPoint stars[] = {
        { 8, 10 }, { 22, 6 }, { 40, 14 }, { 58, 8 }, { 72, 12 },
        { 90, 5 }, { 104, 16 }, { 120, 9 }, { 12, 22 }, { 100, 24 }
    };
    unsigned int si;
    for (si = 0; si < sizeof(stars) / sizeof(stars[0]); si++) {
        GPoint p = stars[si];
        if (p.x < b.size.w && p.y < b.size.h) {
            draw_star(ctx, GPoint(b.origin.x + p.x, b.origin.y + p.y), b.origin.x + b.size.w);
        }
    }
}

static void draw_moon(GContext *ctx, int cx, int cy, GColor sky) {
    graphics_context_set_fill_color(ctx, GColorLightGray);
    graphics_fill_circle(ctx, GPoint(cx, cy), 10);
    graphics_context_set_fill_color(ctx, sky);
    graphics_fill_circle(ctx, GPoint(cx + 5, cy - 2), 9);
}

static void draw_sun_simple(GContext *ctx, int cx, int cy) {
    static const int8_t rdx[] = { 1, 1, 0, -1, -1, -1, 0, 1 };
    static const int8_t rdy[] = { 0, -1, -1, -1, 0, 1, 1, 1 };
    graphics_context_set_fill_color(ctx, GColorYellow);
    graphics_fill_circle(ctx, GPoint(cx, cy), 12);
    graphics_context_set_stroke_color(ctx, GColorOrange);
    graphics_context_set_stroke_width(ctx, 2);
    for (int i = 0; i < 8; i++) {
        int x0 = cx + 14 * rdx[i];
        int y0 = cy + 14 * rdy[i];
        int x1 = cx + 22 * rdx[i];
        int y1 = cy + 22 * rdy[i];
        graphics_draw_line(ctx, GPoint(x0, y0), GPoint(x1, y1));
    }
}

static void draw_cloud_blob(GContext *ctx, int ax, int ay, GColor c) {
    graphics_context_set_fill_color(ctx, c);
    graphics_fill_circle(ctx, GPoint(ax, ay), 8);
    graphics_fill_circle(ctx, GPoint(ax + 10, ay + 2), 9);
    graphics_fill_circle(ctx, GPoint(ax + 20, ay), 7);
}

static void draw_rain(GContext *ctx, GRect b) {
    graphics_context_set_stroke_color(ctx, GColorCyan);
    graphics_context_set_stroke_width(ctx, 1);
    for (int x = 4; x < b.size.w; x += 7) {
        int y0 = b.origin.y + b.size.h / 3;
        int y1 = b.origin.y + b.size.h - 6;
        for (int y = y0; y < y1; y += 9) {
            graphics_draw_line(ctx, GPoint(b.origin.x + x, y), GPoint(b.origin.x + x + 3, y + 6));
        }
    }
}

static void draw_snow(GContext *ctx, GRect b) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    for (int x = 6; x < b.size.w; x += 11) {
        for (int y = b.size.h / 4; y < b.size.h - 8; y += 13) {
            graphics_fill_circle(ctx, GPoint(b.origin.x + x + (y % 7), b.origin.y + y), 2);
        }
    }
}

static void draw_lightning(GContext *ctx, int x, int y) {
    graphics_context_set_stroke_color(ctx, GColorYellow);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(ctx, GPoint(x, y), GPoint(x + 4, y + 10));
    graphics_draw_line(ctx, GPoint(x + 4, y + 10), GPoint(x - 2, y + 14));
    graphics_draw_line(ctx, GPoint(x - 2, y + 14), GPoint(x + 3, y + 28));
}

static void draw_fog_layer(GContext *ctx, GRect b) {
    graphics_context_set_fill_color(ctx, GColorLightGray);
    for (int y = b.size.h / 3; y < b.size.h - 10; y += 6) {
        graphics_fill_rect(ctx, GRect(b.origin.x, b.origin.y + y, b.size.w, 2), 0, GCornerNone);
    }
}
#else
static void draw_stars_field(GContext *ctx, GRect b) { (void)ctx; (void)b; }
static void draw_moon(GContext *ctx, int cx, int cy, GColor sky) { (void)ctx; (void)cx; (void)cy; (void)sky; }
static void draw_sun_simple(GContext *ctx, int cx, int cy) { (void)ctx; (void)cx; (void)cy; }
static void draw_cloud_blob(GContext *ctx, int ax, int ay, GColor c) { (void)ctx; (void)ax; (void)ay; (void)c; }
static void draw_rain(GContext *ctx, GRect b) {
    graphics_context_set_stroke_color(ctx, GColorWhite);
    for (int x = 4; x < b.size.w; x += 6) {
        graphics_draw_line(ctx, GPoint(b.origin.x + x, b.origin.y + 20), GPoint(b.origin.x + x + 2, b.origin.y + 40));
    }
}
static void draw_snow(GContext *ctx, GRect b) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    for (int x = 4; x < b.size.w; x += 10) {
        for (int y = 24; y < b.size.h - 6; y += 12) {
            graphics_fill_rect(ctx, GRect(b.origin.x + x, b.origin.y + y, 2, 2), 0, GCornerNone);
        }
    }
}
static void draw_lightning(GContext *ctx, int x, int y) { (void)ctx; (void)x; (void)y; }
static void draw_fog_layer(GContext *ctx, GRect b) { (void)ctx; (void)b; }
#endif

void weather_background_draw(GContext *ctx, GRect bounds, const Complications *comp, const TrioConfig *config) {
    if (!config->weather_enabled || !comp) return;
    const char *icon = comp->weather_icon;
    if (!icon[0] || icon_is(icon, "off")) return;

    bool night = is_local_nighttime();
    bool light = config->color_scheme == COLOR_SCHEME_LIGHT;

    sky_base(ctx, bounds, night, light);

#ifdef PBL_COLOR
    int w = bounds.size.w;
    int cx = bounds.origin.x + w - 22;
    int cy = bounds.origin.y + 22;
    GColor moon_sky = light ? GColorBlue : GColorDukeBlue;

    if (icon_is(icon, "rain") || icon_is(icon, "storm")) {
        draw_cloud_blob(ctx, bounds.origin.x + w / 2 - 22, bounds.origin.y + 8,
                        night ? GColorDarkGray : GColorLightGray);
    }
    if (icon_is(icon, "snow")) {
        draw_cloud_blob(ctx, bounds.origin.x + w / 2 - 22, bounds.origin.y + 6, GColorLightGray);
    }

    if (icon_is(icon, "clear")) {
        if (night) {
            draw_stars_field(ctx, bounds);
            draw_moon(ctx, cx, cy, moon_sky);
        } else {
            draw_sun_simple(ctx, cx, cy);
        }
    } else if (icon_is(icon, "mainly_clr")) {
        if (night) {
            draw_stars_field(ctx, bounds);
            draw_moon(ctx, cx - 4, cy, moon_sky);
            draw_cloud_blob(ctx, bounds.origin.x + 16, bounds.origin.y + 10, GColorDarkGray);
        } else {
            draw_sun_simple(ctx, cx, cy);
            draw_cloud_blob(ctx, bounds.origin.x + 12, bounds.origin.y + 14, GColorWhite);
        }
    } else if (icon_is(icon, "partly")) {
        if (night) {
            draw_stars_field(ctx, bounds);
            draw_moon(ctx, cx, cy, moon_sky);
            draw_cloud_blob(ctx, bounds.origin.x + w / 2 - 26, bounds.origin.y + 12, GColorDarkGray);
            draw_cloud_blob(ctx, bounds.origin.x + 8, bounds.origin.y + 18, GColorDarkGray);
        } else {
            draw_sun_simple(ctx, cx, cy);
            draw_cloud_blob(ctx, bounds.origin.x + w / 2 - 30, bounds.origin.y + 16, GColorWhite);
            draw_cloud_blob(ctx, bounds.origin.x + 6, bounds.origin.y + 20, GColorLightGray);
        }
    } else if (icon_is(icon, "overcast")) {
        draw_cloud_blob(ctx, bounds.origin.x + w / 2 - 28, bounds.origin.y + 10, GColorDarkGray);
        draw_cloud_blob(ctx, bounds.origin.x + 8, bounds.origin.y + 16, GColorLightGray);
        draw_cloud_blob(ctx, bounds.origin.x + w - 48, bounds.origin.y + 14, GColorDarkGray);
    } else if (icon_is(icon, "fog")) {
        draw_fog_layer(ctx, bounds);
    } else if (icon_is(icon, "rain")) {
        draw_rain(ctx, bounds);
    } else if (icon_is(icon, "snow")) {
        draw_snow(ctx, bounds);
    } else if (icon_is(icon, "storm")) {
        draw_rain(ctx, bounds);
        draw_lightning(ctx, bounds.origin.x + w / 2, bounds.origin.y + 28);
    } else if (icon_is(icon, "cloudy")) {
        /* Legacy JS icon → treat as overcast */
        draw_cloud_blob(ctx, bounds.origin.x + w / 2 - 28, bounds.origin.y + 10, GColorDarkGray);
        draw_cloud_blob(ctx, bounds.origin.x + 8, bounds.origin.y + 16, GColorLightGray);
        draw_cloud_blob(ctx, bounds.origin.x + w - 48, bounds.origin.y + 14, GColorDarkGray);
    } else {
        /* Unknown code: subtle clouds */
        draw_cloud_blob(ctx, bounds.origin.x + w / 2 - 20, bounds.origin.y + 12, GColorLightGray);
    }
#else
    (void)night;
    if (icon_is(icon, "rain") || icon_is(icon, "storm")) draw_rain(ctx, bounds);
    else if (icon_is(icon, "snow")) draw_snow(ctx, bounds);
#endif
}
