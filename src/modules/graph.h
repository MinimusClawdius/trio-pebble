#pragma once
#include "../trio_types.h"

void graph_init(void);
void graph_deinit(void);
void graph_set_data(int16_t *values, int count);
void graph_set_predictions(int16_t *values, int count);
void graph_draw(Layer *layer, GContext *ctx, TrioConfig *config);

/** Load persisted graph data from AppState (call after state_persist_load). */
void graph_restore_from_state(const GraphData *gd);
