#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Portable input policy shared by the physical board and host regressions.
 * A failed I2C sample is unknown, never an invented finger release. */
typedef struct {
    bool pressed, wait_release;
    uint16_t x, y;
} wf_pointer_state;
bool wf_pointer_sample(wf_pointer_state *state, bool valid, bool pressed, uint16_t x, uint16_t y);
void wf_pointer_after_wake(wf_pointer_state *state);

typedef struct {
    bool level, long_sent, wait_release, releasing;
    uint32_t down_at, released_at;
} wf_key_state;
enum { WF_KEY_EVENT_NONE, WF_KEY_EVENT_CLICK, WF_KEY_EVENT_LONG, WF_KEY_EVENT_DOWN };
int wf_key_sample(wf_key_state *state, bool pressed, uint32_t now_ms, bool allow_long);
void wf_key_after_wake(wf_key_state *state);
