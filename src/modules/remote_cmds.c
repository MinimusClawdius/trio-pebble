#include "remote_cmds.h"
#include "remote_send_ui.h"
#include <stdio.h>
#include <string.h>

static Window *s_watchface;
static Window *s_menu_window;
static Window *s_pick_window;

static SimpleMenuLayer *s_menu_layer;
static SimpleMenuSection s_menu_section;
#define REMOTE_MENU_ITEMS 3
static SimpleMenuItem s_menu_items[REMOTE_MENU_ITEMS];

static TextLayer *s_pick_title;
static TextLayer *s_pick_value;
static TextLayer *s_pick_hint;
/** 1 = bolus (tenths U), 2 = carbs (g) */
static int32_t s_pick_cmd_type;
static int32_t s_pick_amount;

void remote_cmds_send_to_phone(int32_t cmd_type, int32_t amount) {
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
        remote_send_ui_on_prepare_send_failed();
        return;
    }
    dict_write_int32(iter, KEY_CMD_TYPE, cmd_type);
    dict_write_int32(iter, KEY_CMD_AMOUNT, amount);
    app_message_outbox_send();
}

static void picker_refresh_value_text(void) {
    static char buf[24];
    if (s_pick_cmd_type == 1) {
        int v = (int)s_pick_amount;
        snprintf(buf, sizeof(buf), "%d.%d U", v / 10, v % 10);
    } else {
        snprintf(buf, sizeof(buf), "%ld g", (long)s_pick_amount);
    }
    text_layer_set_text(s_pick_value, buf);
}

static void picker_back_handler(ClickRecognizerRef recognizer, void *context) {
    (void)recognizer;
    (void)context;
    window_stack_pop(true);
}

static void picker_select_handler(ClickRecognizerRef recognizer, void *context) {
    (void)recognizer;
    (void)context;
    remote_send_ui_push_confirm(s_pick_cmd_type, s_pick_amount);
}

static void picker_up_handler(ClickRecognizerRef recognizer, void *context) {
    (void)recognizer;
    (void)context;
    if (s_pick_cmd_type == 1) {
        if (s_pick_amount < 300) {
            s_pick_amount++;
        }
    } else {
        if (s_pick_amount < 250) {
            s_pick_amount += 5;
        }
    }
    picker_refresh_value_text();
}

static void picker_down_handler(ClickRecognizerRef recognizer, void *context) {
    (void)recognizer;
    (void)context;
    if (s_pick_cmd_type == 1) {
        if (s_pick_amount > 1) {
            s_pick_amount--;
        }
    } else {
        if (s_pick_amount > 5) {
            s_pick_amount -= 5;
        }
    }
    picker_refresh_value_text();
}

static void picker_click_config(void *context) {
    (void)context;
    window_single_click_subscribe(BUTTON_ID_SELECT, picker_select_handler);
    window_single_click_subscribe(BUTTON_ID_UP, picker_up_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, picker_down_handler);
    window_single_click_subscribe(BUTTON_ID_BACK, picker_back_handler);
}

