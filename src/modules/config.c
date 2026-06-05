#include "config.h"
#include "platform_compat.h"

#define CONFIG_KEY 0x54726F3Bu  /* v11: double-free recovery */ 

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
    s_config.graph_scale_mode = GRAPH_SCALE_AUTO;
    s_config.graph_time_range = GRAPH_TIME_3H;
    s_config.graph_smooth = false;
    s_config.header_size = 0;  /* 0 small */
#if !TRIO_DISPLAY_COLOR
    /* Sky/gradient art is color-first; B&W keeps a clean graph. Temp still available if user enables weather. */
    s_config.color_scheme = COLOR_SCHEME_HIGH_CONTRAST;
#endif
}

void config_init(void) {
    set_defaults();

    // One-time migration: clear any old persisted config under previous keys
    // to recover from double-free caused by struct evolution.
    persist_delete(0x54726F39u); // v9
    persist_delete(0x54726F3Au); // v10

    config_load();
}

void config_save(void) {
    persist_write_data(CONFIG_KEY, &s_config, sizeof(TrioConfig));
}

static void sanitize_comp_slots(void) {
    APP_LOG(APP_LOG_LEVEL_INFO, "[SANITIZE] sanitize_comp_slots ENTER");
    for (int i = 0; i < TRIO_COMP_SLOT_COUNT; i++) {
        if (s_config.comp_slot[i] > COMP_SLOT_IOB) {
            s_config.comp_slot[i] = COMP_SLOT_NONE;
        }
    }
}

static void sanitize_graph_scale_mode(void) {
    APP_LOG(APP_LOG_LEVEL_INFO, "[SANITIZE] sanitize_graph_scale_mode ENTER");
    if (s_config.graph_scale_mode > GRAPH_SCALE_LEGACY) {
        s_config.graph_scale_mode = GRAPH_SCALE_LEGACY;
    }
}

static void sanitize_graph_time_range(void) {
    APP_LOG(APP_LOG_LEVEL_INFO, "[SANITIZE] sanitize_graph_time_range ENTER");
    if (s_config.graph_time_range > GRAPH_TIME_24H) {
        s_config.graph_time_range = GRAPH_TIME_24H;
    }
}

void config_load(void) {
    if (persist_exists(CONFIG_KEY)) {
        persist_read_data(CONFIG_KEY, &s_config, sizeof(TrioConfig));
        sanitize_comp_slots();
        sanitize_graph_scale_mode();
        sanitize_graph_time_range();
        if (s_config.header_size > 3) s_config.header_size = 3;
    }
}

void config_apply_message(DictionaryIterator *iter) {
    APP_LOG(APP_LOG_LEVEL_INFO, "[CONFIG] config_apply_message() ENTER");
    APP_LOG(APP_LOG_LEVEL_INFO, "config_apply_message() called");

    Tuple *t;

    t = dict_find(iter, KEY_CONFIG_FACE_TYPE);
    if (t) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Got KEY_CONFIG_FACE_TYPE = %d", (int)t->value->int32);
        s_config.face_type = (FaceType)t->value->int32;
    }

    t = dict_find(iter, KEY_CONFIG_DATA_SOURCE);
    if (t) s_config.data_source = (DataSource)t->value->int32;

    t = dict_find(iter, KEY_CONFIG_HIGH_THRESHOLD);
    if (t) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Got KEY_CONFIG_HIGH_THRESHOLD = %d", (int)t->value->int32);
        s_config.high_threshold = (int16_t)t->value->int32;
    }

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
    if (t) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Got KEY_CONFIG_COLOR_SCHEME = %d", (int)t->value->int32);
        s_config.color_scheme = (ColorScheme)t->value->int32;
    }

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

    t = dict_find(iter, KEY_CONFIG_GRAPH_SCALE_MODE);
    if (t) {
        int32_t v = t->value->int32;
        s_config.graph_scale_mode = (v >= GRAPH_SCALE_AUTO && v <= GRAPH_SCALE_LEGACY) ? (uint8_t)v : GRAPH_SCALE_LEGACY;
    }

    t = dict_find(iter, KEY_CONFIG_GRAPH_TIME_RANGE);
    if (t) {
        int32_t v = t->value->int32;
        s_config.graph_time_range = (v >= GRAPH_TIME_3H && v <= GRAPH_TIME_24H) ? (uint8_t)v : GRAPH_TIME_3H;
    }

    t = dict_find(iter, KEY_CONFIG_GRAPH_SMOOTH);
    if (t) s_config.graph_smooth = t->value->int32 != 0;

    t = dict_find(iter, KEY_CONFIG_HEADER_SIZE);
    if (t) {
        int32_t v = t->value->int32;
        APP_LOG(APP_LOG_LEVEL_INFO, "[CONFIG] Got KEY_CONFIG_HEADER_SIZE = %ld", (long)v);
        APP_LOG(APP_LOG_LEVEL_INFO, "[CONFIG] About to set header_size");
        s_config.header_size = (v >= 0 && v <= 3) ? (uint8_t)v : 0;
        APP_LOG(APP_LOG_LEVEL_INFO, "[CONFIG] header_size set to %u", s_config.header_size);
    }

    sanitize_graph_scale_mode();
    sanitize_graph_time_range();
    if (s_config.header_size > 3) s_config.header_size = 3;
    APP_LOG(APP_LOG_LEVEL_INFO, "[CONFIG] config_apply_message() EXIT (header_size=%u)", s_config.header_size);
}

TrioConfig *config_get(void) {
    return &s_config;
}
