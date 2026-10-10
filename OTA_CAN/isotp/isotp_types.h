#ifndef ISOTP_TYPES_H
#define ISOTP_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "isotp_config.h"
#include "can_frame.h"

/* ── TX callback type ─────────────────────────────────────────────── */
typedef bool (*isotp_tx_fn)(const can_frame_t *frame, void *user);

/* ── PCI (Protocol Control Information) types ─────────────────────── */
typedef enum {
    ISOTP_PCI_SF = 0,   /* Single Frame       */
    ISOTP_PCI_FF = 1,   /* First Frame        */
    ISOTP_PCI_CF = 2,   /* Consecutive Frame  */
    ISOTP_PCI_FC = 3    /* Flow Control       */
} isotp_pci_type_t;

/* ── Flow Control flow status ─────────────────────────────────────── */
typedef enum {
    ISOTP_FC_CTS   = 0, /* Continue To Send   */
    ISOTP_FC_WAIT  = 1, /* Wait               */
    ISOTP_FC_OVFLW = 2  /* Overflow / abort   */
} isotp_fc_status_t;

/* ── ISO-TP RX state machine states ───────────────────────────────── */
typedef enum {
    ISOTP_RX_IDLE,
    ISOTP_RX_WAIT_CF,      /* Received FF, waiting for CFs */
} isotp_rx_state_t;

/* ── ISO-TP TX state machine states ───────────────────────────────── */
typedef enum {
    ISOTP_TX_IDLE,
    ISOTP_TX_WAIT_FC,      /* Sent FF, waiting for FC      */
    ISOTP_TX_SEND_CF,      /* Sending consecutive frames    */
} isotp_tx_state_t;

/* ── RX context ───────────────────────────────────────────────────── */
typedef struct {
    isotp_rx_state_t state;
    uint8_t  buf[ISOTP_RX_BUF_SIZE];
    uint16_t msg_len;       /* Total expected message length */
    uint16_t offset;        /* Bytes received so far         */
    uint8_t  seq;           /* Expected sequence number      */
    uint8_t  bs;            /* Block size from our FC        */
    uint8_t  bs_count;      /* Frames received in this block */
    uint32_t timer_ms;      /* Timeout start timestamp       */
} isotp_rx_ctx_t;

/* ── TX context ───────────────────────────────────────────────────── */
typedef struct {
    isotp_tx_state_t state;
    uint8_t  buf[ISOTP_TX_BUF_SIZE];
    uint16_t msg_len;       /* Total message length          */
    uint16_t offset;        /* Bytes sent so far             */
    uint8_t  seq;           /* Next sequence number           */
    uint8_t  bs;            /* Block size from remote FC      */
    uint8_t  bs_count;      /* Frames sent in current block   */
    uint8_t  stmin;         /* STmin from remote FC (ms)      */
    uint32_t timer_ms;      /* Timeout / STmin timer start    */
    uint8_t  wft_count;     /* Wait-frame count              */
} isotp_tx_ctx_t;

/* ── Complete ISO-TP channel ──────────────────────────────────────── */
typedef struct {
    isotp_rx_ctx_t rx;
    isotp_tx_ctx_t tx;
    uint32_t rx_id;         /* CAN ID we receive on          */
    uint32_t tx_id;         /* CAN ID we transmit on         */
    isotp_tx_fn tx_cb;      /* TX callback (NULL = error)     */
    void       *tx_user;    /* User context for callback      */
} isotp_channel_t;

/* ── Result codes for isotp_process / isotp_send ──────────────────── */
typedef enum {
    ISOTP_OK,
    ISOTP_BUSY,
    ISOTP_TIMEOUT,
    ISOTP_OVERFLOW,
    ISOTP_ERROR
} isotp_result_t;

#endif /* ISOTP_TYPES_H */