static void pick_window_load(Window *window) {
    Layer *root = window_get_root_layer(window);
    GRect b = layer_get_bounds(root);

    s_pick_title = text_layer_create(GRect(0, 12, b.size.w, 28));
    text_layer_set_background_color(s_pick_title, GColorClear);
    text_layer_set_text_alignment(s_pick_title, GTextAlignmentCenter);
    text_layer_set_font(s_pick_title, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text(s_pick_title, s_pick_cmd_type == 1 ? "Bolus" : "Carbs");
    layer_add_child(root, text_layer_get_layer(s_pick_title));

    s_pick_value = text_layer_create(GRect(0, 48, b.size.w, 44));
    text_layer_set_background_color(s_pick_value, GColorClear);
    text_layer_set_text_alignment(s_pick_value, GTextAlignmentCenter);
    /* BITHAM_34_MEDIUM_NUMBERS has no "U" / "g" — use Gothic so units render (fixes "XX" tofu). */
    text_layer_set_font(s_pick_value, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    layer_add_child(root, text_layer_get_layer(s_pick_value));

    s_pick_hint = text_layer_create(GRect(8, b.size.h - 44, b.size.w - 16, 40));
    text_layer_set_background_color(s_pick_hint, GColorClear);
    text_layer_set_text_alignment(s_pick_hint, GTextAlignmentCenter);
    text_layer_set_font(s_pick_hint, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text(s_pick_hint, "UP/DOWN adjust\nSELECT review");
    layer_add_child(root, text_layer_get_layer(s_pick_hint));

    window_set_click_config_provider(window, picker_click_config);
    picker_refresh_value_text();
}

static void pick_window_unload(Window *window) {
    (void)window;
    text_layer_destroy(s_pick_title);
    text_layer_destroy(s_pick_value);
    text_layer_destroy(s_pick_hint);
    s_pick_title = s_pick_value = s_pick_hint = NULL;
}

/** Push bolus (1) or carb (2) amount picker with initial amount. */
static void push_amount_picker(int32_t cmd_type, int32_t amount) {
    s_pick_cmd_type = cmd_type;
    s_pick_amount = amount;

    if (!s_pick_window) {
        s_pick_window = window_create();
        window_set_window_handlers(s_pick_window, (WindowHandlers){
            .load = pick_window_load,
            .unload = pick_window_unload,
        });
    }
    window_stack_push(s_pick_window, true);
}

static void open_amount_picker(int32_t cmd_type) {
    if (cmd_type == 1) {
        push_amount_picker(1, 20);
    } else {
        push_amount_picker(2, 15);
    }
}

void remote_cmds_open_bolus_picker_preset(int32_t tenths) {
    if (tenths < 1) {
        return;
    }
    if (s_pick_window && window_stack_contains_window(s_pick_window)) {
        s_pick_cmd_type = 1;
        s_pick_amount = tenths;
        picker_refresh_value_text();
        return;
    }
    push_amount_picker(1, tenths);
}

static void menu_select_cb(int index, void *context) {
    (void)context;
    if (index == 2) {
        window_stack_pop(true);
        return;
    }
    window_stack_pop(true);
    open_amount_picker(index == 0 ? 1 : 2);
}

static void menu_window_load(Window *window) {
    Layer *root = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(root);

    s_menu_items[0] = (SimpleMenuItem){
        .title = "Remote bolus",
        .subtitle = "0.1 U",
        .callback = menu_select_cb,
    };
    s_menu_items[1] = (SimpleMenuItem){
        .title = "Remote carbs",
        .subtitle = "5 g",
        .callback = menu_select_cb,
    };
    s_menu_items[2] = (SimpleMenuItem){
        .title = "Cancel",
        .subtitle = NULL,
        .callback = menu_select_cb,
    };

    s_menu_section = (SimpleMenuSection){
        .title = "Trio remote",
        .num_items = REMOTE_MENU_ITEMS,
        .items = s_menu_items,
    };

    s_menu_layer = simple_menu_layer_create(bounds, window, &s_menu_section, 1, NULL);
    layer_add_child(root, simple_menu_layer_get_layer(s_menu_layer));
}

static void menu_window_unload(Window *window) {
    (void)window;
    if (s_menu_layer) {
        simple_menu_layer_destroy(s_menu_layer);
        s_menu_layer = NULL;
    }
}

void remote_cmds_set_watchface_window(Window *watchface) {
    s_watchface = watchface;
}

void remote_cmds_try_open(AppState *state) {
    if (!s_watchface) {
        APP_LOG(APP_LOG_LEVEL_WARNING, "remote: s_watchface NULL");
        return;
    }
    /* Only block if our menu/picker is already showing. Do NOT require
     * window_stack_get_top_window() == s_watchface: on Pebble 2 (diorite) and some firmware,
     * the top window pointer can differ from the watchface we created even while the face is
     * visible — that check prevented the remote menu from ever opening. */
    if ((s_menu_window && window_stack_contains_window(s_menu_window)) ||
        (s_pick_window && window_stack_contains_window(s_pick_window)) ||
        remote_send_ui_blocks_remote()) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "remote: already in menu/picker/send UI");
        return;
    }

    DataSource ds = state->config.data_source;
    if (ds != DATA_SOURCE_TRIO && ds != DATA_SOURCE_APPLE_HEALTH_VIA_TRIO) {
        APP_LOG(APP_LOG_LEVEL_INFO, "remote: wrong data_source=%d (need Trio)", (int)ds);
        vibes_double_pulse();
        return;
    }

    if (!s_menu_window) {
        s_menu_window = window_create();
        window_set_window_handlers(s_menu_window, (WindowHandlers){
            .load = menu_window_load,
            .unload = menu_window_unload,
        });
    }

    APP_LOG(APP_LOG_LEVEL_INFO, "remote: pushing menu (stack top %p)", window_stack_get_top_window());
    window_stack_push(s_menu_window, true);
}
