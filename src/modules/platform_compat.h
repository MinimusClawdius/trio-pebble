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
