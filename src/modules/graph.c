#include "graph.h"
#include "platform_compat.h"
#include "weather_background.h"
#include <string.h>

/* CloudPebble / strict C: callers must not see grid_color before its prototype. */
static GColor grid_color(TrioConfig *config);

static int16_t s_values[MAX_GRAPH_POINTS];
static int s_count = 0;
static int16_t s_predictions[MAX_PREDICTIONS];
static int s_pred_count = 0;

#define GRAPH_MIN 40
#define GRAPH_MAX 400

void graph_init(void) {
    s_count = 0;
    s_pred_count = 0;
    memset(s_values, 0, sizeof(s_values));
    memset(s_predictions, 0, sizeof(s_predictions));
}

void graph_deinit(void) {
    s_count = 0;
    s_pred_count = 0;
}

void graph_set_data(int16_t *values, int count) {
    if (count > MAX_GRAPH_POINTS) count = MAX_GRAPH_POINTS;
    s_count = count;
    memcpy(s_values, values, count * sizeof(int16_t));
}

void graph_set_predictions(int16_t *values, int count) {
    if (count > MAX_PREDICTIONS) count = MAX_PREDICTIONS;
    s_pred_count = count;
    memcpy(s_predictions, values, count * sizeof(int16_t));
}

void graph_restore_from_state(const GraphData *gd) {
    if (!gd || gd->count <= 0) return;
    graph_set_data((int16_t *)gd->values, gd->count);
    if (gd->prediction_count > 0) {
        graph_set_predictions((int16_t *)gd->predictions, gd->prediction_count);
    }
}

static int map_y(int glucose, int height) {
    int clamped = glucose;
    if (clamped < GRAPH_MIN) clamped = GRAPH_MIN;
    if (clamped > GRAPH_MAX) clamped = GRAPH_MAX;
    return height - ((clamped - GRAPH_MIN) * height / (GRAPH_MAX - GRAPH_MIN));
}

static GColor glucose_color(int glucose, TrioConfig *config) {
#ifdef PBL_COLOR
    if (glucose <= config->urgent_low) return GColorRed;
    if (glucose <= config->low_threshold) return GColorRed;
    if (glucose >= config->high_threshold + 60) return GColorRed;
    if (glucose >= config->high_threshold) return GColorOrange;
    return GColorGreen;
#else
    (void)glucose;
    return (config->color_scheme == COLOR_SCHEME_LIGHT) ? GColorBlack : GColorWhite;
#endif
}

#ifndef PBL_COLOR
typedef enum { TRIO_BW_LINE_OK = 0, TRIO_BW_LINE_HIGH = 1, TRIO_BW_LINE_LOW = 2 } TrioBwLineKind;

static TrioBwLineKind glucose_bw_line_kind(int glucose, TrioConfig *config) {
    if (glucose <= 0) {
        return TRIO_BW_LINE_OK;
    }
    if (glucose <= config->urgent_low || glucose <= config->low_threshold) {
        return TRIO_BW_LINE_LOW;
    }
    if (glucose >= config->high_threshold + 60 || glucose >= config->high_threshold) {
        return TRIO_BW_LINE_HIGH;
    }
    return TRIO_BW_LINE_OK;
}

static int glucose_bw_stroke(TrioBwLineKind k) {
    switch (k) {
        case TRIO_BW_LINE_LOW: return 2;
        case TRIO_BW_LINE_HIGH: return 1;
        default: return 1;
    }
}
#endif

static GColor bg_color(TrioConfig *config) {
    switch (config->color_scheme) {
        case COLOR_SCHEME_LIGHT: return GColorWhite;
        case COLOR_SCHEME_HIGH_CONTRAST: return GColorBlack;
        default: return GColorBlack;
    }
}

static GColor grid_color(TrioConfig *config) {
#ifdef PBL_COLOR
    switch (config->color_scheme) {
        case COLOR_SCHEME_LIGHT: return GColorLightGray;
        case COLOR_SCHEME_HIGH_CONTRAST: return GColorDarkGray;
        default: return GColorDarkGray;
    }
#else
    return (config->color_scheme == COLOR_SCHEME_LIGHT) ? GColorBlack : GColorWhite;
#endif
}

