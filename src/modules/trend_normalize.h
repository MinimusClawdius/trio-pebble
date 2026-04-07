#pragma once
#include <stddef.h>

/** Normalize CGM trend text to short UTF-8 arrows for all data paths (JS + native BLE). */
void trio_normalize_trend_str(char *buf, size_t cap);
