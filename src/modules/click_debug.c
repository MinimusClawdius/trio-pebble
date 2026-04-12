#include "click_debug.h"
#include <stdio.h>
#include <string.h>

#define PERSIST_KEY_LOG 0x434C4B47u /* CLKg */
#define PERSIST_KEY_ON  0x434C4442u /* CLDB */

#define CLICK_LOG_LINES 6
#define LINE_CHARS 26

static TextLayer *s_layer;
static char s_lines[CLICK_LOG_LINES][LINE_CHARS + 1];
static bool s_overlay_on;

static void persist_save_lines(void) {
    struct {
        uint8_t magic;
        uint8_t pad[3];
        char lines[CLICK_LOG_LINES][LINE_CHARS + 1];
    } blob;
    memset(&blob, 0, sizeof(blob));
    blob.magic = 0xC1;
    memcpy(blob.lines, s_lines, sizeof(s_lines));
    /* Single persist slot max 256 bytes on all targets. */
    if ((int)sizeof(blob) <= 256) {
        persist_write_data(PERSIST_KEY_LOG, &blob, sizeof(blob));
    }
}

static void persist_load_lines(void) {
    struct {
        uint8_t magic;
        uint8_t pad[3];
        char lines[CLICK_LOG_LINES][LINE_CHARS + 1];
    } blob;
    if (!persist_exists(PERSIST_KEY_LOG)) {
        return;
    }
    if (persist_read_data(PERSIST_KEY_LOG, &blob, sizeof(blob)) != (int)sizeof(blob)) {
        return;
    }
    if (blob.magic != 0xC1) {
        return;
    }
    memcpy(s_lines, blob.lines, sizeof(s_lines));
}

static void refresh_text(void) {
    if (!s_layer) {
        return;
    }
    if (!s_overlay_on) {
        layer_set_hidden(text_layer_get_layer(s_layer), true);
        return;
    }
    layer_set_hidden(text_layer_get_layer(s_layer), false);
    static char buf[CLICK_LOG_LINES * (LINE_CHARS + 2)];
    buf[0] = '\0';
    for (int i = 0; i < CLICK_LOG_LINES; i++) {
        if (s_lines[i][0] == '\0') {
            continue;
        }
        if (buf[0] != '\0') {
            strncat(buf, "\n", sizeof(buf) - strlen(buf) - 1);
        }
        strncat(buf, s_lines[i], sizeof(buf) - strlen(buf) - 1);
    }
    text_layer_set_text(s_layer, buf[0] ? buf : "—");
}

void click_debug_init(void) {
    memset(s_lines, 0, sizeof(s_lines));
    s_overlay_on = persist_exists(PERSIST_KEY_ON) && persist_read_bool(PERSIST_KEY_ON);
    persist_load_lines();
}

void click_debug_attach_to_window(Window *window) {
    s_layer = NULL;
    Layer *root = window_get_root_layer(window);
    GRect b = layer_get_bounds(root);
    const int h = 54;
    s_layer = text_layer_create(GRect(1, b.size.h - h - 1, b.size.w - 2, h));
    text_layer_set_background_color(s_layer, GColorBlack);
    text_layer_set_text_color(s_layer, GColorWhite);
    text_layer_set_font(s_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(s_layer, GTextAlignmentLeft);
    layer_add_child(root, text_layer_get_layer(s_layer));
    refresh_text();
}

void click_debug_detach_window(void) {
    s_layer = NULL;
}

void click_debug_toggle_overlay(void) {
    s_overlay_on = !s_overlay_on;
    persist_write_bool(PERSIST_KEY_ON, s_overlay_on);
    refresh_text();
}

void click_debug_log(const char *msg) {
    char line[LINE_CHARS + 1];
    time_t raw = time(NULL);
    struct tm *tm = localtime(&raw);
    int hh = tm ? tm->tm_hour : 0;
    int mm = tm ? tm->tm_min : 0;

    const char *m = msg ? msg : "?";
    snprintf(line, sizeof(line), "%02d:%02d %.20s", hh, mm, m);

    for (int i = CLICK_LOG_LINES - 1; i > 0; i--) {
        memcpy(s_lines[i], s_lines[i - 1], sizeof(s_lines[0]));
    }
    memcpy(s_lines[0], line, sizeof(line));

    persist_save_lines();
    refresh_text();
}
