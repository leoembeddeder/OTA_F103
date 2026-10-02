#ifndef DTC_STORE_H
#define DTC_STORE_H

#include "store_types.h"

/*
 * DTC status byte bits (ISO 14229-1):
 *   bit 0: testFailed
 *   bit 1: testFailedThisOperationCycle
 *   bit 2: pendingDTC
 *   bit 3: confirmedDTC
 *   bit 4: testNotCompletedSinceLastClear
 *   bit 5: testFailedSinceLastClear
 *   bit 6: testNotCompletedThisOperationCycle
 *   bit 7: warningIndicatorRequested
 */
#define DTC_STATUS_TEST_FAILED              0x01
#define DTC_STATUS_TEST_FAILED_THIS_CYCLE   0x02
#define DTC_STATUS_PENDING                  0x04
#define DTC_STATUS_CONFIRMED                0x08
#define DTC_STATUS_NOT_COMPLETED_CLEAR      0x10
#define DTC_STATUS_FAILED_SINCE_CLEAR       0x20
#define DTC_STATUS_NOT_COMPLETED_CYCLE      0x40
#define DTC_STATUS_WARNING_INDICATOR        0x80

typedef struct {
    uint32_t dtc;                           /* 3-byte DTC (lower 24 bits)  */
    uint8_t  status;                        /* DTC status byte             */
    bool     has_snapshot;                   /* Snapshot data available     */
    uint8_t  snapshot[DTC_SNAPSHOT_SIZE];    /* Freeze frame data           */
    uint8_t  snapshot_len;
    bool     has_extdata;                    /* Extended data available     */
    uint8_t  extdata[DTC_EXTDATA_SIZE];     /* Extended diagnostic data    */
    uint8_t  extdata_len;
} dtc_entry_t;

/** Reset the DTC store (clears all entries). */
void dtc_store_init(void);

/** Add a DTC with optional snapshot and extended data. */
void dtc_store_add(uint32_t dtc, uint8_t status,
                   const uint8_t *snap, uint8_t snap_len,
                   const uint8_t *ext, uint8_t ext_len);

/**
 * Restore DTC statuses from persistent storage.
 * Call after all dtc_store_add() calls and after store_set_persistence().
 */
void dtc_store_restore_from_nv(void);

/** Get number of DTCs matching a status mask. */
uint16_t dtc_store_count_by_mask(uint8_t status_mask);

/** Get DTC entry by index. */
const dtc_entry_t *dtc_store_get_by_index(uint8_t index);

/** Find DTC entry by DTC number.  Returns NULL if not found. */
const dtc_entry_t *dtc_store_find(uint32_t dtc);

/** Get total DTC count. */
uint8_t dtc_store_count(void);

/** Get the supported status mask (0xFF for simulator). */
uint8_t dtc_store_get_status_mask(void);

/** Clear all DTCs (reset status bytes). */
void dtc_store_clear_all(void);

/** Clear DTCs matching a specific 3-byte group (0xFFFFFF = all). */
void dtc_store_clear_group(uint32_t group);

#endif /* DTC_STORE_H */
