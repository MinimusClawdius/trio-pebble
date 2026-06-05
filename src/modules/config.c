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
    // sanitize_comp_slots();  // TEMP DEBUG
    // sanitize_graph_scale_mode();  // TEMP DEBUG
    // sanitize_graph_time_range();  // TEMP DEBUG
        if (s_config.header_size > 3) s_config.header_size = 3;
    }
}

void config_apply_message(DictionaryIterator *iter) {
    APP_LOG(APP_LOG_LEVEL_INFO, "[CONFIG] config_apply_message() ENTER (MINIMAL MODE)");

    Tuple *t;
    int count = 0;
    for (t = dict_read_first(iter); t != NULL; t = dict_read_next(iter)) {
        APP_LOG(APP_LOG_LEVEL_INFO, "[CONFIG] Received key %ld = %ld", (long)t->key, (long)t->value->int32);
        count++;
        if (count > 30) break; // safety
    }

    APP_LOG(APP_LOG_LEVEL_INFO, "[CONFIG] config_apply_message() EXIT (MINIMAL MODE)");
}

TrioConfig *config_get(void) {
    return &s_config;
}
