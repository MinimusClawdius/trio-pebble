#pragma once
#include <pebble.h>

/// Format mg/dL value for display (integer mg/dL or one-decimal mmol/L).
void format_glucose_display(char *buf, size_t buflen, int16_t mg_dl, bool is_mmol);
