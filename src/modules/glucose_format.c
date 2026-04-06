#include "glucose_format.h"
#include <stdio.h>

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
