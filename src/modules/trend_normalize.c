#include "trend_normalize.h"
#include <ctype.h>
#include <string.h>

/* UTF-8 arrows (U+219x) — compact for layout next to glucose */
static const char ARR_R[] = "\xe2\x86\x92";   /* → */
static const char ARR_U[] = "\xe2\x86\x91";   /* ↑ */
static const char ARR_D[] = "\xe2\x86\x93";   /* ↓ */
static const char ARR_UR[] = "\xe2\x86\x97";  /* ↗ */
static const char ARR_DR[] = "\xe2\x86\x98";  /* ↘ */
static const char ARR_UU[] = "\xe2\x86\x91\xe2\x86\x91"; /* ↑↑ */
static const char ARR_DD[] = "\xe2\x86\x93\xe2\x86\x93"; /* ↓↓ */
static const char SYM_Q[] = "?";
static const char SYM_DASH[] = "--";
static const char SYM_WARN[] = "!";

static void trim_inplace(char *s) {
    char *p = s;
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    if (p != s) {
        memmove(s, p, strlen(p) + 1);
    }
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) {
        n--;
    }
    s[n] = '\0';
}

static void to_compact_lower(const char *src, char *out, size_t out_cap) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j + 1 < out_cap; i++) {
        unsigned char c = (unsigned char)src[i];
        if (c == ' ' || c == '_' || c == '-' || c == '\t') {
            continue;
        }
        out[j++] = (char)tolower((int)c);
    }
    out[j] = '\0';
}

static void set_out(const char *utf8, char *buf, size_t cap) {
    if (!utf8 || cap < 2) {
        return;
    }
    size_t n = strlen(utf8);
    if (n >= cap) {
        n = cap - 1;
    }
    memcpy(buf, utf8, n);
    buf[n] = '\0';
}

void trio_normalize_trend_str(char *buf, size_t cap) {
    if (!buf || cap < 4) {
        return;
    }
    buf[cap - 1] = '\0';

    unsigned char *u = (unsigned char *)buf;
    if (u[0] == 0xe2 && u[1] == 0x86) {
        return;
    }

    char *plus = strchr(buf, '+');
    if (plus != NULL && plus != buf) {
        *plus = '\0';
    }

    trim_inplace(buf);
    if (buf[0] == '\0') {
        set_out(SYM_DASH, buf, cap);
        return;
    }

    char key[32];
    to_compact_lower(buf, key, sizeof(key));

    if (strcmp(key, "flat") == 0 || strcmp(key, "stable") == 0 || strcmp(key, "steady") == 0) {
        set_out(ARR_R, buf, cap);
        return;
    }
    if (strcmp(key, "singleup") == 0) {
        set_out(ARR_U, buf, cap);
        return;
    }
    if (strcmp(key, "doubleup") == 0) {
        set_out(ARR_UU, buf, cap);
        return;
    }
    if (strcmp(key, "tripleup") == 0) {
        set_out(ARR_UU, buf, cap);
        return;
    }
    if (strcmp(key, "fortyfiveup") == 0) {
        set_out(ARR_UR, buf, cap);
        return;
    }
    if (strcmp(key, "singledown") == 0) {
        set_out(ARR_D, buf, cap);
        return;
    }
    if (strcmp(key, "doubledown") == 0) {
        set_out(ARR_DD, buf, cap);
        return;
    }
    if (strcmp(key, "tripledown") == 0) {
        set_out(ARR_DD, buf, cap);
        return;
    }
    if (strcmp(key, "fortyfivedown") == 0) {
        set_out(ARR_DR, buf, cap);
        return;
    }
    if (strcmp(key, "none") == 0) {
        set_out(SYM_DASH, buf, cap);
        return;
    }
    if (strcmp(key, "notcomputable") == 0 || strcmp(key, "unknown") == 0) {
        set_out(SYM_Q, buf, cap);
        return;
    }
    if (strcmp(key, "rateoutofrange") == 0) {
        set_out(SYM_WARN, buf, cap);
        return;
    }

    if (key[0] >= '1' && key[0] <= '9' && key[1] == '\0') {
        static const char *num_map[] = {
            NULL, ARR_UU, ARR_U, ARR_UR, ARR_R, ARR_DR, ARR_D, ARR_DD, SYM_Q, SYM_WARN
        };
        int idx = key[0] - '0';
        if (idx >= 1 && idx <= 9) {
            set_out(num_map[idx], buf, cap);
            return;
        }
    }

    /* Short unknown ASCII (e.g. legacy codes) — keep */
    if (strlen(buf) <= 6 && (unsigned char)buf[0] < 0x80) {
        return;
    }
    set_out(SYM_DASH, buf, cap);
}
