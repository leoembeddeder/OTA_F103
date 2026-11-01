#ifndef UDS_SESSION_H
#define UDS_SESSION_H

#include "uds_types.h"

/* ── Session management ───────────────────────────────────────────── */

/** Get current diagnostic session. */
uds_session_t uds_session_get(void);

/** Set diagnostic session and reset S3 timer. */
void uds_session_set(uds_session_t session);

/** Reset to default session (called on S3 timeout or ECUReset). */
void uds_session_reset(void);

/** Call periodically; returns true if S3 timeout expired (non-default session). */
bool uds_session_check_timeout(void);

/** Refresh S3 timer (call on every valid request). */
void uds_session_refresh(void);

/* ── Security access FSM ──────────────────────────────────────────── */

typedef enum {
    SECURITY_LOCKED,
    SECURITY_SEED_SENT,
    SECURITY_UNLOCKED,
    SECURITY_LOCKED_OUT,
} security_state_t;

/** Get current security state. */
security_state_t uds_security_get_state(void);

/** Check if ECU is security-unlocked. */
bool uds_security_is_unlocked(void);

/** Generate and store a new seed.  Returns pointer to SECURITY_SEED_LEN-byte seed. */
const uint8_t *uds_security_request_seed(void);

/** Get seed length. */
uint8_t uds_security_seed_len(void);

/** Validate key against current seed.  Returns true on success. */
bool uds_security_send_key(const uint8_t *key, uint8_t key_len);

/** Reset security to locked (called on session change). */
void uds_security_reset(void);

/** Check/manage lockout timer.  Call periodically. */
void uds_security_poll(void);

/* ── Transfer state (0x34/0x35/0x36/0x37) ─────────────────────────── */

typedef enum {
    TRANSFER_IDLE,
    TRANSFER_UPLOAD,
    TRANSFER_APP_MANAGED,       /* App-managed transfer (FW OTA, etc.) */
} transfer_state_t;

typedef struct {
    transfer_state_t state;
    uint32_t address;
    uint32_t total_size;
    uint32_t transferred;       /* Bytes transferred so far          */
    uint8_t  block_seq;
} transfer_ctx_t;

/** Get pointer to transfer context. */
transfer_ctx_t *uds_transfer_get(void);

/** Reset transfer state. */
void uds_transfer_reset(void);

/* ── Periodic DID state (0x2A) ────────────────────────────────────── */

typedef struct {
    uint8_t  pids[PERIODIC_MAX_PIDS];
    uint8_t  count;
    uint32_t last_ms;
} periodic_rate_t;

/** Get periodic rate contexts (slow=0, medium=1, fast=2). */
periodic_rate_t *uds_periodic_get(uint8_t rate_index);

/** Clear all periodic transmissions. */
void uds_periodic_clear_all(void);

/* ── Link control state (0x87) ────────────────────────────────────── */

/** Get/set pending baud rate (verified but not yet transitioned). */
void uds_link_set_pending_baud(uint32_t baud_kbps);
bool uds_link_has_pending(void);
uint32_t uds_link_get_pending_baud(void);
void uds_link_clear_pending(void);

/** Initialise all session/security/transfer state. */
void uds_state_init(void);

#endif /* UDS_SESSION_H */
