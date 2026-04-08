#include "graph.h"
#include "glucose_format.h"
#include "platform_compat.h"
#include <string.h>
#include <stdio.h>

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

static GColor bg_color(TrioConfig *config) {
    switch (config->color_scheme) {
        case COLOR_SCHEME_LIGHT: return GColorWhite;
        case COLOR_SCHEME_HIGH_CONTRAST: return GColorBlack;
        default: return GColorBlack;
    }
}

/** Trace + labels: high-contrast ink on graph (LOOP spec: black on pastels / B&W). */
static GColor graph_ink(TrioConfig *config) {
    return (config->color_scheme == COLOR_SCHEME_LIGHT) ? GColorBlack : GColorWhite;
}

static int graph_x_for_point(int i, int w, int count) {
    if (count <= 1) {
        return w / 2;
    }
    return i * w / (count - 1);
}

static void draw_dotted_horizontal(GContext *ctx, int y, int x0, int x1, int h, GColor c) {
    if (y < 0 || y >= h) {
        return;
    }
    graphics_context_set_stroke_color(ctx, c);
    graphics_context_set_stroke_width(ctx, 1);
    for (int x = x0; x < x1; x += 5) {
        int xe = x + 2;
        if (xe > x1) {
            xe = x1;
        }
        graphics_draw_line(ctx, GPoint(x, y), GPoint(xe, y));
    }
}

static void draw_graph_threshold_labels(GContext *ctx, TrioConfig *config, int w, int h, int y_hi, int y_lo) {
    char hi_lbl[12], lo_lbl[12];
    format_threshold_label(hi_lbl, sizeof(hi_lbl), config->high_threshold, config->is_mmol);
    format_threshold_label(lo_lbl, sizeof(lo_lbl), config->low_threshold, config->is_mmol);

    GColor ink = graph_ink(config);
    graphics_context_set_text_color(ctx, ink);
    GFont f = fonts_get_system_font(FONT_KEY_GOTHIC_14);

    int hi_y = y_hi - 15;
    if (hi_y < 0) {
        hi_y = 0;
    }
    GRect r_hi = GRect(2, hi_y, 36, 16);
    graphics_draw_text(ctx, hi_lbl, f, r_hi, GTextOverflowModeFill, GTextAlignmentLeft, NULL);

    int lo_y = y_lo + 2;
    if (lo_y > h - 14) {
        lo_y = h - 14;
    }
    GRect r_lo = GRect(2, lo_y, 36, 16);
    graphics_draw_text(ctx, lo_lbl, f, r_lo, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
}

/** Latest point: filled interior + outline (LOOP / Emery spec). */
static void draw_endpoint_hollow(GContext *ctx, GPoint p, int r, GColor inner, GColor edge) {
    graphics_context_set_fill_color(ctx, inner);
    graphics_fill_circle(ctx, p, r);
    graphics_context_set_stroke_color(ctx, edge);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_circle(ctx, p, r);
}

void graph_draw(Layer *layer, GContext *ctx, TrioConfig *config) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;

    /* LOOP target: clean graph (no weather illustration behind trace). */
    graphics_context_set_fill_color(ctx, bg_color(config));
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    int y_high = map_y(config->high_threshold, h);
    int y_low = map_y(config->low_threshold, h);
    if (y_high < 0) {
        y_high = 0;
    }
    if (y_low > h - 1) {
        y_low = h - 1;
    }
    if (y_low < y_high) {
        int t = y_high;
        y_high = y_low;
        y_low = t;
    }

#ifdef PBL_COLOR
    /* Pebble Time 2 / color: muted high / in-range / low bands (no vertical grid). */
    graphics_context_set_fill_color(ctx, GColorVeryLightYellow);
    graphics_fill_rect(ctx, GRect(0, 0, w, y_high), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, GColorMintGreen);
    graphics_fill_rect(ctx, GRect(0, y_high, w, y_low - y_high), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, GColorRoseVale);
    graphics_fill_rect(ctx, GRect(0, y_low, w, h - y_low), 0, GCornerNone);
#endif

    GColor ink = graph_ink(config);
#ifdef PBL_COLOR
    GColor dash = GColorDarkGray;
#else
    GColor dash = ink;
#endif
    draw_dotted_horizontal(ctx, y_high, 0, w, h, dash);
    draw_dotted_horizontal(ctx, y_low, 0, w, h, dash);
    draw_graph_threshold_labels(ctx, config, w, h, y_high, y_low);

    if (s_count < 2) {
        if (s_count == 1) {
            int x = graph_x_for_point(0, w, s_count);
            int y = map_y(s_values[0], h);
            GColor inner = bg_color(config);
#ifdef PBL_COLOR
            inner = GColorWhite;
#endif
            draw_endpoint_hollow(ctx, GPoint(x, y), 4, inner, ink);
        }
        goto draw_predictions;
    }

    int spacing = w / (s_count - 1);
    if (spacing < 1) {
        spacing = 1;
    }

    graphics_context_set_stroke_color(ctx, ink);
    graphics_context_set_stroke_width(ctx, 2);
    for (int i = 1; i < s_count; i++) {
        int x0 = (i - 1) * spacing;
        int y0 = map_y(s_values[i - 1], h);
        int x1 = i * spacing;
        int y1 = map_y(s_values[i], h);
        graphics_draw_line(ctx, GPoint(x0, y0), GPoint(x1, y1));
    }

    for (int i = 0; i < s_count; i++) {
        int x = i * spacing;
        int y = map_y(s_values[i], h);
        if (i == s_count - 1) {
            GColor inner = bg_color(config);
#ifdef PBL_COLOR
            inner = GColorWhite;
#endif
            draw_endpoint_hollow(ctx, GPoint(x, y), 4, inner, ink);
        } else {
            graphics_context_set_fill_color(ctx, ink);
            graphics_fill_circle(ctx, GPoint(x, y), 3);
        }
    }

draw_predictions:
    if (s_pred_count < 2) {
        return;
    }

    int pred_start_x = (s_count > 0) ? graph_x_for_point(s_count - 1, w, s_count) : 0;
    int pred_spacing = (w - pred_start_x) / (s_pred_count > 1 ? s_pred_count - 1 : 1);
    if (pred_spacing < 1) {
        pred_spacing = 1;
    }

#ifdef PBL_COLOR
    graphics_context_set_stroke_color(ctx, GColorCyan);
#else
    graphics_context_set_stroke_color(ctx, ink);
#endif
    graphics_context_set_stroke_width(ctx, 1);

    for (int i = 1; i < s_pred_count; i++) {
        int x0 = pred_start_x + (i - 1) * pred_spacing;
        int y0 = map_y(s_predictions[i - 1], h);
        int x1 = pred_start_x + i * pred_spacing;
        int y1 = map_y(s_predictions[i], h);
        graphics_draw_line(ctx, GPoint(x0, y0), GPoint(x1, y1));
    }
}
