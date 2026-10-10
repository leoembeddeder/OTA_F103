#ifndef UDS_LIB_CONFIG_H
#define UDS_LIB_CONFIG_H

/*
 * UDS library configuration defaults.
 *
 * If the application defines UDS_APP_CONFIG (via -D compiler flag),
 * that header is included first to override defaults.
 */

#ifdef UDS_APP_CONFIG
#include UDS_APP_CONFIG
#endif

#include <stdint.h>

/* ── UDS timing ───────────────────────────────────────────────────── */
#ifndef UDS_P2_SERVER_MS
#define UDS_P2_SERVER_MS    50
#endif

#ifndef UDS_P2_STAR_MS
#define UDS_P2_STAR_MS      5000
#endif

#ifndef UDS_S3_SERVER_MS
#define UDS_S3_SERVER_MS    5000
#endif

/* ── Security access ──────────────────────────────────────────────── */
#ifndef SECURITY_SEED_LEN
#define SECURITY_SEED_LEN   4
#endif

#ifndef SECURITY_SECRET
#define SECURITY_SECRET     { 0xCA, 0xFE, 0xBA, 0xBE }
#endif

#ifndef SECURITY_SECRET_LEN
#define SECURITY_SECRET_LEN 4
#endif

#ifndef SECURITY_MAX_ATTEMPTS
#define SECURITY_MAX_ATTEMPTS 3
#endif

#ifndef SECURITY_LOCKOUT_MS
#define SECURITY_LOCKOUT_MS 10000
#endif

/* ── Transfer (0x34/0x35/0x36/0x37) ───────────────────────────────── */
#ifndef TRANSFER_MAX_BLOCK
#define TRANSFER_MAX_BLOCK  4094
#endif

/* ── Dynamic DIDs (0x2C) ──────────────────────────────────────────── */
#ifndef DDID_MAX_ENTRIES
#define DDID_MAX_ENTRIES    8
#endif

#ifndef DDID_MAX_SOURCES
#define DDID_MAX_SOURCES    4
#endif

#ifndef DDID_RANGE_START
#define DDID_RANGE_START    0xF200
#endif

#ifndef DDID_RANGE_END
#define DDID_RANGE_END      0xF3FF
#endif

/* ── Periodic DIDs (0x2A) ─────────────────────────────────────────── */
#ifndef PERIODIC_MAX_PIDS
#define PERIODIC_MAX_PIDS   8
#endif

#ifndef PERIODIC_SLOW_MS
#define PERIODIC_SLOW_MS    1000
#endif

#ifndef PERIODIC_MEDIUM_MS
#define PERIODIC_MEDIUM_MS  200
#endif

#ifndef PERIODIC_FAST_MS
#define PERIODIC_FAST_MS    100
#endif

/* ── Link Control (0x87) ──────────────────────────────────────────── */
#ifndef LINK_DEFAULT_BAUD
#define LINK_DEFAULT_BAUD   500
#endif

/* ── Max services ─────────────────────────────────────────────────── */
#ifndef UDS_MAX_SERVICES
#define UDS_MAX_SERVICES    24
#endif

/* ── ISO-TP buf sizes (needed for response struct) ────────────────── */
#ifndef ISOTP_TX_BUF_SIZE
#define ISOTP_TX_BUF_SIZE   512
#endif

#ifndef ISOTP_RX_BUF_SIZE
#define ISOTP_RX_BUF_SIZE   4352
#endif

#endif /* UDS_LIB_CONFIG_H */
