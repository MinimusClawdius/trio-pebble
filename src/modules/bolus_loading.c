#include "bolus_loading.h"
#include "../trio_types.h"
#include <pebble.h>

static Window *s_loading_window = NULL;
static Layer *s_animation_layer = NULL;

// Animation state
static int32_t s_drip_y = 0;
static int32_t s_drip_radius = 0;
static int32_t s_drip_phase = 0; // 0: growing, 1: falling, 2: shrinking/resetting

// Constants for drawing
#define SYRINGE_WIDTH 24
#define SYRINGE_HEIGHT 48
#define NEEDLE_WIDTH 2
#define NEEDLE_HEIGHT 12
#define DRIP_MAX_RADIUS 5
#define DRIP_MAX_TRAVEL 15

// Animation timer
static AppTimer *s_timer = NULL;

static void animation_timer_callback(void *context) {
    (void)context;
    if (!s_animation_layer) return;

    // Update drip phase
    s_drip_phase++;
    if (s_drip_phase > 3) {
        s_drip_phase = 0;
        s_drip_y = 0;
        s_drip_radius = 0;
    }

    // Simple state machine for the drip
    if (s_drip_phase == 0) { // Growing
        s_drip_radius += 1;
        if (s_drip_radius >= DRIP_MAX_RADIUS) s_drip_phase = 1;
    } else if (s_drip_phase == 1) { // Falling
        s_drip_y += 2;
        if (s_drip_y >= DRIP_MAX_TRAVEL) s_drip_phase = 2;
    } else if (s_drip_phase == 2) { // Shrinking
        s_drip_radius -= 1;
        if (s_drip_radius <= 0) s_drip_phase = 0;
    }

    layer_mark_dirty(s_animation_layer);
    s_timer = app_timer_register(app_timer_range(100, 200), animation_timer_callback, NULL);
}

static void animation_layer_update(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int center_x = bounds.size.w / 2;
    int center_y = bounds.size.h / 2;

    // Syringe Barrel
    GRect barrel = GRect(center_x - (SYRINGE_WIDTH / 2), center_y - (SYRINGE_HEIGHT / 2), SYRINGE_WIDTH, SYRINGE_HEIGHT);
    graphics_draw_rect(ctx, barrel);
    // Syringe plunger (filled rectangle)
    graphics_fill_rect(ctx, GRect(barrel.origin.x + 4, barrel.origin.y + 4, SYRINGE_WIDTH - 8, SYRINGE_HEIGHT - 8), 1, GCornerNone);

    // Needle
    GRect needle = GRect(center_x - (NEEDLE_WIDTH / 2), barrel.origin.y + SYRINGE_HEIGHT, NEEDLE_WIDTH, NEEDLE_HEIGHT);
    graphics_draw_line(ctx, needle.origin.x + (NEEDLE_WIDTH / 2), needle.origin.y, 
                      needle.origin.x + (NEEDLE_WIDTH / 2), needle.origin.y + NEEDLE_HEIGHT);

    // The Drip
    if (s_drip_radius > 0) {
        int drip_x = center_x;
        int drip_y = needle.origin.y + NEEDLE_HEIGHT + s_drip_y;
        // Use a small ellipse for a circle
        GRect drip_rect = GRect(drip_x - s_drip_radius, drip_y - s_drip_radius, s_drip_radius * 2, s_drip_radius * 2);
        graphics_fill_ellipse(ctx, drip_rect, GCornerNone);
    }
}

static void loading_window_load(Window *window) {
    Layer *root = window_get_root_layer(window);
    s_animation_layer = layer_create(root);
    layer_set_update_proc(s_animation_layer, animation_layer_update);
    
    s_drip_phase = 0;
    s_drip_y = 0;
    s_drip_radius = 0;
    s_timer = app_timer_register(app_timer_range(100, 200), animation_timer_callback, NULL);
}

static void loading_window_unload(Window *window) {
    (void)window;
    if (s_timer) {
        app_timer_cancel(s_timer);
        s_timer = NULL;
    }
    layer_destroy(s_animation_layer);
}

void bolus_loading_show(void) {
    if (!s_loading_window) {
        s_loading_window = window_create();
        window_set_window_handlers(s_loading_window, (WindowHandlers){
            .load = loading_window_load,
            .unload = loading_window_unload,
        });
    }
    window_stack_push(s_loading_window, true);
}

void bolus_loading_hide(void) {
    if (s_loading_window && window_stack_contains_window(s_loading_window)) {
        window_stack_pop(true);
    }
}