static void draw_horizontal_value_grid(GContext *ctx, int w, int h, TrioConfig *config, bool dim_on_sky) {
    static const int16_t levels[] = { 100, 150, 200, 250, 300 };
    GColor gc = grid_color(config);
    unsigned int gi;
    int g, y, x;
#ifdef PBL_COLOR
    if (dim_on_sky) {
        gc = GColorBlack;
    }
#else
    (void)dim_on_sky;
#endif
    graphics_context_set_stroke_color(ctx, gc);
    for (gi = 0; gi < sizeof(levels) / sizeof(levels[0]); gi++) {
        g = levels[gi];
        if (g < GRAPH_MIN || g > GRAPH_MAX) continue;
        y = map_y(g, h);
        for (x = 0; x < w; x += 5) {
            graphics_draw_pixel(ctx, GPoint(x, y));
            if (x + 2 < w) {
                graphics_draw_pixel(ctx, GPoint(x + 1, y));
            }
        }
    }
}

void graph_draw(Layer *layer, GContext *ctx, TrioConfig *config) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;

    AppState *app = app_state_get();
    bool use_weather_bg = false;
#if TRIO_DISPLAY_COLOR
    use_weather_bg = config->weather_enabled && app->comp.weather_icon[0] != '\0'
        && strcmp(app->comp.weather_icon, "off") != 0;
#endif

    if (use_weather_bg) {
        weather_background_draw(ctx, bounds, &app->comp, config);
    } else {
        graphics_context_set_fill_color(ctx, bg_color(config));
        graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    }

    // Target range band
    int y_high = map_y(config->high_threshold, h);
    int y_low  = map_y(config->low_threshold, h);

#ifdef PBL_COLOR
    GColor range_color;
    switch (config->color_scheme) {
        case COLOR_SCHEME_LIGHT: range_color = GColorMintGreen; break;
        case COLOR_SCHEME_HIGH_CONTRAST: range_color = GColorDarkGreen; break;
        default: range_color = GColorIslamicGreen; break;
    }
#endif
    int band_h = y_low - y_high;
    if (band_h > 0) {
#ifdef PBL_COLOR
        if (use_weather_bg) {
            graphics_context_set_fill_color(ctx, GColorIslamicGreen);
        } else {
            graphics_context_set_fill_color(ctx, range_color);
        }
        graphics_fill_rect(ctx, GRect(0, y_high, w, band_h), 0, GCornerNone);
#else
        /* Original Pebble: avoid gray fills (1-bit dither); outline target band. */
        GColor ink = (config->color_scheme == COLOR_SCHEME_LIGHT) ? GColorBlack : GColorWhite;
        graphics_context_set_stroke_color(ctx, ink);
        graphics_context_set_stroke_width(ctx, 1);
        graphics_draw_rect(ctx, GRect(0, y_high, w, band_h));
#endif
    }

    draw_horizontal_value_grid(ctx, w, h, config, use_weather_bg);

    // Threshold lines (dashed effect via short segments)
    graphics_context_set_stroke_color(ctx, grid_color(config));
    for (int x = 0; x < w; x += 6) {
        graphics_draw_line(ctx, GPoint(x, y_high), GPoint(x + 3, y_high));
        graphics_draw_line(ctx, GPoint(x, y_low), GPoint(x + 3, y_low));
    }

    // Urgent low line
    int y_urgent = map_y(config->urgent_low, h);
#ifdef PBL_COLOR
    graphics_context_set_stroke_color(ctx, GColorRed);
#else
    graphics_context_set_stroke_color(ctx,
        (config->color_scheme == COLOR_SCHEME_LIGHT) ? GColorBlack : GColorWhite);
