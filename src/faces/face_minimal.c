// Face: Minimal
// Two-hero layout only: very large TIME + very large BG.
// No trend, delta, sparkline, or complications chrome on this face.

#include "face_minimal.h"
#include "../modules/glucose_format.h"
#include "../modules/platform_compat.h"
#include "../modules/time_display.h"
#include <stdio.h>

static TextLayer *s_time, *s_glucose;
static char s_time_buf[16], s_glucose_buf[16];

static TextLayer *make_text(Layer *root, GRect frame, const char *font_key, GTextAlignment align,
                             GColor fg) {
    TextLayer *tl = text_layer_create(frame);
    text_layer_set_background_color(tl, GColorClear);
    text_layer_set_text_color(tl, fg);
    text_layer_set_font(tl, fonts_get_system_font(font_key));
    text_layer_set_text_alignment(tl, align);
    /* Fill mode: allow large glyphs; avoid ellipsis on 3-digit BG / HH:MM */
    text_layer_set_overflow_mode(tl, GTextOverflowModeFill);
    layer_add_child(root, text_layer_get_layer(tl));
    return tl;
}

/** Largest practical clock font (includes ':' ). */
static const char *minimal_time_font(GRect bounds) {
    (void)bounds;
    /* Bitham 42 is the largest system face with colon support. */
    return FONT_KEY_BITHAM_42_BOLD;
}

/**
 * Largest practical glucose font.
 * ROBOTO_BOLD_SUBSET_49 is ~49px numbers-only (great for mg/dL).
 * mmol needs '.' so fall back to Bitham 42.
 */
static const char *minimal_glucose_font(bool is_mmol, GRect bounds) {
    (void)bounds;
    if (is_mmol) {
        return FONT_KEY_BITHAM_42_BOLD;
    }
    return FONT_KEY_ROBOTO_BOLD_SUBSET_49;
}

void face_minimal_load(Window *window, Layer *root, GRect bounds) {
    (void)window;
    int w = bounds.size.w;
    int h = bounds.size.h;
    bool light = config_get()->color_scheme == COLOR_SCHEME_LIGHT;
    GColor fg = light ? GColorBlack : GColorWhite;
    bool large = trio_large_rect(bounds);
    bool is_mmol = config_get()->is_mmol;

    /*
     * Emery 200x228 example (approx):
     *
     *   y=0  ─────────────────────────
     *        TIME band  (~48% of height)
     *        Bitham 42 clock centered
     *   mid  ─────────────────────────
     *        BG band    (~52% of height)
     *        Roboto 49 / Bitham 42 BG
     *   y=h  ─────────────────────────
     *
     * No other chrome — maximizes glyph room.
     */
    int pad_x = large ? 4 : 2;
    int gap = large ? 4 : 2;

    /* Split: time gets slightly less than half; BG gets the rest (numbers read larger). */
    int time_band_h = (h * 48) / 100;
    int bg_band_y = time_band_h + gap;
    int bg_band_h = h - bg_band_y - (large ? 4 : 2);
    if (bg_band_h < 48) {
        bg_band_h = 48;
        bg_band_y = h - bg_band_h - 2;
        time_band_h = bg_band_y - gap;
    }

    /* Font metrics (approx): Bitham42 ~44–48px tall, Roboto49 ~52–56px.
     * Vertical-center each label inside its band so they don't hug bezels. */
    int time_font_h = large ? 52 : 46;
    int bg_font_h = is_mmol ? (large ? 52 : 46) : (large ? 58 : 52);

    if (time_font_h > time_band_h - 4) {
        time_font_h = time_band_h - 4;
    }
    if (bg_font_h > bg_band_h - 4) {
        bg_font_h = bg_band_h - 4;
    }

    int time_y = (time_band_h - time_font_h) / 2;
    if (time_y < 2) time_y = 2;

    int bg_y = bg_band_y + (bg_band_h - bg_font_h) / 2;
    if (bg_y < bg_band_y) bg_y = bg_band_y;

    APP_LOG(APP_LOG_LEVEL_INFO,
            "[MINIMAL] bounds=%dx%d time_band_h=%d bg_y=%d bg_h=%d time_font_h=%d bg_font_h=%d mmol=%d",
            w, h, time_band_h, bg_y, bg_band_h, time_font_h, bg_font_h, (int)is_mmol);

    s_time = make_text(root,
                       GRect(pad_x, time_y, w - 2 * pad_x, time_font_h),
                       minimal_time_font(bounds),
                       GTextAlignmentCenter, fg);
    text_layer_set_text(s_time, "12:00");

    s_glucose = make_text(root,
                          GRect(pad_x, bg_y, w - 2 * pad_x, bg_font_h),
                          minimal_glucose_font(is_mmol, bounds),
                          GTextAlignmentCenter, fg);
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

    /* If units changed at runtime, swap glucose font (mmol needs decimal glyph). */
    static bool s_last_mmol = false;
    static bool s_init = false;
    if (!s_init || s_last_mmol != state->config.is_mmol) {
        Layer *gl = text_layer_get_layer(s_glucose);
        GRect frame = layer_get_frame(gl);
        text_layer_set_font(s_glucose,
                            fonts_get_system_font(minimal_glucose_font(state->config.is_mmol, frame)));
        s_last_mmol = state->config.is_mmol;
        s_init = true;
    }
}
