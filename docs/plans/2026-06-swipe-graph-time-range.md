# Swipe Left/Right on Graph to Change Time Range — Implementation Plan

**Goal:** Add native touch swipe support (left/right) on the graph area to cycle the graph time range (3H → 6H → 12H → 24H) on Pebble Time 2 (emery) and other touch-enabled hardware.

**Architecture:** 
- Use the real `touch_service` + `TouchEvent` API (as used in TouchyWeather).
- Extend the existing `tap_framework` to handle real touch events and detect horizontal swipes.
- When a horizontal swipe is detected over the graph layer, cycle `config.graph_time_range`, persist it, trigger a data refresh, and redraw.
- Keep backward compatibility with accelerometer tap.
- Guard all touch code with `#if defined(PBL_TOUCH)`.

**Tech Stack:** Pebble C SDK (emery target), touch_service API, existing TrioConfig + graph module.

---

## Task 1: Extend tap_framework.h with TouchEvent support

**Objective:** Add function signatures for real touch handling.

**Files:**
- Modify: `src/modules/tap_framework.h`

**Changes:**

Add after the existing declarations:

```c
#if defined(PBL_TOUCH)
void tap_framework_handle_touch_event(const TouchEvent *event);
#endif
```

## Task 2: Implement touch handling + swipe detection in tap_framework.c

**Objective:** Port the swipe detection logic from TouchyWeather and integrate with TapAction / graph cycle.

**Files:**
- Modify: `src/modules/tap_framework.c`

**Step-by-step code to add:**

At the top (after includes):

```c
#if defined(PBL_TOUCH)
static bool s_tracking = false;
static int16_t s_start_x = 0;
static int16_t s_start_y = 0;
static GRect s_graph_bounds = {0};  // set by faces when registering
#endif
```

New function:

```c
#if defined(PBL_TOUCH)
void tap_framework_set_graph_bounds(GRect bounds) {
  s_graph_bounds = bounds;
}

void tap_framework_handle_touch_event(const TouchEvent *event) {
  if (!event) return;

  switch (event->type) {
    case TouchEvent_Touchdown:
      s_tracking = true;
      s_start_x = event->x;
      s_start_y = event->y;
      break;

    case TouchEvent_Liftoff: {
      if (!s_tracking) break;

      int16_t dx = event->x - s_start_x;
      int16_t dy = event->y - s_start_y;
      int16_t adx = dx < 0 ? -dx : dx;
      int16_t ady = dy < 0 ? -dy : dy;

      const int16_t HSWIPE_THRESHOLD = 30;
      const int16_t TAP_THRESHOLD = 15;

      bool in_graph = grect_contains_point(&s_graph_bounds, &(GPoint){event->x, event->y});

      if (adx > HSWIPE_THRESHOLD && adx > ady && in_graph) {
        // Horizontal swipe on graph → cycle time range
        tap_framework_send_action(TAP_ACTION_CYCLE_GRAPH_TIME);
      } else if (adx < TAP_THRESHOLD && ady < TAP_THRESHOLD) {
        // tap handling (existing zone logic can be extended here)
      }

      s_tracking = false;
      break;
    }
    default:
      break;
  }
}
#endif
```

Also update `tap_framework_send_action` to handle the new `TAP_ACTION_CYCLE_GRAPH_TIME`.

## Task 3: Add TAP_ACTION_CYCLE_GRAPH_TIME to trio_types.h

**Files:**
- Modify: `src/trio_types.h`

Add to the `TapAction` enum:

```c
TAP_ACTION_CYCLE_GRAPH_TIME = 6,
```

## Task 4: Wire touch subscription in main.c

**Files:**
- Modify: `src/main.c`

Add in the appropriate init section (near accel_tap_service_subscribe):

```c
#if defined(PBL_TOUCH)
  if (touch_service_is_enabled()) {
    touch_service_subscribe(tap_framework_handle_touch_event, NULL);
  }
#endif
```

Add unsubscription in deinit.

Also add a handler for the new action that cycles the graph time range.

## Task 5: Register graph bounds in face load functions

**Files:**
- Modify: relevant face_*.c files (start with face_classic.c)

In the face load function, after creating the graph layer:

```c
tap_framework_set_graph_bounds(graph_layer_bounds);
```

## Task 6: Implement the cycle + refresh logic

**Objective:** When `TAP_ACTION_CYCLE_GRAPH_TIME` is received, cycle the config and request new data.

**Files:**
- Modify: `src/main.c` or a new handler in config/graph

Add logic to cycle `config.graph_time_range` and call the existing refresh path.

## Task 7: Update Documentation

**Files:**
- Modify: `README.md` (Touch/Tap Framework section)
- Create/Update: reference in docs/ if needed

Update the section to document the new real touch support and swipe behavior.

## Verification Steps

1. Build for emery target.
2. Install on Pebble Time 2.
3. Swipe left/right on the graph area.
4. Confirm time range cycles and graph updates.

---

**Ready to execute?** I can now implement task-by-task using the plan above.