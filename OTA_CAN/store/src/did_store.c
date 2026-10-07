#include "did_store.h"
#include <uds_platform_time.h>
#include <string.h>

/* ── DID table ────────────────────────────────────────────────────── */

static did_entry_t s_dids[DID_MAX_ENTRIES];
static uint8_t     s_did_count = 0;

/* Simple PRNG for sensor variation */
static uint32_t s_rng_state = 0x12345678;
static uint16_t prng16(void) {
    s_rng_state ^= s_rng_state << 13;
    s_rng_state ^= s_rng_state >> 17;
    s_rng_state ^= s_rng_state << 5;
    return (uint16_t)(s_rng_state & 0xFFFF);
}

/* ── Public API ───────────────────────────────────────────────────── */

void did_store_init(void) {
    s_did_count = 0;
    s_rng_state = uds_platform_time_ms() ^ 0xDEADBEEF;
}

void did_store_add(uint16_t did, const uint8_t *data, uint8_t len,
                   bool writable, did_access_t access) {
    if (s_did_count >= DID_MAX_ENTRIES) return;
    did_entry_t *e = &s_dids[s_did_count++];
    memset(e, 0, sizeof(*e));
    e->did = did;
    e->len = (len > DID_MAX_DATA_LEN) ? DID_MAX_DATA_LEN : len;
    memcpy(e->data, data, e->len);
    e->writable = writable;
    e->access = access;
    e->varies = false;
}

void did_store_add_sensor(uint16_t did, uint8_t byte_size,
                          uint16_t initial, uint16_t vmin, uint16_t vmax,
                          did_access_t access) {
    if (s_did_count >= DID_MAX_ENTRIES) return;
    did_entry_t *e = &s_dids[s_did_count++];
    memset(e, 0, sizeof(*e));
    e->did = did;
    e->writable = false;
    e->access = access;
    e->varies = true;
    e->vary_min = vmin;
    e->vary_max = vmax;

    if (byte_size == 1) {
        e->data[0] = (uint8_t)initial;
        e->len = 1;
    } else {
        e->data[0] = (uint8_t)(initial >> 8);
        e->data[1] = (uint8_t)(initial & 0xFF);
        e->len = 2;
    }
}

void did_store_restore_from_nv(void) {
    const store_persistence_t *p = store_get_persistence();
    if (!p || !p->did_load) return;

    for (uint8_t i = 0; i < s_did_count; i++) {
        if (!s_dids[i].writable) continue;
        uint8_t nv_len = 0;
        const uint8_t *nv_data = p->did_load(s_dids[i].did, &nv_len);
        if (nv_data && nv_len > 0 && nv_len <= DID_MAX_DATA_LEN) {
            memcpy(s_dids[i].data, nv_data, nv_len);
            s_dids[i].len = nv_len;
        }
    }
}

const did_entry_t *did_store_read(uint16_t did) {
    for (uint8_t i = 0; i < s_did_count; i++) {
        if (s_dids[i].did == did) {
            return &s_dids[i];
        }
    }
    return NULL;
}

bool did_store_write(uint16_t did, const uint8_t *data, uint8_t len) {
    for (uint8_t i = 0; i < s_did_count; i++) {
        if (s_dids[i].did == did) {
            if (!s_dids[i].writable) return false;
            if (len > DID_MAX_DATA_LEN) return false;
            memcpy(s_dids[i].data, data, len);
            s_dids[i].len = len;

            const store_persistence_t *p = store_get_persistence();
            if (p && p->did_save) {
                p->did_save(did, data, len);
            }
            return true;
        }
    }
    return false;
}

uint8_t did_store_count(void) {
    return s_did_count;
}

const did_entry_t *did_store_get_by_index(uint8_t index) {
    if (index >= s_did_count) return NULL;
    return &s_dids[index];
}

/* ── Sensor variation (random walk) ───────────────────────────────── */

void did_store_update_sensors(void) {
    for (uint8_t i = 0; i < s_did_count; i++) {
        did_entry_t *e = &s_dids[i];
        if (!e->varies) continue;

        uint16_t cur;
        if (e->len == 1) {
            cur = e->data[0];
        } else {
            cur = ((uint16_t)e->data[0] << 8) | e->data[1];
        }

        /* Random walk: +/- ~2% of range */
        uint16_t range = e->vary_max - e->vary_min;
        uint16_t step = (range / 50) + 1; /* ~2% */
        uint16_t r = prng16();

        if (r & 1) {
            cur = (cur + step > e->vary_max) ? e->vary_max : cur + step;
        } else {
            cur = (cur < e->vary_min + step) ? e->vary_min : cur - step;
        }

        if (e->len == 1) {
            e->data[0] = (uint8_t)cur;
        } else {
            e->data[0] = (uint8_t)(cur >> 8);
            e->data[1] = (uint8_t)(cur & 0xFF);
        }
    }
}
