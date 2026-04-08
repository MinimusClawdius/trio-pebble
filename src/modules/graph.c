#include "graph.h"
#include "glucose_format.h"
#include "platform_compat.h"
#include <string.h>
#include <stdio.h>

static int16_t s_values[MAX_GRAPH_POINTS];
static int s_count = 0;
static int16_t s_predictions[MAX_PREDICTIONS];
static int s_pred_count = 0;

/** Legacy fixed axis (mg/dL) when graph_scale_mode == GRAPH_SCALE_LEGACY. */
#define GRAPH_LEGACY_MIN 40
#define GRAPH_LEGACY_MAX 400

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

static int map_y_sc(int glucose, int h, int g_min, int g_max) {
    if (g_max <= g_min) {
        g_max = g_min + 1;
    }
    int32_t c = glucose;
    if (c < g_min) {
        c = g_min;
    }
    if (c > g_max) {
        c = g_max;
    }
    return (int)(h - (c - g_min) * (int32_t)h / (g_max - g_min));
}

static void threshold_fallback_range(const TrioConfig *cfg, int16_t *g_min, int16_t *g_max) {
    int lo = cfg->low_threshold;
    int hi = cfg->high_threshold;
    int span = hi - lo;
    if (span < 25) {
        span = 25;
    }
    int pad = span / 6;
    if (pad < 12) {
        pad = 12;
    }
    *g_min = (int16_t)(lo - pad);
    *g_max = (int16_t)(hi + pad);
    if (*g_min < 10) {
        *g_min = 10;
    }
    if (*g_max > 500) {
        *g_max = 500;
    }
    if (*g_max <= *g_min) {
        *g_max = (int16_t)(*g_min + 40);
    }
}

static void compute_graph_y_range(const TrioConfig *cfg, int16_t *g_min, int16_t *g_max) {
    switch ((GraphScaleMode)cfg->graph_scale_mode) {
        case GRAPH_SCALE_LEGACY:
            *g_min = GRAPH_LEGACY_MIN;
            *g_max = GRAPH_LEGACY_MAX;
            return;
        case GRAPH_SCALE_THRESHOLDS:
            threshold_fallback_range(cfg, g_min, g_max);
            return;
        case GRAPH_SCALE_AUTO:
        default: {
            int dmin = 32767;
            int dmax = -32768;
            int i;
            for (i = 0; i < s_count; i++) {
                int v = s_values[i];
                if (v > 0) {
                    if (v < dmin) {
                        dmin = v;
                    }
                    if (v > dmax) {
                        dmax = v;
                    }
                }
            }
            for (i = 0; i < s_pred_count; i++) {
                int v = s_predictions[i];
                if (v > 0) {
                    if (v < dmin) {
                        dmin = v;
                    }
                    if (v > dmax) {
                        dmax = v;
                    }
                }
            }
            {
                AppState *st = app_state_get();
                if (st->cgm.glucose > 0) {
                    int v = st->cgm.glucose;
                    if (v < dmin) {
                        dmin = v;
                    }
                    if (v > dmax) {
                        dmax = v;
                    }
                }
            }
            if (dmin > dmax) {
                threshold_fallback_range(cfg, g_min, g_max);
                return;
            }
            {
                int span = dmax - dmin;
                if (span < 1) {
                    span = 1;
                }
                int pad = (span * 15) / 100;
                if (pad < 8) {
                    pad = 8;
                }
                *g_min = (int16_t)(dmin - pad);
                *g_max = (int16_t)(dmax + pad);
                if (*g_min < 10) {
                    *g_min = 10;
                }
                if (*g_max > 500) {
                    *g_max = 500;
                }
                if (*g_max <= *g_min) {
                    *g_max = (int16_t)(*g_min + 20);
                }
            }
            return;
        }
    }
}

static GColor bg_color(TrioConfig *config) {
    switch (config->color_scheme) {
        case COLOR_SCHEME_LIGHT: return GColorWhite;
        case COLOR_SCHEME_HIGH_CONTRAST: return GColorBlack;
        default: return GColorBlack;
    }
}

/** Horizontal space reserved for threshold text; dotted lines start to the right. */
#define GRAPH_THRESHOLD_LABEL_RESERVE 44

static GColor graph_panel_bg(TrioConfig *config) {
    if (trio_classic_chrome_active(config)) {
        return trio_classic_light_pills(config) ? GColorWhite : GColorBlack;
    }
    return bg_color(config);
}

static GColor graph_ink(TrioConfig *config) {
    if (trio_classic_chrome_active(config)) {
        return trio_classic_light_pills(config) ? GColorBlack : GColorWhite;
    }
    return (config->color_scheme == COLOR_SCHEME_LIGHT) ? GColorBlack : GColorWhite;
}

