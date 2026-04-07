// Face: Compact
// Two-row data + graph. Similar to T1000 design.
// Row 1: large glucose + trend arrow
// Row 2: delta, time since reading
// Graph fills remaining space with clear threshold lines.

#include "face_compact.h"
#include "../modules/graph.h"
#include "../modules/glucose_format.h"
#include "../modules/platform_compat.h"

static TextLayer *s_glucose, *s_trend, *s_delta, *s_age, *s_time;
static Layer *s_graph_layer;
static char s_time_buf[8], s_glucose_buf[16], s_age_buf[16];

static void graph_proc(Layer *layer, GContext *ctx) {
    graph_draw(layer, ctx, config_get());
}

static TextLayer *make_text(Layer *root, GRect frame, const char *font_key, GTextAlignment align, GColor fg) {
    TextLayer *tl = text_layer_create(frame);
    text_layer_set_background_color(tl, GColorClear);
    text_layer_set_text_color(tl, fg);
    text_layer_set_font(tl, fonts_get_system_font(font_key));
    text_layer_set_text_alignment(tl, align);
    layer_add_child(root, text_layer_get_layer(tl));
    return tl;
}

void face_compact_load(Window *window, Layer *root, GRect bounds) {
    (void)window;
    int w = bounds.size.w;
    int h = bounds.size.h;
    bool light = config_get()->color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;
    GColor fg2 = trio_secondary_fg(config_get());

    {
        s_glucose = make_text(root, GRect(4, -4, 56, 42), FONT_KEY_BITHAM_34_MEDIUM_NUMBERS, GTextAlignmentLeft, fg);
        text_layer_set_text(s_glucose, "--");
        s_trend = make_text(root, GRect(56, 0, w - 60, 44), FONT_KEY_GOTHIC_28_BOLD, GTextAlignmentLeft, fg);
    }

    s_delta = make_text(root, GRect(0, 40, w / 3, 16), FONT_KEY_GOTHIC_14, GTextAlignmentCenter, fg2);
    s_age = make_text(root, GRect(w / 3, 40, w / 3, 16), FONT_KEY_GOTHIC_14, GTextAlignmentCenter, fg2);
    s_time = make_text(root, GRect(2 * w / 3, 36, w / 3 - 2, 24), FONT_KEY_BITHAM_30_BLACK, GTextAlignmentCenter, fg2);

    // Graph - fill the rest
    int graph_top = 58;
    s_graph_layer = layer_create(trio_graph_layer_bounds(bounds, graph_top, h - graph_top - 2));
    layer_set_update_proc(s_graph_layer, graph_proc);
    layer_add_child(root, s_graph_layer);
}

void face_compact_unload(void) {
    text_layer_destroy(s_glucose);
    text_layer_destroy(s_trend);
    text_layer_destroy(s_delta);
    text_layer_destroy(s_age);
    text_layer_destroy(s_time);
    layer_destroy(s_graph_layer);
}

void face_compact_update(AppState *state) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(s_time_buf, sizeof(s_time_buf), "%H:%M", t);
    text_layer_set_text(s_time, s_time_buf);

    format_glucose_display(s_glucose_buf, sizeof(s_glucose_buf), state->cgm.glucose,
                           state->config.is_mmol);
    text_layer_set_text(s_glucose, s_glucose_buf);

#ifdef PBL_COLOR
    if (state->cgm.glucose > 0) {
        TrioConfig *cfg = &state->config;
        GColor gc;
        if (state->cgm.glucose <= cfg->low_threshold) gc = GColorRed;
        else if (state->cgm.glucose >= cfg->high_threshold) gc = GColorOrange;
        else gc = GColorGreen;
        text_layer_set_text_color(s_glucose, gc);
        text_layer_set_text_color(s_trend, gc);
    }
#endif

    text_layer_set_text(s_trend, state->cgm.trend_str);
    text_layer_set_text(s_delta, state->cgm.delta_str);

    // Reading age
    if (state->cgm.last_reading_time > 0) {
        int age_min = (int)(now - state->cgm.last_reading_time) / 60;
        snprintf(s_age_buf, sizeof(s_age_buf), "%dm ago", age_min);
    } else {
        snprintf(s_age_buf, sizeof(s_age_buf), "--");
    }
    text_layer_set_text(s_age, s_age_buf);

    layer_mark_dirty(s_graph_layer);
}
