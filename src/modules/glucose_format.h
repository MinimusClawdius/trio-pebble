#pragma once
#include <pebble.h>

/// Format mg/dL value for display (integer mg/dL or one-decimal mmol/L).
void format_glucose_display(char *buf, size_t buflen, int16_t mg_dl, bool is_mmol);

/// Uppercase age line e.g. "1 MIN AGO" / "NOW" (LOOP-style header).
void format_reading_age_upper(char *buf, size_t buflen, time_t last_reading, time_t now);

/// Threshold next to graph guides (same units as glucose display).
void format_threshold_label(char *buf, size_t buflen, int16_t threshold_mg_dl, bool is_mmol);