#endif
    for (int x = 0; x < w; x += 4) {
        graphics_draw_line(ctx, GPoint(x, y_urgent), GPoint(x + 2, y_urgent));
    }

    // Hour markers on X axis
    graphics_context_set_stroke_color(ctx, grid_color(config));
    if (s_count > 12) {
        for (int hr = 12; hr < s_count; hr += 12) {
            int x = (hr * w) / (s_count > 1 ? s_count - 1 : 1);
            for (int y = 0; y < h; y += 4) {
                graphics_draw_pixel(ctx, GPoint(x, y));
                if (y + 1 < h) {
                    graphics_draw_pixel(ctx, GPoint(x, y + 1));
                }
            }
        }
    }

    if (s_count < 2) return;

    int spacing = (s_count > 1) ? w / (s_count - 1) : w;
    if (spacing < 1) spacing = 1;

    // Draw glucose line segments (outline + fill for CGM-style readability on busy backgrounds)
    for (int i = 1; i < s_count; i++) {
        int x0 = (i - 1) * spacing;
        int y0 = map_y(s_values[i - 1], h);
        int x1 = i * spacing;
        int y1 = map_y(s_values[i], h);

        GColor seg = glucose_color(s_values[i], config);
#ifdef PBL_COLOR
        if (use_weather_bg) {
            graphics_context_set_stroke_color(ctx, GColorBlack);
            graphics_context_set_stroke_width(ctx, 2);
            graphics_draw_line(ctx, GPoint(x0, y0), GPoint(x1, y1));
        }
        graphics_context_set_stroke_color(ctx, seg);
        graphics_context_set_stroke_width(ctx, 1);
        graphics_draw_line(ctx, GPoint(x0, y0), GPoint(x1, y1));
#else
        {
            TrioBwLineKind k0 = glucose_bw_line_kind(s_values[i - 1], config);
            TrioBwLineKind k1 = glucose_bw_line_kind(s_values[i], config);
            TrioBwLineKind k = (k0 > k1) ? k0 : k1;
            graphics_context_set_stroke_color(ctx, seg);
            graphics_context_set_stroke_width(ctx, glucose_bw_stroke(k));
            graphics_draw_line(ctx, GPoint(x0, y0), GPoint(x1, y1));
        }
#endif
    }

    /* Dots at every sample made the trace look like a thick ribbon; draw current reading only. */
    for (int i = 0; i < s_count; i++) {
        if (s_count > 1 && i != s_count - 1) {
            continue;
        }
        int x = i * spacing;
        int y = map_y(s_values[i], h);
        GColor dot = glucose_color(s_values[i], config);
#ifdef PBL_COLOR
        if (use_weather_bg) {
            graphics_context_set_fill_color(ctx, GColorBlack);
            graphics_fill_circle(ctx, GPoint(x, y), 3);
        }
        graphics_context_set_fill_color(ctx, dot);
        graphics_fill_circle(ctx, GPoint(x, y), 2);
#else
        graphics_context_set_fill_color(ctx, dot);
        graphics_fill_circle(ctx, GPoint(x, y), 2);
#endif
    }

    // Draw predictions (dashed, different style)
    if (s_pred_count >= 2) {
        int pred_start_x = (s_count > 0) ? (s_count - 1) * spacing : 0;
        int pred_spacing = (w - pred_start_x) / (s_pred_count > 1 ? s_pred_count - 1 : 1);
        if (pred_spacing < 1) pred_spacing = 1;

#ifdef PBL_COLOR
        graphics_context_set_stroke_color(ctx, GColorCyan);
#else
        graphics_context_set_stroke_color(ctx,
            (config->color_scheme == COLOR_SCHEME_LIGHT) ? GColorBlack : GColorWhite);
#endif
        graphics_context_set_stroke_width(ctx, 1);

        for (int i = 1; i < s_pred_count; i++) {
            int x0 = pred_start_x + (i - 1) * pred_spacing;
            int y0 = map_y(s_predictions[i - 1], h);
            int x1 = pred_start_x + i * pred_spacing;
            int y1 = map_y(s_predictions[i], h);
            // Dashed: draw every other segment
            if (i % 2 == 0) {
                graphics_draw_line(ctx, GPoint(x0, y0), GPoint(x1, y1));
            }
        }
    }

    // Current value indicator line (rightmost point, horizontal)
    if (s_count > 0) {
        int last_y = map_y(s_values[s_count - 1], h);
        GColor last_color = glucose_color(s_values[s_count - 1], config);
        graphics_context_set_stroke_color(ctx, last_color);
        graphics_context_set_stroke_width(ctx, 1);
        graphics_draw_line(ctx, GPoint(w - 20, last_y), GPoint(w, last_y));
    }
}
