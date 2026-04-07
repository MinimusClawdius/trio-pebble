#include "time_display.h"
#include <stdio.h>
#include <string.h>

void trio_format_clock(char *buf, size_t buf_len, time_t now, bool use_24h) {
    if (!buf || buf_len < 6) {
        return;
    }
    struct tm *t = localtime(&now);
    if (!t) {
        buf[0] = '\0';
        return;
    }
    if (use_24h) {
        strftime(buf, buf_len, "%H:%M", t);
        return;
    }
    strftime(buf, buf_len, "%I:%M", t);
    if (buf[0] == '0' && buf[1] != '\0') {
        memmove(buf, buf + 1, strlen(buf));
    }
    size_t n = strlen(buf);
    if (n + 4 < buf_len) {
        buf[n++] = ' ';
        if (t->tm_hour >= 12) {
            memcpy(buf + n, "PM", 3);
        } else {
            memcpy(buf + n, "AM", 3);
        }
    }
}
