#ifndef ISOTP_CONFIG_H
#define ISOTP_CONFIG_H

/*
 * ISO-TP library configuration.
 *
 * If the application defines ISOTP_APP_CONFIG (via -D compiler flag),
 * that header is included first and can override any default below.
 * Otherwise, sensible defaults are used.
 */

#ifdef ISOTP_APP_CONFIG
#include ISOTP_APP_CONFIG
#endif

/* ── Buffer sizes ─────────────────────────────────────────────────── */
#ifndef ISOTP_MAX_PAYLOAD
#define ISOTP_MAX_PAYLOAD   4095
#endif

#ifndef ISOTP_RX_BUF_SIZE
#define ISOTP_RX_BUF_SIZE   4352
#endif

#ifndef ISOTP_TX_BUF_SIZE
#define ISOTP_TX_BUF_SIZE   512
#endif

/* ── Flow Control parameters ──────────────────────────────────────── */
#ifndef ISOTP_BS
#define ISOTP_BS            0       /* Block Size in FC (0 = no limit) */
#endif

#ifndef ISOTP_STMIN_MS
#define ISOTP_STMIN_MS      0       /* STmin in FC frames (ms)         */
#endif

/* ── Timeouts ─────────────────────────────────────────────────────── */
#ifndef ISOTP_CF_TIMEOUT_MS
#define ISOTP_CF_TIMEOUT_MS 1000    /* Timeout waiting for next CF     */
#endif

#ifndef ISOTP_FC_TIMEOUT_MS
#define ISOTP_FC_TIMEOUT_MS 1000    /* Timeout waiting for FC          */
#endif

#ifndef ISOTP_WFT_MAX
#define ISOTP_WFT_MAX       5       /* Max wait-frame transmissions    */
#endif

#endif /* ISOTP_CONFIG_H */
