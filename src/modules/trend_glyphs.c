#include "trend_glyphs.h"
#include <string.h>

static char s_trend_utf8[16];
static GColor s_trend_ink;
static bool s_light_color_scheme;

static GBitmap *s_trend_bmp;
static char s_cached_utf8[16] = "";
static bool s_cached_light = true;

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

static uint32_t resource_for_trend(TrendGlyphKind k, bool light_bg) {
    if (light_bg) {
        switch (k) {
            case TG_RIGHT: return RESOURCE_ID_TRIO_TREND_FLAT_BLACK;
            case TG_UP: return RESOURCE_ID_TRIO_TREND_UP_BLACK;
            case TG_DOWN: return RESOURCE_ID_TRIO_TREND_DOWN_BLACK;
            case TG_UR: return RESOURCE_ID_TRIO_TREND_UP45_BLACK;
            case TG_DR: return RESOURCE_ID_TRIO_TREND_DOWN45_BLACK;
            case TG_UU: return RESOURCE_ID_TRIO_TREND_DOUBLE_UP_BLACK;
            case TG_DD: return RESOURCE_ID_TRIO_TREND_DOUBLE_DOWN_BLACK;
            case TG_TEXT:
            default: return RESOURCE_ID_TRIO_TREND_NONE_BLACK;
        }
    }
    switch (k) {
        case TG_RIGHT: return RESOURCE_ID_TRIO_TREND_FLAT;
        case TG_UP: return RESOURCE_ID_TRIO_TREND_UP;
        case TG_DOWN: return RESOURCE_ID_TRIO_TREND_DOWN;
        case TG_UR: return RESOURCE_ID_TRIO_TREND_UP45;
        case TG_DR: return RESOURCE_ID_TRIO_TREND_DOWN45;
        case TG_UU: return RESOURCE_ID_TRIO_TREND_DOUBLE_UP;
        case TG_DD: return RESOURCE_ID_TRIO_TREND_DOUBLE_DOWN;
        case TG_TEXT:
        default: return RESOURCE_ID_TRIO_TREND_NONE;
    }
}

void trio_trend_layer_set(const char *utf8, GColor ink, bool light_color_scheme) {
    char next[16];
    if (utf8) {
        strncpy(next, utf8, sizeof(next) - 1);
        next[sizeof(next) - 1] = '\0';
    } else {
        next[0] = '\0';
    }
    bool changed = (strcmp(next, s_trend_utf8) != 0) || (light_color_scheme != s_light_color_scheme);
    strncpy(s_trend_utf8, next, sizeof(s_trend_utf8) - 1);
    s_trend_utf8[sizeof(s_trend_utf8) - 1] = '\0';
    s_trend_ink = ink;
    s_light_color_scheme = light_color_scheme;
    if (changed) {
        trio_trend_glyphs_deinit();
    }
}

void trio_trend_glyphs_deinit(void) {
    if (s_trend_bmp) {
        gbitmap_destroy(s_trend_bmp);
        s_trend_bmp = NULL;
    }
    s_cached_utf8[0] = '\0';
}

void trio_trend_glyph_draw(GContext *ctx, GRect bounds, const char *utf8, GColor ink) {
    (void)ink;
    TrendGlyphKind k = classify_trend(utf8);
    bool light = s_light_color_scheme;

    if (!s_trend_bmp) {
        uint32_t rid = resource_for_trend(k, light);
        s_trend_bmp = gbitmap_create_with_resource(rid);
        strncpy(s_cached_utf8, utf8 ? utf8 : "", sizeof(s_cached_utf8) - 1);
        s_cached_utf8[sizeof(s_cached_utf8) - 1] = '\0';
        s_cached_light = light;
        if (!s_trend_bmp) {
            APP_LOG(APP_LOG_LEVEL_ERROR, "trend: gbitmap_create failed id=%lu", (unsigned long)rid);
        }
    }

    if (s_trend_bmp) {
        GRect bi = gbitmap_get_bounds(s_trend_bmp);
        int bw = bi.size.w;
        int bh = bi.size.h;
        /* ~1.4× scale, clamped to layer (LOOP-style large arrow). */
        int dw = bw * 14 / 10;
        int dh = bh * 14 / 10;
        if (dw > bounds.size.w && bw > 0) {
            dh = dh * bounds.size.w / dw;
            dw = bounds.size.w;
        }
        if (dh > bounds.size.h && dh > 0) {
            dw = dw * bounds.size.h / dh;
            dh = bounds.size.h;
        }
        int dx = bounds.origin.x + (bounds.size.w - dw) / 2;
        int dy = bounds.origin.y + (bounds.size.h - dh) / 2;
        GRect dest = GRect(dx, dy, dw, dh);
        graphics_draw_bitmap_in_rect(ctx, s_trend_bmp, dest);
        return;
    }

    if (k == TG_TEXT && utf8 && utf8[0]) {
        graphics_context_set_text_color(ctx, s_trend_ink);
        GFont f = fonts_get_system_font(FONT_KEY_GOTHIC_34);
        graphics_draw_text(ctx, utf8, f, bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }
}

void trio_trend_layer_update_proc(Layer *layer, GContext *ctx) {
    GRect b = layer_get_bounds(layer);
    trio_trend_glyph_draw(ctx, b, s_trend_utf8, s_trend_ink);
}
