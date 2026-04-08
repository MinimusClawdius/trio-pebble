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
    /* 12h without AM/PM — saves width (avoids "..." in narrow header). User infers period from context. */
    strftime(buf, buf_len, "%I:%M", t);
    if (buf[0] == '0' && buf[1] != '\0') {
        memmove(buf, buf + 1, strlen(buf));
    }
}
