#pragma once
#include <pebble.h>

/** Call once at startup (before first window). Loads overlay flag + log from persist. */
void click_debug_init(void);

/** Create debug text layer on top of watchface root (call from window_load). */
void click_debug_attach_to_window(Window *window);

/** Clear layer pointer before window is destroyed (call from window_unload). */
void click_debug_detach_window(void);

/** Append one line (timestamped); updates persist + on-screen text if overlay on. */
void click_debug_log(const char *msg);

/** Toggle visibility of the overlay (also persists preference). */
void click_debug_toggle_overlay(void);
