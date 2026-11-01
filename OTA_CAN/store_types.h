#ifndef STORE_TYPES_H
#define STORE_TYPES_H

/*
 * Store library types, persistence callbacks, and configuration.
 *
 * If the application defines STORE_APP_CONFIG (via -D compiler flag),
 * that header is included first to override defaults.
 */

#ifdef STORE_APP_CONFIG
#include STORE_APP_CONFIG
#endif

#include <stdint.h>
#include <stdbool.h>

/* ── DID / DTC / IO limits (defaults, overridable via app config) ── */

#ifndef DID_MAX_ENTRIES
#define DID_MAX_ENTRIES     40
#endif

#ifndef DID_MAX_DATA_LEN
#define DID_MAX_DATA_LEN    32
#endif

#ifndef DTC_MAX_ENTRIES
#define DTC_MAX_ENTRIES     16
#endif

#ifndef DTC_SNAPSHOT_SIZE
#define DTC_SNAPSHOT_SIZE   8
#endif

#ifndef DTC_EXTDATA_SIZE
#define DTC_EXTDATA_SIZE    8
#endif

#ifndef IO_OUTPUT_MAX
#define IO_OUTPUT_MAX       8
#endif

#ifndef IO_OUTPUT_MAX_SIZE
#define IO_OUTPUT_MAX_SIZE  4
#endif

/* ── DID access levels (moved from uds_types.h — store concept) ── */

typedef enum {
    DID_ACCESS_PUBLIC,       /* Readable in any session                */
    DID_ACCESS_EXTENDED,     /* Requires extended/engineering session  */
    DID_ACCESS_PROTECTED,    /* Requires security unlock              */
} did_access_t;

/* ── Persistence callbacks ────────────────────────────────────────── */

typedef struct {
    /** Save a DID value to persistent storage. */
    void (*did_save)(uint16_t did, const uint8_t *data, uint8_t len);

    /** Load a DID value from persistent storage. Returns NULL if not found. */
    const uint8_t *(*did_load)(uint16_t did, uint8_t *len);

    /** Save a DTC status to persistent storage. */
    void (*dtc_save)(uint32_t dtc_number, uint8_t status);

    /** Load a DTC status from persistent storage. Returns 0xFF if not found. */
    uint8_t (*dtc_load)(uint32_t dtc_number);
} store_persistence_t;

/**
 * Set persistence callbacks.  Call once at init before store operations.
 * If NULL, stores operate without persistence (RAM-only).
 */
void store_set_persistence(const store_persistence_t *p);

/**
 * Get current persistence callbacks (for internal use by store modules).
 */
const store_persistence_t *store_get_persistence(void);

#endif /* STORE_TYPES_H */
