#include "device_input.h"

bool wf_pointer_sample(wf_pointer_state *s, bool valid, bool pressed, uint16_t x, uint16_t y) {
    if (s->wait_release) {
        if (valid && !pressed) s->wait_release = false;
        s->pressed = false;
        return false;
    }
    if (valid) {
        s->pressed = pressed;
        if (pressed) { s->x = x; s->y = y; }
    }
    return s->pressed;
}
void wf_pointer_after_wake(wf_pointer_state *s) {
    s->pressed = false;
    s->wait_release = true;
}
int wf_key_sample(wf_key_state *s, bool pressed, uint32_t now, bool allow_long) {
    if (s->wait_release) {
        if (pressed) s->releasing = false;
        else if (!s->releasing) { s->releasing = true; s->released_at = now; }
        else if (now - s->released_at >= 30) {
            s->wait_release = s->level = s->releasing = false;
        }
        return WF_KEY_EVENT_NONE;
    }
    int event = WF_KEY_EVENT_NONE;
    if (pressed) {
        s->releasing = false;
        if (!s->level) {
            s->level = true; s->down_at = now; s->long_sent = false;
            return WF_KEY_EVENT_DOWN;
        }
    } else if (s->level) {
        if (!s->releasing) { s->releasing = true; s->released_at = now; }
        if (now - s->released_at >= 30) {
            if (!s->long_sent && s->released_at - s->down_at > 30) event = WF_KEY_EVENT_CLICK;
            s->level = s->releasing = false;
        }
        return event;
    }
    uint32_t held = now - s->down_at; /* unsigned subtraction also handles tick wrap */
    if (pressed && allow_long && !s->long_sent && held > 700) {
        s->long_sent = true; event = WF_KEY_EVENT_LONG;
    }
    return event;
}
void wf_key_after_wake(wf_key_state *s) { s->wait_release = true; s->releasing = false; }
