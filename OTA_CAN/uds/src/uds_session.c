#include "uds_session.h"
#include "uds_config.h"
#include "uds_platform_time.h"

#include <string.h>

/* ── Static state ─────────────────────────────────────────────────── */

static uds_session_t  s_session       = UDS_SESSION_DEFAULT;
static uint32_t       s_s3_timer_ms   = 0;

/* Security */
static security_state_t s_sec_state   = SECURITY_LOCKED;
static uint8_t        s_seed[SECURITY_SEED_LEN];
static uint8_t        s_sec_attempts  = 0;
static uint32_t       s_lockout_start = 0;
static const uint8_t  s_secret[]      = SECURITY_SECRET;

/* Transfer */
static transfer_ctx_t s_transfer;

/* Periodic DIDs */
static periodic_rate_t s_periodic[3]; /* 0=slow, 1=medium, 2=fast */

/* Link control */
static uint32_t       s_pending_baud  = 0;
static bool           s_has_pending_baud = false;

/* ── Session management ───────────────────────────────────────────── */

uds_session_t uds_session_get(void) {
    return s_session;
}

void uds_session_set(uds_session_t session) {
    s_session = session;
    s_s3_timer_ms = uds_platform_time_ms();
    uds_security_reset();
    uds_transfer_reset();
    uds_periodic_clear_all();
}

void uds_session_reset(void) {
    uds_session_set(UDS_SESSION_DEFAULT);
}

bool uds_session_check_timeout(void) {
    if (s_session == UDS_SESSION_DEFAULT) {
        return false;
    }
    if (uds_platform_time_expired(s_s3_timer_ms, UDS_S3_SERVER_MS)) {
        uds_session_reset();
        return true;
    }
    return false;
}

void uds_session_refresh(void) {
    s_s3_timer_ms = uds_platform_time_ms();
}

/* ── Security access FSM (4-byte seed, XOR algorithm) ─────────────── */

security_state_t uds_security_get_state(void) {
    return s_sec_state;
}

bool uds_security_is_unlocked(void) {
    return s_sec_state == SECURITY_UNLOCKED;
}

uint8_t uds_security_seed_len(void) {
    return SECURITY_SEED_LEN;
}

const uint8_t *uds_security_request_seed(void) {
    static uint8_t seed_out[SECURITY_SEED_LEN];

    if (s_sec_state == SECURITY_UNLOCKED) {
        memset(seed_out, 0, SECURITY_SEED_LEN);
        return seed_out;
    }

    /* Generate pseudo-random seed from timer */
    uint32_t t = uds_platform_time_ms();
    s_seed[0] = (uint8_t)(t >> 24);
    s_seed[1] = (uint8_t)(t >> 16);
    s_seed[2] = (uint8_t)(t >> 8);
    s_seed[3] = (uint8_t)(t & 0xFF);
    /* Ensure non-zero when locked */
    if (s_seed[0] == 0 && s_seed[1] == 0 && s_seed[2] == 0 && s_seed[3] == 0) {
        s_seed[0] = 0x12;
        s_seed[1] = 0x34;
        s_seed[2] = 0x56;
        s_seed[3] = 0x78;
    }

    memcpy(seed_out, s_seed, SECURITY_SEED_LEN);
    s_sec_state = SECURITY_SEED_SENT;
    return seed_out;
}

bool uds_security_send_key(const uint8_t *key, uint8_t key_len) {
    if (key_len < SECURITY_SEED_LEN) return false;

    /* Expected key: seed[i] XOR secret[i % secret_len] */
    bool match = true;
    for (uint8_t i = 0; i < SECURITY_SEED_LEN; i++) {
        uint8_t expected = s_seed[i] ^ s_secret[i % SECURITY_SECRET_LEN];
        if (key[i] != expected) {
            match = false;
            break;
        }
    }

    if (match) {
        s_sec_state = SECURITY_UNLOCKED;
        s_sec_attempts = 0;
        return true;
    }

    s_sec_attempts++;
    if (s_sec_attempts >= SECURITY_MAX_ATTEMPTS) {
        s_sec_state = SECURITY_LOCKED_OUT;
        s_lockout_start = uds_platform_time_ms();
    } else {
        s_sec_state = SECURITY_LOCKED;
    }
    return false;
}

void uds_security_reset(void) {
    if (s_sec_state != SECURITY_LOCKED_OUT) {
        s_sec_state = SECURITY_LOCKED;
    }
    memset(s_seed, 0, SECURITY_SEED_LEN);
}

void uds_security_poll(void) {
    if (s_sec_state == SECURITY_LOCKED_OUT) {
        if (uds_platform_time_expired(s_lockout_start, SECURITY_LOCKOUT_MS)) {
            s_sec_state = SECURITY_LOCKED;
            s_sec_attempts = 0;
        }
    }
}

/* ── Transfer state ───────────────────────────────────────────────── */

transfer_ctx_t *uds_transfer_get(void) {
    return &s_transfer;
}

void uds_transfer_reset(void) {
    memset(&s_transfer, 0, sizeof(s_transfer));
    s_transfer.state = TRANSFER_IDLE;
}

/* ── Periodic DIDs ────────────────────────────────────────────────── */

periodic_rate_t *uds_periodic_get(uint8_t rate_index) {
    if (rate_index > 2) return NULL;
    return &s_periodic[rate_index];
}

void uds_periodic_clear_all(void) {
    for (int i = 0; i < 3; i++) {
        s_periodic[i].count = 0;
        s_periodic[i].last_ms = 0;
    }
}

/* ── Link control ─────────────────────────────────────────────────── */

void uds_link_set_pending_baud(uint32_t baud_kbps) {
    s_pending_baud = baud_kbps;
    s_has_pending_baud = true;
}

bool uds_link_has_pending(void) {
    return s_has_pending_baud;
}

uint32_t uds_link_get_pending_baud(void) {
    return s_pending_baud;
}

void uds_link_clear_pending(void) {
    s_has_pending_baud = false;
    s_pending_baud = 0;
}

/* ── Global init ──────────────────────────────────────────────────── */

void uds_state_init(void) {
    s_session = UDS_SESSION_DEFAULT;
    s_s3_timer_ms = uds_platform_time_ms();
    s_sec_state = SECURITY_LOCKED;
    memset(s_seed, 0, SECURITY_SEED_LEN);
    s_sec_attempts = 0;
    uds_transfer_reset();
    uds_periodic_clear_all();
    uds_link_clear_pending();
}
