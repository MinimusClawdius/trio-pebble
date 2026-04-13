#pragma once
#include <pebble.h>

// ============================================================
// Trio Pebble - Shared Types & Constants
// ============================================================

#define APP_VERSION "2.16.0"
#define MAX_GRAPH_POINTS 48
#define MAX_PREDICTIONS 24

// ---------- AppMessage Keys ----------
// Must match package.json messageKeys
typedef enum {
    KEY_GLUCOSE = 0,
    KEY_TREND,
    KEY_DELTA,
    KEY_IOB,
    KEY_COB,
    KEY_LAST_LOOP,
    KEY_GLUCOSE_STALE,
    KEY_CMD_TYPE,
    KEY_CMD_AMOUNT,
    KEY_CMD_STATUS,
    KEY_GRAPH_DATA,
    KEY_GRAPH_COUNT,
    KEY_LOOP_STATUS,
    KEY_UNITS,
    KEY_PUMP_STATUS,
    KEY_RESERVOIR,
    // v2 keys
    KEY_CONFIG_FACE_TYPE,
    KEY_CONFIG_DATA_SOURCE,
    KEY_CONFIG_HIGH_THRESHOLD,
    KEY_CONFIG_LOW_THRESHOLD,
    KEY_CONFIG_ALERT_HIGH_ENABLED,
    KEY_CONFIG_ALERT_LOW_ENABLED,
    KEY_CONFIG_ALERT_URGENT_LOW,
    KEY_CONFIG_ALERT_SNOOZE_MIN,
    KEY_CONFIG_COLOR_SCHEME,
    KEY_BATTERY_PHONE,
    KEY_WEATHER_TEMP,
    KEY_WEATHER_ICON,
    KEY_STEPS,
    KEY_HEART_RATE,
    KEY_PREDICTIONS_DATA,
    KEY_PREDICTIONS_COUNT,
    KEY_PUMP_BATTERY,
    KEY_SENSOR_AGE,
    KEY_CONFIG_CHANGED,
    KEY_TAP_ACTION,
    KEY_CONFIG_WEATHER_ENABLED,
    KEY_CONFIG_COMP_SLOT_0,
    KEY_CONFIG_COMP_SLOT_1,
    KEY_CONFIG_COMP_SLOT_2,
    KEY_CONFIG_COMP_SLOT_3,
    KEY_CONFIG_CLOCK_24H,
    /** 0=auto-fit Y-axis to data, 1=fit around High/Low thresholds, 2=legacy 40–400 mg/dL */
    KEY_CONFIG_GRAPH_SCALE_MODE,
    /** 0=3h, 1=6h, 2=12h, 3=24h — phone fetches & down-samples to 48 points */
    KEY_CONFIG_GRAPH_TIME_RANGE,
    /** PebbleKit JS: short Trio/HTTP status (e.g. "No phone"); cleared on CGM update */
    KEY_TRIO_LINK,
    /** After carb POST, pkjs may send recommended bolus in tenths U to open the bolus picker (Trio HTTP). */
    KEY_SUGGESTED_BOLUS_TENTHS,
    KEY_COUNT
} AppMessageKey;

// ---------- Data Source ----------
typedef enum {
    DATA_SOURCE_TRIO = 0,
    DATA_SOURCE_DEXCOM_SHARE,
    DATA_SOURCE_NIGHTSCOUT,
    /** Same HTTP API as Trio; use when CGM reaches Trio via Apple Health / Dexcom / etc. */
    DATA_SOURCE_APPLE_HEALTH_VIA_TRIO
} DataSource;

// ---------- Face Type ----------
typedef enum {
    FACE_CLASSIC = 0,
    FACE_GRAPH_FOCUS,
    FACE_COMPACT,
    FACE_DASHBOARD,
    FACE_MINIMAL,
    /** Retro digital / LCD bezel — inspired by classic faces like 91 Dub */
    FACE_RETRO_DIGITAL,
    FACE_COUNT
} FaceType;

// ---------- Color Scheme ----------
typedef enum {
    COLOR_SCHEME_DARK = 0,
    COLOR_SCHEME_LIGHT,
    COLOR_SCHEME_HIGH_CONTRAST,
    COLOR_SCHEME_COUNT
} ColorScheme;

// ---------- Trend Direction ----------
typedef enum {
    TREND_NONE = 0,
    TREND_DOUBLE_UP,
    TREND_SINGLE_UP,
    TREND_FORTY_FIVE_UP,
    TREND_FLAT,
    TREND_FORTY_FIVE_DOWN,
    TREND_SINGLE_DOWN,
    TREND_DOUBLE_DOWN,
    TREND_NOT_COMPUTABLE
} TrendDirection;

