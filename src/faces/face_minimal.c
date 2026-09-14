// Face: Minimal
// Massive full-width TIME + BG with trend arrow between them.
// Uses custom fonts (Liberation Sans Bold @ 68 / 90 px).

#include "face_minimal.h"
#include "../modules/glucose_format.h"
#include "../modules/platform_compat.h"
#include "../modules/time_display.h"
#include "../modules/trend_glyphs.h"
#include <stdio.h>

static TextLayer *s_time, *s_glucose;
static Layer *s_trend_layer;
static GFont s_font_time, s_font_glucose;
static char s_time_buf[16], s_glucose_buf[16];

static TextLayer *make_text(Layer *root, GRect frame, GFont font, GTextAlignment align, GColor fg) {
    TextLayer *tl = text_layer_create(frame);
    text_layer_set_background_color(tl, GColorClear);
    text_layer_set_text_color(tl, fg);
    text_layer_set_font(tl, font);
    text_layer_set_text_alignment(tl, align);
    text_layer_set_overflow_mode(tl, GTextOverflowModeFill);
    layer_add_child(root, text_layer_get_layer(tl));
    return tl;
}

static GFont load_custom_or_system(uint32_t resource_id, const char *fallback_key) {
    ResHandle rh = resource_get_handle(resource_id);
    GFont f = fonts_load_custom_font(rh);
    if (f) {
        return f;
    }
    APP_LOG(APP_LOG_LEVEL_ERROR, "[MINIMAL] custom font id=%lu failed — system fallback",
            (unsigned long)resource_id);
    return fonts_get_system_font(fallback_key);
}

void face_minimal_load(Window *window, Layer *root, GRect bounds) {
    (void)window;
    int w = bounds.size.w;
    int h = bounds.size.h;
    bool light = config_get()->color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;
    bool large = trio_large_rect(bounds);

    /*
     * Full-bleed hero layout (Emery 200x228 target):
     *
     *   ┌──────────────────────────┐
     *   │         12:34            │  ~68px custom, nearly full width
     *   │           ▲              │  trend glyph
     *   │          142             │  ~90px custom, nearly full width
     *   └──────────────────────────┘
     *
     * Band math leaves max vertical room for the two numbers while keeping
     * the trend readable between them.
     */
    int pad_x = large ? 2 : 1;
    int pad_y = large ? 2 : 1;

    int trend_sz = large ? 40 : 28;
    int gap = large ? 2 : 1;

    /* Prefer giving BG a bit more height than time (glucose is the hero). */
    int reserved_mid = trend_sz + 2 * gap;
    int usable = h - 2 * pad_y - reserved_mid;
    if (usable < 80) {
        usable = h - 2 * pad_y;
        trend_sz = large ? 28 : 22;
        reserved_mid = trend_sz + 2 * gap;
        usable = h - 2 * pad_y - reserved_mid;
    }

    int time_band_h = (usable * 42) / 100; /* ~42% of remaining for clock */
    int bg_band_h = usable - time_band_h;  /* rest for BG */

    /* Font frame heights: leave a couple px so glyphs aren't clipped. */
    int time_font_h = time_band_h - 2;
    int bg_font_h = bg_band_h - 2;
    if (time_font_h < 36) time_font_h = 36;
    if (bg_font_h < 42) bg_font_h = 42;

    int time_y = pad_y + (time_band_h - time_font_h) / 2;
    int trend_y = pad_y + time_band_h + gap;
    int bg_band_y = trend_y + trend_sz + gap;
    int bg_y = bg_band_y + (bg_band_h - bg_font_h) / 2;

    /* Clamp if math drifts past bottom bezel */
    if (bg_y + bg_font_h > h - pad_y) {
        bg_y = h - pad_y - bg_font_h;
        if (bg_y < bg_band_y) bg_y = bg_band_y;
    }

    APP_LOG(APP_LOG_LEVEL_INFO,
            "[MINIMAL] %dx%d time_y=%d th=%d trend_y=%d ts=%d bg_y=%d bh=%d",
            w, h, time_y, time_font_h, trend_y, trend_sz, bg_y, bg_font_h);

    /* Custom massive fonts (subsetted in package.json via characterRegex). */
    s_font_time = load_custom_or_system(RESOURCE_ID_FONT_MINIMAL_TIME_68,
                                        FONT_KEY_BITHAM_42_BOLD);
    s_font_glucose = load_custom_or_system(RESOURCE_ID_FONT_MINIMAL_GLUCOSE_90,
                                           FONT_KEY_ROBOTO_BOLD_SUBSET_49);

    s_time = make_text(root,
                       GRect(pad_x, time_y, w - 2 * pad_x, time_font_h + 4),
                       s_font_time, GTextAlignmentCenter, fg);
    text_layer_set_text(s_time, "12:00");

    /* Trend centered under the clock */
    int trend_x = (w - trend_sz) / 2;
    s_trend_layer = layer_create(GRect(trend_x, trend_y, trend_sz, trend_sz));
    layer_set_clips(s_trend_layer, true);
    layer_set_update_proc(s_trend_layer, trio_trend_layer_update_proc);
    layer_add_child(root, s_trend_layer);

    s_glucose = make_text(root,
                          GRect(pad_x, bg_y, w - 2 * pad_x, bg_font_h + 6),
                          s_font_glucose, GTextAlignmentCenter, fg);
    text_layer_set_text(s_glucose, "--");
}

void face_minimal_unload(void) {
    if (s_time) {
        text_layer_destroy(s_time);
        s_time = NULL;
    }
    if (s_glucose) {
        text_layer_destroy(s_glucose);
        s_glucose = NULL;
    }
    if (s_trend_layer) {
        layer_destroy(s_trend_layer);
        s_trend_layer = NULL;
    }
    /* Only unload if they were custom loads (system fonts must not be unloaded).
     * fonts_load_custom_font failure returns system font — unloading those is unsafe.
     * Track via resource attempt: always try unload; SDK no-ops invalid handles poorly,
     * so we only unload when resource handles resolved (non-null custom).
     * Safer approach: compare against system fonts. */
    GFont sys42 = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
    GFont sys49 = fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49);
    if (s_font_time && s_font_time != sys42) {
        fonts_unload_custom_font(s_font_time);
    }
    if (s_font_glucose && s_font_glucose != sys49) {
        fonts_unload_custom_font(s_font_glucose);
    }
    s_font_time = NULL;
    s_font_glucose = NULL;
    trio_trend_glyphs_deinit();
}

void face_minimal_update(AppState *state) {
    if (!state) return;
    if (!s_time || !s_glucose) return;

    time_t now = time(NULL);
    bool light = state->config.color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;

    trio_format_clock(s_time_buf, sizeof(s_time_buf), now, state->config.clock_24h);
    text_layer_set_text(s_time, s_time_buf);
    text_layer_set_text_color(s_time, fg);

    format_glucose_display(s_glucose_buf, sizeof(s_glucose_buf), state->cgm.glucose,
                           state->config.is_mmol);
    text_layer_set_text(s_glucose, s_glucose_buf);
    text_layer_set_text_color(s_glucose, fg);

    trio_trend_layer_set(state->cgm.trend_str, fg,
                         trio_trend_light_background_assets(&state->config));
    if (s_trend_layer) {
        layer_mark_dirty(s_trend_layer);
    }
}
