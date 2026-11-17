#include <assert.h>
#include <stdio.h>
#include "device_input.h"

int main(void) {
    wf_pointer_state p = {0};
    assert(wf_pointer_sample(&p, true, true, 120, 220));
    assert(wf_pointer_sample(&p, false, false, 0, 0));
    assert(p.x == 120 && p.y == 220); /* no fabricated click on an I2C error */
    assert(!wf_pointer_sample(&p, true, false, 0, 0));
    wf_pointer_after_wake(&p);
    assert(!wf_pointer_sample(&p, true, true, 100, 200) && p.wait_release);
    assert(!wf_pointer_sample(&p, false, false, 0, 0) && p.wait_release);
    assert(!wf_pointer_sample(&p, true, true, 100, 200) && p.wait_release);
    assert(!wf_pointer_sample(&p, true, false, 0, 0) && !p.wait_release);
    assert(wf_pointer_sample(&p, true, true, 80, 90));
    wf_key_state k = {0};
    assert(wf_key_sample(&k, true, 100, true) == WF_KEY_EVENT_DOWN);
    k.long_sent = true; /* timing consumed at DOWN, so no delayed second input */
    assert(wf_key_sample(&k, false, 110, true) == WF_KEY_EVENT_NONE);
    assert(wf_key_sample(&k, true, 115, true) == WF_KEY_EVENT_NONE); /* contact bounce */
    assert(wf_key_sample(&k, false, 120, true) == WF_KEY_EVENT_NONE);
    assert(wf_key_sample(&k, false, 150, true) == WF_KEY_EVENT_NONE);
    assert(wf_key_sample(&k, true, 200, true) == WF_KEY_EVENT_DOWN);
    assert(wf_key_sample(&k, true, 901, true) == WF_KEY_EVENT_LONG);
    assert(wf_key_sample(&k, true, 1200, true) == WF_KEY_EVENT_NONE);
    assert(wf_key_sample(&k, false, 1300, true) == WF_KEY_EVENT_NONE);
    assert(wf_key_sample(&k, false, 1330, true) == WF_KEY_EVENT_NONE);
    wf_key_after_wake(&k);
    assert(wf_key_sample(&k, true, 2000, true) == WF_KEY_EVENT_NONE);
    assert(wf_key_sample(&k, true, 4000, true) == WF_KEY_EVENT_NONE);
    assert(wf_key_sample(&k, false, 4010, true) == WF_KEY_EVENT_NONE);
    assert(wf_key_sample(&k, true, 4020, true) == WF_KEY_EVENT_NONE);
    assert(wf_key_sample(&k, false, 4030, true) == WF_KEY_EVENT_NONE);
    assert(wf_key_sample(&k, false, 4060, true) == WF_KEY_EVENT_NONE && !k.wait_release);
    assert(wf_key_sample(&k, true, UINT32_MAX-20u, true) == WF_KEY_EVENT_DOWN);
    assert(wf_key_sample(&k, false, 50, true) == WF_KEY_EVENT_NONE);
    assert(wf_key_sample(&k, false, 80, true) == WF_KEY_EVENT_CLICK);
    puts("device input: wake guards, I2C failures, press-edge timing, debounce, long press and tick wrap: passed");
    return 0;
}
