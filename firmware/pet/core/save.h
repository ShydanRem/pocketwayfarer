#pragma once
/* Versioned, checksummed snapshot of wf_game. Storage backends (NVS on the
 * device, files on the host) only move these bytes around. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "game.h"

#define WF_SAVE_MAGIC 0x31465750u   /* "PWF1" */
#define WF_SAVE_VERSION 1u

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint32_t crc;
    uint32_t sequence;   /* increases at every write; newest slot wins */
} wf_save_header;

#define WF_SAVE_BYTES (sizeof(wf_save_header) + sizeof(wf_game))

uint32_t wf_crc32(const void *data, size_t len);
size_t wf_save_encode(const wf_game *g, uint32_t sequence, uint8_t *out, size_t cap);
/* Returns false on any mismatch; *g untouched in that case. */
bool wf_save_decode(const uint8_t *in, size_t len, wf_game *g, uint32_t *sequence);
/* Clamps every field into its valid range after a load. */
void wf_save_sanitize(wf_game *g);
