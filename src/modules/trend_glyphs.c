#include "trend_glyphs.h"
#include <string.h>

static char s_trend_utf8[16];
static GColor s_trend_ink;

void trio_trend_layer_set(const char *utf8, GColor ink) {
    if (utf8) {
        strncpy(s_trend_utf8, utf8, sizeof(s_trend_utf8) - 1);
        s_trend_utf8[sizeof(s_trend_utf8) - 1] = '\0';
    } else {
        s_trend_utf8[0] = '\0';
    }
    s_trend_ink = ink;
}

typedef enum {
    TG_RIGHT = 0,
    TG_UP,
    TG_DOWN,
    TG_UR,
    TG_DR,
    TG_UU,
    TG_DD,
    TG_TEXT
} TrendGlyphKind;

static TrendGlyphKind classify_trend(const char *s) {
    const unsigned char *u = (const unsigned char *)s;
    if (!s || !*s) {
        return TG_TEXT;
    }
    if (u[0] == 0xe2 && u[1] == 0x86) {
        if (u[2] == 0x92) {
            return TG_RIGHT;
        }
        if (u[2] == 0x91) {
            if (u[3] == 0xe2 && u[4] == 0x86 && u[5] == 0x91) {
                return TG_UU;
            }
            return TG_UP;
        }
        if (u[2] == 0x93) {
            if (u[3] == 0xe2 && u[4] == 0x86 && u[5] == 0x93) {
                return TG_DD;
            }
            return TG_DOWN;
        }
        if (u[2] == 0x97) {
            return TG_UR;
        }
        if (u[2] == 0x98) {
            return TG_DR;
        }
    }
    return TG_TEXT;
}

static void fill_fat_right(GContext *ctx, int cx, int cy, GColor c) {
    graphics_context_set_fill_color(ctx, c);
    graphics_fill_rect(ctx, GRect(cx - 12, cy - 4, 17, 8));
    graphics_fill_rect(ctx, GRect(cx + 4, cy - 8, 6, 6));
    graphics_fill_rect(ctx, GRect(cx + 4, cy + 2, 6, 6));
    graphics_fill_rect(ctx, GRect(cx + 8, cy - 5, 6, 10));
}

static void fill_fat_up(GContext *ctx, int cx, int cy, GColor c) {
    graphics_context_set_fill_color(ctx, c);
    graphics_fill_rect(ctx, GRect(cx - 4, cy - 2, 8, 14));
    graphics_fill_rect(ctx, GRect(cx - 8, cy - 10, 6, 6));
    graphics_fill_rect(ctx, GRect(cx + 2, cy - 10, 6, 6));
    graphics_fill_rect(ctx, GRect(cx - 5, cy - 14, 10, 6));
}

static void fill_fat_down(GContext *ctx, int cx, int cy, GColor c) {
    graphics_context_set_fill_color(ctx, c);
    graphics_fill_rect(ctx, GRect(cx - 4, cy - 12, 8, 14));
    graphics_fill_rect(ctx, GRect(cx - 8, cy + 4, 6, 6));
    graphics_fill_rect(ctx, GRect(cx + 2, cy + 4, 6, 6));
    graphics_fill_rect(ctx, GRect(cx - 5, cy + 8, 10, 6));
}

static void fill_fat_ur(GContext *ctx, int cx, int cy, GColor c) {
    graphics_context_set_fill_color(ctx, c);
    int i;
    for (i = 0; i < 5; i++) {
        graphics_fill_rect(ctx, GRect(cx - 9 + i * 3, cy + 5 - i * 3, 6, 6));
    }
}

static void fill_fat_dr(GContext *ctx, int cx, int cy, GColor c) {
    graphics_context_set_fill_color(ctx, c);
    int i;
    for (i = 0; i < 5; i++) {
        graphics_fill_rect(ctx, GRect(cx - 9 + i * 3, cy - 9 + i * 3, 6, 6));
    }
}

static void fill_fat_uu(GContext *ctx, int cx, int cy, GColor c) {
    fill_fat_up(ctx, cx - 5, cy, c);
    fill_fat_up(ctx, cx + 5, cy, c);
}

static void fill_fat_dd(GContext *ctx, int cx, int cy, GColor c) {
    fill_fat_down(ctx, cx - 5, cy, c);
    fill_fat_down(ctx, cx + 5, cy, c);
}

void trio_trend_glyph_draw(GContext *ctx, GRect bounds, const char *utf8, GColor ink) {
    int cx = bounds.origin.x + bounds.size.w / 2;
    int cy = bounds.origin.y + bounds.size.h / 2;
    TrendGlyphKind k = classify_trend(utf8);

    switch (k) {
        case TG_RIGHT:
            fill_fat_right(ctx, cx, cy, ink);
            return;
        case TG_UP:
            fill_fat_up(ctx, cx, cy, ink);
            return;
        case TG_DOWN:
            fill_fat_down(ctx, cx, cy, ink);
            return;
        case TG_UR:
            fill_fat_ur(ctx, cx, cy, ink);
            return;
        case TG_DR:
            fill_fat_dr(ctx, cx, cy, ink);
            return;
        case TG_UU:
            fill_fat_uu(ctx, cx, cy, ink);
            return;
        case TG_DD:
            fill_fat_dd(ctx, cx, cy, ink);
            return;
        case TG_TEXT:
        default: {
            graphics_context_set_text_color(ctx, ink);
            GFont f = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
            graphics_draw_text(ctx, utf8, f, bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
            return;
        }
    }
}

void trio_trend_layer_update_proc(Layer *layer, GContext *ctx) {
    GRect b = layer_get_bounds(layer);
    trio_trend_glyph_draw(ctx, b, s_trend_utf8, s_trend_ink);
}
