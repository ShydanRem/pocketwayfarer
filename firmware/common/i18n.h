#pragma once
/* Two-language text: every string is written in Italian and English side by side. */
#include <stdint.h>

typedef struct { const char *it, *en; } wf_text;

extern uint8_t wf_lang;   /* 0 = italiano, 1 = English */
static inline const char *TR(const char *it, const char *en) { return wf_lang ? en : it; }
static inline const char *TX(wf_text t) { return wf_lang ? t.en : t.it; }
