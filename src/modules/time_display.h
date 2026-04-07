#pragma once
#include <pebble.h>
#include <stddef.h>

/** Format local time: 24h "HH:MM" or 12h "H:MM AM" (buf >= 14 bytes). */
void trio_format_clock(char *buf, size_t buf_len, time_t now, bool use_24h);