/** Vertical scale for CGM graph (values are always mg/dL on the wire). */
typedef enum {
    GRAPH_SCALE_AUTO = 0,
    GRAPH_SCALE_THRESHOLDS = 1,
    GRAPH_SCALE_LEGACY = 2
} GraphScaleMode;

/** Horizontal span for graph samples (phone-side fetch); watch still gets ≤48 points. */
typedef enum {
    GRAPH_TIME_3H = 0,
    GRAPH_TIME_6H = 1,
    GRAPH_TIME_12H = 2,
    GRAPH_TIME_24H = 3
} GraphTimeRange;

// ---------- Tap Action (future touch framework) ----------
typedef enum {
    TAP_ACTION_NONE = 0,
    TAP_ACTION_OPEN_CARBS,
    TAP_ACTION_OPEN_BOLUS,
    TAP_ACTION_OPEN_TEMP_BASAL,
    TAP_ACTION_REFRESH,
    TAP_ACTION_TOGGLE_FACE
} TapAction;

/** Bottom bar slots (left → right). Values sent via KEY_CONFIG_COMP_SLOT_0..3. */
typedef enum {
    COMP_SLOT_NONE = 0,
    COMP_SLOT_WATCH_BATTERY,
    COMP_SLOT_PHONE_BATTERY,
    COMP_SLOT_STEPS,
    COMP_SLOT_HEART_RATE,
    COMP_SLOT_WEATHER,
    /** Loop / pump IOB string (e.g. 45u) — footer matches LOOP watchface spec */
    COMP_SLOT_IOB = 6
} ComplicationSlotKind;

#define TRIO_COMP_SLOT_COUNT 4
/** Visible columns in the footer bar (slots 0..2; slot 3 kept for protocol / future). */
#define TRIO_COMP_BAR_COLUMNS 3

// ---------- Configuration ----------
typedef struct {
    FaceType face_type;
    DataSource data_source;
    ColorScheme color_scheme;
    int16_t high_threshold;     // mg/dL
    int16_t low_threshold;      // mg/dL
    int16_t urgent_low;         // mg/dL
    bool alert_high_enabled;
    bool alert_low_enabled;
    uint8_t alert_snooze_min;
    bool show_complications;
    bool is_mmol;               // display: mg/dL vs mmol/L (from KEY_UNITS)
    bool weather_enabled;       // master switch: fetch/draw weather data
    uint8_t comp_slot[TRIO_COMP_SLOT_COUNT]; /* ComplicationSlotKind per column */
    bool clock_24h;             /* true = 24h clock, false = 12h (no AM/PM on watch) */
    uint8_t graph_scale_mode;   /* GraphScaleMode */
    uint8_t graph_time_range;     /* GraphTimeRange */
} TrioConfig;

// ---------- CGM State ----------
typedef struct {
    int16_t glucose;
    TrendDirection trend;
    char trend_str[16];
    char delta_str[16];
    bool is_stale;
    char units[8];
    time_t last_reading_time;
} CGMState;

// ---------- Loop State ----------
typedef struct {
    char iob[16];
    char cob[16];
    char last_loop_time[16];
    /** When non-empty, dashboard shows this instead of last_loop_time (HTTP link hint from JS). */
    char trio_link[16];
    char loop_status[32];
    char pump_status[16];
    int8_t reservoir;           // percentage or units
    int8_t pump_battery;        // percentage
    char sensor_age[16];
} LoopState;

// ---------- Complications ----------
typedef struct {
    int8_t phone_battery;       // 0-100
    int16_t weather_temp;       // degrees
    char weather_icon[16];      // condition code from phone (clear, partly, rain, …)
    int32_t steps;
    int16_t heart_rate;
    uint8_t watch_battery;      // 0-100
    bool watch_charging;
} Complications;

// ---------- Graph Data ----------
typedef struct {
    int16_t values[MAX_GRAPH_POINTS];
    int count;
    int16_t predictions[MAX_PREDICTIONS];
    int prediction_count;
} GraphData;

// ---------- Alert State ----------
typedef struct {
    bool high_active;
    bool low_active;
    bool urgent_low_active;
    time_t last_alert_time;
    time_t last_urgent_alert_time;
    time_t snooze_until;
} AlertState;

// ---------- Full App State ----------
typedef struct {
    TrioConfig config;
    CGMState cgm;
    LoopState loop;
    Complications comp;
    GraphData graph;
    AlertState alerts;
} AppState;

// ---------- Face Render Interface ----------
typedef void (*FaceLoadFunc)(Window *window, Layer *root, GRect bounds);
typedef void (*FaceUnloadFunc)(void);
typedef void (*FaceUpdateFunc)(AppState *state);

typedef struct {
    const char *name;
    FaceLoadFunc load;
    FaceUnloadFunc unload;
    FaceUpdateFunc update;
} FaceDefinition;

// ---------- Globals ----------
AppState *app_state_get(void);
TrioConfig *config_get(void);
