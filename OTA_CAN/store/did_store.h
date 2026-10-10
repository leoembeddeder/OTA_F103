#ifndef DID_STORE_H
#define DID_STORE_H

#include "store_types.h"

typedef struct {
    uint16_t    did;
    uint8_t     data[DID_MAX_DATA_LEN];
    uint8_t     len;
    bool        writable;
    did_access_t access;        /* Public / Extended / Protected */
    bool        varies;         /* Simulated sensor variation    */
    uint16_t    vary_min;       /* Min value for variation       */
    uint16_t    vary_max;       /* Max value for variation       */
} did_entry_t;

/** Reset the DID store (clears all entries). */
void did_store_init(void);

/** Add a fixed DID to the store. */
void did_store_add(uint16_t did, const uint8_t *data, uint8_t len,
                   bool writable, did_access_t access);

/** Add a varying sensor DID to the store. */
void did_store_add_sensor(uint16_t did, uint8_t byte_size,
                          uint16_t initial, uint16_t vmin, uint16_t vmax,
                          did_access_t access);

/** Read a DID.  Returns pointer to entry or NULL if not found. */
const did_entry_t *did_store_read(uint16_t did);

/** Write a DID.  Returns true on success, false if not found or not writable. */
bool did_store_write(uint16_t did, const uint8_t *data, uint8_t len);

/** Get total number of configured DIDs. */
uint8_t did_store_count(void);

/** Get DID entry by index (for CLI listing). */
const did_entry_t *did_store_get_by_index(uint8_t index);

/**
 * Update all varying (sensor) DIDs.
 * Call periodically (~100ms) to simulate sensor readings.
 */
void did_store_update_sensors(void);

/**
 * Restore writable DIDs from persistent storage.
 * Call after all did_store_add() calls and after store_set_persistence().
 */
void did_store_restore_from_nv(void);

#endif /* DID_STORE_H */
