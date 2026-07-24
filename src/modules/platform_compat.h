#pragma once
#include "../trio_types.h"
#include <pebble.h>

/*
 * Multi-platform PBW: Rebble installs the correct ELF for the paired watch
 * (aplite = original Pebble B&W, basalt/chalk/diorite/emery = color, chalk = round).
 * There is no single runtime "switch" in one binary — autodetection = which build is installed.
 */

#if defined(PBL_COLOR)
#define TRIO_DISPLAY_COLOR 1
#else
#define TRIO_DISPLAY_COLOR 0
#endif

#ifdef PBL_ROUND
#define TRIO_GRAPH_SIDE_INSET 12
#define TRIO_GRAPH_TOP_TRIM 4
#define TRIO_GRAPH_BOTTOM_TRIM 16
#else
#define TRIO_GRAPH_SIDE_INSET 2
#define TRIO_GRAPH_TOP_TRIM 0
#define TRIO_GRAPH_BOTTOM_TRIM 0
#endif

/** Inset frame for graph / sparkline layers (round = keep art off bezel / chin). */
static inline GRect trio_graph_layer_bounds(GRect window_bounds, int top, int height) {
    int w = window_bounds.size.w;
    int x = TRIO_GRAPH_SIDE_INSET;
    int ww = w - 2 * TRIO_GRAPH_SIDE_INSET;
    int y = top + TRIO_GRAPH_TOP_TRIM;
    int hh = height - TRIO_GRAPH_TOP_TRIM - TRIO_GRAPH_BOTTOM_TRIM;
    if (hh < 28) {
        y = top;
        hh = height;
    }
    return GRect(x, y, ww, hh);
}

/** Secondary labels (time, delta): solid B/W on aplite instead of dithered grays. */
static inline GColor trio_secondary_fg(const TrioConfig *cfg) {
#if TRIO_DISPLAY_COLOR
    if (cfg->color_scheme == COLOR_SCHEME_LIGHT) {
        return GColorDarkGray;
    }
    return GColorLightGray;
#else
    if (cfg->color_scheme == COLOR_SCHEME_LIGHT) {
        return GColorBlack;
    }
    return GColorWhite;
#endif
}

/** Classic face: custom chrome (not High Contrast). */
static inline bool trio_classic_chrome_active(const TrioConfig *cfg) {
    return cfg->face_type == FACE_CLASSIC && cfg->color_scheme != COLOR_SCHEME_HIGH_CONTRAST;
}

/** Light: black header/footer; white rounded center card. Dark: inverse (white bars, black card). */
static inline bool trio_classic_light_pills(const TrioConfig *cfg) {
    return cfg->face_type == FACE_CLASSIC && cfg->color_scheme == COLOR_SCHEME_LIGHT;
}

/** Trend PNG set: *_BLACK variants (dark ink on light); invert at raster time. Plain set on dark panels. */
static inline bool trio_trend_light_background_assets(const TrioConfig *cfg) {
    return cfg->color_scheme == COLOR_SCHEME_LIGHT;
}

/** Classic chrome footer: light ink on black strip vs dark ink on white strip. */
static inline bool trio_classic_footer_light_ink(const TrioConfig *cfg) {
    return trio_classic_chrome_active(cfg) && trio_classic_light_pills(cfg);
}

/* ============================================================
 * Emery / large screen (Pebble Time 2) adaptations
 * emery: 200x228 (SDK), classic rect: 144x168. Layouts were
 * originally 144px; use these to prevent cramped/overlapping
 * elements and undersized glyphs on larger hardware.
 * ============================================================ */

/** True on emery and other wide rect screens (>=180px). */
static inline bool trio_large_rect(GRect bounds) {
    return bounds.size.w >= 180;
}

/** Hero area height (glucose + trend) for classic/retro style. */
static inline int trio_hero_height(GRect bounds) {
    return trio_large_rect(bounds) ? 80 : 54;
}

/** Header strip height (time/age row). */
static inline int trio_header_height(GRect bounds) {
    return trio_large_rect(bounds) ? 48 : 28;
}

/** Square size for trend glyph layer (larger on big screens so arrows don't look tiny). */
static inline int trio_trend_size(GRect bounds) {
    if (trio_large_rect(bounds)) {
        return 64;
    }
    return 36;
}

/** Glucose number font — prefer largest bold subset on color hardware. */
static inline const char *trio_glucose_font(bool is_color) {
    if (is_color) {
        return FONT_KEY_ROBOTO_BOLD_SUBSET_49;
    }
    return FONT_KEY_BITHAM_42_BOLD;
}

/** Scale factor hint for manual NN trend bitmap (bigger preferred size on large screens). */
static inline int trio_trend_scale_numer(void) {
    return 20; /* 2.0x base */
}
