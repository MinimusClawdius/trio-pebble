#include "glucose_format.h"
#include <stdio.h>

void format_reading_age_upper(char *buf, size_t buflen, time_t last_reading, time_t now) {
    if (buflen < 8) return;
    if (last_reading <= 0) {
        snprintf(buf, buflen, "--");
        return;
    }
    long age_sec = (long)(now - last_reading);
    if (age_sec < 0) age_sec = 0;
    if (age_sec < 60) {
        snprintf(buf, buflen, "NOW");
        return;
    }
    int mins = (int)(age_sec / 60);
    if (mins >= 120) {
        snprintf(buf, buflen, "%d HR+ AGO", mins / 60);
        return;
    }
    if (mins == 1) {
        snprintf(buf, buflen, "1 MIN AGO");
    } else {
        snprintf(buf, buflen, "%d MIN AGO", mins);
    }
}

void format_threshold_label(char *buf, size_t buflen, int16_t threshold_mg_dl, bool is_mmol) {
    if (buflen < 4) return;
    format_glucose_display(buf, buflen, threshold_mg_dl, is_mmol);
}

void format_glucose_display(char *buf, size_t buflen, int16_t mg_dl, bool is_mmol) {
    if (buflen < 4) return;
    if (mg_dl <= 0) {
        snprintf(buf, buflen, "--");
        return;
    }
    if (!is_mmol) {
        snprintf(buf, buflen, "%d", (int)mg_dl);
        return;
    }
    /* mmol/L ≈ mg/dL / 18, one decimal, rounded */
    int tenths = ((int)mg_dl * 10 + 9) / 18;
    if (tenths < 0) tenths = 0;
    int whole = tenths / 10;
    int frac = tenths % 10;
    snprintf(buf, buflen, "%d.%d", whole, frac);
}
