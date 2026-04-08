#include "config.h"
#include "platform_compat.h"

#define CONFIG_KEY 0x54726F36u  /* v6: clock_24h; invalidates shorter TrioConfig blobs */

static TrioConfig s_config;

static void set_defaults(void) {
    s_config.face_type = FACE_CLASSIC;
    s_config.data_source = DATA_SOURCE_TRIO;
    s_config.color_scheme = COLOR_SCHEME_DARK;
    s_config.high_threshold = 180;
    s_config.low_threshold = 70;
    s_config.urgent_low = 55;
    s_config.alert_high_enabled = true;
    s_config.alert_low_enabled = true;
    s_config.alert_snooze_min = 15;
    s_config.show_complications = true;
    s_config.is_mmol = false;
    s_config.weather_enabled = true;
    s_config.comp_slot[0] = COMP_SLOT_WATCH_BATTERY;
    s_config.comp_slot[1] = COMP_SLOT_WEATHER;
    s_config.comp_slot[2] = COMP_SLOT_IOB;
    s_config.comp_slot[3] = COMP_SLOT_NONE;
    s_config.clock_24h = true;
#if !TRIO_DISPLAY_COLOR
    /* Sky/gradient art is color-first; B&W keeps a clean graph. Temp still available if user enables weather. */
    s_config.color_scheme = COLOR_SCHEME_HIGH_CONTRAST;
#endif
}

void config_init(void) {
    set_defaults();
    config_load();
}

void config_save(void) {
    persist_write_data(CONFIG_KEY, &s_config, sizeof(TrioConfig));
}

static void sanitize_comp_slots(void) {
    for (int i = 0; i < TRIO_COMP_SLOT_COUNT; i++) {
        if (s_config.comp_slot[i] > COMP_SLOT_IOB) {
            s_config.comp_slot[i] = COMP_SLOT_NONE;
        }
    }
}

void config_load(void) {
    if (persist_exists(CONFIG_KEY)) {
        persist_read_data(CONFIG_KEY, &s_config, sizeof(TrioConfig));
        sanitize_comp_slots();
    }
}

void config_apply_message(DictionaryIterator *iter) {
    Tuple *t;

    t = dict_find(iter, KEY_CONFIG_FACE_TYPE);
    if (t) s_config.face_type = (FaceType)t->value->int32;

    t = dict_find(iter, KEY_CONFIG_DATA_SOURCE);
    if (t) s_config.data_source = (DataSource)t->value->int32;

    t = dict_find(iter, KEY_CONFIG_HIGH_THRESHOLD);
    if (t) s_config.high_threshold = (int16_t)t->value->int32;

    t = dict_find(iter, KEY_CONFIG_LOW_THRESHOLD);
    if (t) s_config.low_threshold = (int16_t)t->value->int32;

    t = dict_find(iter, KEY_CONFIG_ALERT_URGENT_LOW);
    if (t) s_config.urgent_low = (int16_t)t->value->int32;

    t = dict_find(iter, KEY_CONFIG_ALERT_HIGH_ENABLED);
    if (t) s_config.alert_high_enabled = t->value->int32 != 0;

    t = dict_find(iter, KEY_CONFIG_ALERT_LOW_ENABLED);
    if (t) s_config.alert_low_enabled = t->value->int32 != 0;

    t = dict_find(iter, KEY_CONFIG_ALERT_SNOOZE_MIN);
    if (t) s_config.alert_snooze_min = (uint8_t)t->value->int32;

    t = dict_find(iter, KEY_CONFIG_COLOR_SCHEME);
    if (t) s_config.color_scheme = (ColorScheme)t->value->int32;

    t = dict_find(iter, KEY_UNITS);
    if (t) {
        const char *u = t->value->cstring;
        /* "mmol" / "mmol/L" style */
        s_config.is_mmol = (u && u[0] == 'm' && u[1] == 'm' && u[2] == 'o');
    }

    t = dict_find(iter, KEY_CONFIG_WEATHER_ENABLED);
    if (t) s_config.weather_enabled = t->value->int32 != 0;

    t = dict_find(iter, KEY_CONFIG_COMP_SLOT_0);
    if (t) {
        int32_t v = t->value->int32;
        s_config.comp_slot[0] = (v >= COMP_SLOT_NONE && v <= COMP_SLOT_IOB) ? (uint8_t)v : COMP_SLOT_NONE;
    }
    t = dict_find(iter, KEY_CONFIG_COMP_SLOT_1);
    if (t) {
        int32_t v = t->value->int32;
        s_config.comp_slot[1] = (v >= COMP_SLOT_NONE && v <= COMP_SLOT_IOB) ? (uint8_t)v : COMP_SLOT_NONE;
    }
    t = dict_find(iter, KEY_CONFIG_COMP_SLOT_2);
    if (t) {
        int32_t v = t->value->int32;
        s_config.comp_slot[2] = (v >= COMP_SLOT_NONE && v <= COMP_SLOT_IOB) ? (uint8_t)v : COMP_SLOT_NONE;
    }
    t = dict_find(iter, KEY_CONFIG_COMP_SLOT_3);
    if (t) {
        int32_t v = t->value->int32;
        s_config.comp_slot[3] = (v >= COMP_SLOT_NONE && v <= COMP_SLOT_IOB) ? (uint8_t)v : COMP_SLOT_NONE;
    }

    t = dict_find(iter, KEY_CONFIG_CLOCK_24H);
    if (t) s_config.clock_24h = t->value->int32 != 0;

    config_save();
}

TrioConfig *config_get(void) {
    return &s_config;
}
