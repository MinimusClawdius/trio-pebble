#include "trend_glyphs.h"
#include "config.h"
#include "platform_compat.h"
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

/**
 * Pebble's graphics_draw_bitmap_in_rect() scales some 1-bit assets by tiling on certain
 * platforms — looks like a 2×2 grid. Nearest-neighbor rasterization avoids that.
 */
static bool trend_bitmap_pixel_on(const GBitmap *bmp, int x, int y) {
    GRect bi = gbitmap_get_bounds(bmp);
    if (x < 0 || y < 0 || x >= bi.size.w || y >= bi.size.h) {
        return false;
    }
    const uint8_t *data = (const uint8_t *)gbitmap_get_data(bmp);
    int row = gbitmap_get_bytes_per_row(bmp);
    GBitmapFormat fmt = gbitmap_get_format(bmp);

    switch (fmt) {
        case GBitmapFormat1Bit:
        case GBitmapFormat1BitPalette: {
            uint8_t byte = data[y * row + x / 8];
            return (byte >> (7 - (x % 8))) & 1;
        }
        case GBitmapFormat8Bit:
            return data[y * row + x] != 0;
        default:
            return false;
    }
}

static bool trend_bitmap_nn_supported(const GBitmap *bmp) {
    switch (gbitmap_get_format(bmp)) {
        case GBitmapFormat1Bit:
        case GBitmapFormat1BitPalette:
        case GBitmapFormat8Bit:
            return true;
        default:
            return false;
    }
}

static void draw_bitmap_nn_scaled(GContext *ctx, const GBitmap *bmp, GRect container) {
    GRect bi = gbitmap_get_bounds(bmp);
    int sw = bi.size.w;
    int sh = bi.size.h;
    if (sw < 1 || sh < 1) {
        return;
    }

    int dw = sw * 17 / 10;
    int dh = sh * 17 / 10;
    if (dw > container.size.w && sw > 0) {
        dh = dh * container.size.w / dw;
        dw = container.size.w;
    }
    if (dh > container.size.h && dh > 0) {
        dw = dw * container.size.h / dh;
        dh = container.size.h;
    }
    if (dw < 1) {
        dw = 1;
    }
    if (dh < 1) {
        dh = 1;
    }

    int ox = container.origin.x + (container.size.w - dw) / 2;
    int oy = container.origin.y + (container.size.h - dh) / 2;

    graphics_context_set_fill_color(ctx, s_trend_ink);
    for (int y = 0; y < dh; y++) {
        int sy = (y * sh + dh / 2) / dh;
        if (sy >= sh) {
            sy = sh - 1;
        }
        int x = 0;
        while (x < dw) {
            int sx = (x * sw + dw / 2) / dw;
            if (sx >= sw) {
                sx = sw - 1;
            }
            if (!trend_bitmap_pixel_on(bmp, sx, sy)) {
                x++;
                continue;
            }
            int x0 = x;
            x++;
            while (x < dw) {
                int sx2 = (x * sw + dw / 2) / dw;
                if (sx2 >= sw) {
                    sx2 = sw - 1;
                }
                if (!trend_bitmap_pixel_on(bmp, sx2, sy)) {
                    break;
                }
                x++;
            }
            graphics_fill_rect(ctx, GRect(ox + x0, oy + y, x - x0, 1), 0, GCornerNone);
        }
    }
}

static void draw_trend_bitmap_safe(GContext *ctx, const GBitmap *bmp, GRect bounds) {
    if (trend_bitmap_nn_supported(bmp)) {
        draw_bitmap_nn_scaled(ctx, bmp, bounds);
        return;
    }
    GRect bi = gbitmap_get_bounds(bmp);
    int bw = bi.size.w;
    int bh = bi.size.h;
    int dx = bounds.origin.x + (bounds.size.w - bw) / 2;
    int dy = bounds.origin.y + (bounds.size.h - bh) / 2;
    graphics_draw_bitmap_in_rect(ctx, bmp, GRect(dx, dy, bw, bh));
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
    {
        TrioConfig *cfg = config_get();
        GColor bg;
        if (trio_classic_chrome_active(cfg)) {
            bg = GColorWhite;
        } else {
            switch (cfg->color_scheme) {
                case COLOR_SCHEME_LIGHT:
                    bg = GColorWhite;
                    break;
                case COLOR_SCHEME_HIGH_CONTRAST:
                    bg = GColorBlack;
                    break;
                default:
                    bg = GColorBlack;
                    break;
            }
        }
        graphics_context_set_fill_color(ctx, bg);
        graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    }

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
        draw_trend_bitmap_safe(ctx, s_trend_bmp, bounds);
        return;
    }

    if (k == TG_TEXT && utf8 && utf8[0]) {
        graphics_context_set_text_color(ctx, s_trend_ink);
        GFont f = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
        graphics_draw_text(ctx, utf8, f, bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }
}

void trio_trend_layer_update_proc(Layer *layer, GContext *ctx) {
    GRect b = layer_get_bounds(layer);
    trio_trend_glyph_draw(ctx, b, s_trend_utf8, s_trend_ink);
}