static int graph_x_for_point(int i, int w, int count) {
    if (count <= 1) {
        return w / 2;
    }
    return i * (w - 1) / (count - 1);
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

    /* Keep baseline band fully above the dashed threshold (Gothic ~14px cap height). */
    int hi_y = y_hi - 16;
    if (hi_y < 0) {
        hi_y = 0;
    }
    if (hi_y + 14 > y_hi && y_hi > 18) {
        hi_y = y_hi - 18;
    }
    GRect r_hi = GRect(2, hi_y, GRAPH_THRESHOLD_LABEL_RESERVE - 4, 12);
    graphics_draw_text(ctx, hi_lbl, f, r_hi, GTextOverflowModeFill, GTextAlignmentLeft, NULL);

    int lo_y = y_lo + 3;
    if (lo_y + 12 > h) {
        lo_y = h - 12;
    }
    if (lo_y <= y_lo) {
        lo_y = y_lo + 2;
    }
    GRect r_lo = GRect(2, lo_y, GRAPH_THRESHOLD_LABEL_RESERVE - 4, 12);
    graphics_draw_text(ctx, lo_lbl, f, r_lo, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
}

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

    int16_t g_min, g_max;
    compute_graph_y_range(config, &g_min, &g_max);

    graphics_context_set_fill_color(ctx, graph_panel_bg(config));
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    int y_high = map_y_sc(config->high_threshold, h, g_min, g_max);
    int y_low = map_y_sc(config->low_threshold, h, g_min, g_max);
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
    graphics_context_set_fill_color(ctx, GColorYellow);
    graphics_fill_rect(ctx, GRect(0, 0, w, y_high), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, GColorMintGreen);
    graphics_fill_rect(ctx, GRect(0, y_high, w, y_low - y_high), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, GColorRed);
    graphics_fill_rect(ctx, GRect(0, y_low, w, h - y_low), 0, GCornerNone);
#endif

    GColor ink = graph_ink(config);
#ifdef PBL_COLOR
    GColor dash = GColorDarkGray;
#else
    GColor dash = ink;
#endif
    draw_dotted_horizontal(ctx, y_high, GRAPH_THRESHOLD_LABEL_RESERVE, w, h, dash);
    draw_dotted_horizontal(ctx, y_low, GRAPH_THRESHOLD_LABEL_RESERVE, w, h, dash);
    draw_graph_threshold_labels(ctx, config, w, h, y_high, y_low);

    if (s_count < 2) {
        if (s_count == 1) {
            int x = graph_x_for_point(0, w, s_count);
            int y = map_y_sc(s_values[0], h, g_min, g_max);
            draw_endpoint_hollow(ctx, GPoint(x, y), 4, graph_panel_bg(config), ink);
        }
        goto draw_predictions;
    }

    graphics_context_set_stroke_color(ctx, ink);
    graphics_context_set_stroke_width(ctx, 2);
    for (int i = 1; i < s_count; i++) {
        int x0 = graph_x_for_point(i - 1, w, s_count);
        int y0 = map_y_sc(s_values[i - 1], h, g_min, g_max);
        int x1 = graph_x_for_point(i, w, s_count);
        int y1 = map_y_sc(s_values[i], h, g_min, g_max);
        graphics_draw_line(ctx, GPoint(x0, y0), GPoint(x1, y1));
    }

    for (int i = 0; i < s_count; i++) {
        int x = graph_x_for_point(i, w, s_count);
        int y = map_y_sc(s_values[i], h, g_min, g_max);
        if (i == s_count - 1) {
            draw_endpoint_hollow(ctx, GPoint(x, y), 4, graph_panel_bg(config), ink);
        } else {
            graphics_context_set_fill_color(ctx, ink);
            graphics_fill_circle(ctx, GPoint(x, y), 3);
        }
    }

draw_predictions:
    if (s_pred_count < 2) {
        return;
    }

    int pred_start_x = (s_count > 0) ? graph_x_for_point(s_count - 1, w, s_count) : w / 2;
    int pred_span = w - pred_start_x;
    if (pred_span < 4) {
        pred_span = 4;
    }
    int pred_spacing = pred_span / (s_pred_count > 1 ? s_pred_count - 1 : 1);
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
        int y0 = map_y_sc(s_predictions[i - 1], h, g_min, g_max);
        int x1 = pred_start_x + i * pred_spacing;
        int y1 = map_y_sc(s_predictions[i], h, g_min, g_max);
        graphics_draw_line(ctx, GPoint(x0, y0), GPoint(x1, y1));
    }
}
