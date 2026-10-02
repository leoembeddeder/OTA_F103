#include "isotp.h"
#include "isotp_config.h"
#include <uds_platform_time.h>

#include <string.h>

/* ── Helpers ──────────────────────────────────────────────────────── */

static bool send_can_frame(isotp_channel_t *ch, uint32_t id,
                           const uint8_t *data, uint8_t dlc) {
    can_frame_t f;
    memset(&f, 0, sizeof(f));
    f.id     = id;
    f.dlc    = dlc;
    f.is_ext = (id > 0x7FF);
    f.is_rtr = false;
    if (dlc > 0) {
        memcpy(f.data, data, dlc);
    }
    return ch->tx_cb(&f, ch->tx_user);
}

/* Pad unused bytes with 0xCC (ISO-TP padding) */
static void pad_frame(uint8_t *data, uint8_t used, uint8_t total) {
    for (uint8_t i = used; i < total; i++) {
        data[i] = 0xCC;
    }
}

/* ── Flow Control transmission (we send FC to remote) ─────────────── */

static void send_fc(isotp_channel_t *ch, isotp_fc_status_t fs) {
    uint8_t buf[8];
    buf[0] = (ISOTP_PCI_FC << 4) | (fs & 0x0F);
    buf[1] = ISOTP_BS;          /* Block Size */
    buf[2] = ISOTP_STMIN_MS;    /* STmin */
    pad_frame(buf, 3, 8);
    send_can_frame(ch, ch->tx_id, buf, 8);
}

/* ── Initialisation ───────────────────────────────────────────────── */

void isotp_init(isotp_channel_t *ch, uint32_t rx_id, uint32_t tx_id,
                isotp_tx_fn tx_cb, void *tx_user) {
    memset(ch, 0, sizeof(*ch));
    ch->rx_id   = rx_id;
    ch->tx_id   = tx_id;
    ch->tx_cb   = tx_cb;
    ch->tx_user = tx_user;
    ch->rx.state = ISOTP_RX_IDLE;
    ch->tx.state = ISOTP_TX_IDLE;
}

/* ── RX state machine ─────────────────────────────────────────────── */

static void handle_sf(isotp_channel_t *ch, const can_frame_t *frame) {
    uint8_t sf_dl = frame->data[0] & 0x0F;
    if (sf_dl == 0 || sf_dl > 7 || sf_dl > frame->dlc - 1) {
        return; /* Invalid */
    }

    memcpy(ch->rx.buf, &frame->data[1], sf_dl);
    ch->rx.msg_len = sf_dl;
    ch->rx.offset  = sf_dl;
    ch->rx.state   = ISOTP_RX_IDLE; /* Complete message */
}

static void handle_ff(isotp_channel_t *ch, const can_frame_t *frame) {
    uint16_t ff_dl = ((uint16_t)(frame->data[0] & 0x0F) << 8) | frame->data[1];
    if (ff_dl > ISOTP_RX_BUF_SIZE) {
        /* Message too large -- send FC overflow */
        send_fc(ch, ISOTP_FC_OVFLW);
        return;
    }

    /* Copy first 6 data bytes */
    uint8_t first_bytes = (frame->dlc > 2) ? (frame->dlc - 2) : 0;
    if (first_bytes > 6) first_bytes = 6;
    memcpy(ch->rx.buf, &frame->data[2], first_bytes);

    ch->rx.msg_len  = ff_dl;
    ch->rx.offset   = first_bytes;
    ch->rx.seq      = 1;
    ch->rx.bs       = ISOTP_BS;
    ch->rx.bs_count = 0;
    ch->rx.state    = ISOTP_RX_WAIT_CF;
    ch->rx.timer_ms = uds_platform_time_ms();

    /* Send Flow Control - Continue To Send */
    send_fc(ch, ISOTP_FC_CTS);
}

static void handle_cf(isotp_channel_t *ch, const can_frame_t *frame) {
    if (ch->rx.state != ISOTP_RX_WAIT_CF) {
        return; /* Unexpected CF */
    }

    uint8_t sn = frame->data[0] & 0x0F;
    if (sn != (ch->rx.seq & 0x0F)) {
        /* Sequence number mismatch -- abort */
        ch->rx.state = ISOTP_RX_IDLE;
        return;
    }

    uint16_t remaining = ch->rx.msg_len - ch->rx.offset;
    uint8_t  cf_bytes  = (frame->dlc > 1) ? (frame->dlc - 1) : 0;
    if (cf_bytes > 7) cf_bytes = 7;
    if (cf_bytes > remaining) cf_bytes = (uint8_t)remaining;

    memcpy(&ch->rx.buf[ch->rx.offset], &frame->data[1], cf_bytes);
    ch->rx.offset += cf_bytes;
    ch->rx.seq++;
    ch->rx.bs_count++;
    ch->rx.timer_ms = uds_platform_time_ms();

    if (ch->rx.offset >= ch->rx.msg_len) {
        /* Message complete */
        ch->rx.state = ISOTP_RX_IDLE;
        return;
    }

    /* If block size reached, send another FC */
    if (ch->rx.bs != 0 && ch->rx.bs_count >= ch->rx.bs) {
        ch->rx.bs_count = 0;
        send_fc(ch, ISOTP_FC_CTS);
    }
}

/* ── TX state machine -- handle incoming FC ───────────────────────── */

static void handle_fc_rx(isotp_channel_t *ch, const can_frame_t *frame) {
    if (ch->tx.state != ISOTP_TX_WAIT_FC) {
        return; /* Unexpected FC */
    }

    isotp_fc_status_t fs = (isotp_fc_status_t)(frame->data[0] & 0x0F);

    switch (fs) {
    case ISOTP_FC_CTS:
        ch->tx.bs     = frame->data[1];
        ch->tx.stmin  = frame->data[2];
        ch->tx.bs_count = 0;
        ch->tx.state  = ISOTP_TX_SEND_CF;
        ch->tx.timer_ms = uds_platform_time_ms();
        break;

    case ISOTP_FC_WAIT:
        ch->tx.wft_count++;
        if (ch->tx.wft_count > ISOTP_WFT_MAX) {
            ch->tx.state = ISOTP_TX_IDLE; /* Abort */
        } else {
            ch->tx.timer_ms = uds_platform_time_ms(); /* Restart timer */
        }
        break;

    case ISOTP_FC_OVFLW:
    default:
        ch->tx.state = ISOTP_TX_IDLE; /* Abort */
        break;
    }
}

/* ── Frame dispatch ───────────────────────────────────────────────── */

void isotp_on_rx(isotp_channel_t *ch, const can_frame_t *frame) {
    if (frame->dlc < 1) return;

    isotp_pci_type_t pci = (isotp_pci_type_t)(frame->data[0] >> 4);

    switch (pci) {
    case ISOTP_PCI_SF:
        handle_sf(ch, frame);
        break;
    case ISOTP_PCI_FF:
        if (frame->dlc >= 2) handle_ff(ch, frame);
        break;
    case ISOTP_PCI_CF:
        handle_cf(ch, frame);
        break;
    case ISOTP_PCI_FC:
        handle_fc_rx(ch, frame);
        break;
    }
}

/* ── TX: send consecutive frames ──────────────────────────────────── */

static void tx_send_cf(isotp_channel_t *ch) {
    uint8_t buf[8];
    uint16_t remaining = ch->tx.msg_len - ch->tx.offset;
    uint8_t  cf_bytes  = (remaining > 7) ? 7 : (uint8_t)remaining;

    buf[0] = (ISOTP_PCI_CF << 4) | (ch->tx.seq & 0x0F);
    memcpy(&buf[1], &ch->tx.buf[ch->tx.offset], cf_bytes);
    pad_frame(buf, 1 + cf_bytes, 8);

    if (!send_can_frame(ch, ch->tx_id, buf, 8)) {
        return; /* TX buffer full, retry next poll */
    }

    ch->tx.offset += cf_bytes;
    ch->tx.seq++;
    ch->tx.bs_count++;
    ch->tx.timer_ms = uds_platform_time_ms();

    if (ch->tx.offset >= ch->tx.msg_len) {
        /* Transmission complete */
        ch->tx.state = ISOTP_TX_IDLE;
        return;
    }

    /* Block size check: if BS != 0 and we've sent BS frames, wait for FC */
    if (ch->tx.bs != 0 && ch->tx.bs_count >= ch->tx.bs) {
        ch->tx.state = ISOTP_TX_WAIT_FC;
        ch->tx.wft_count = 0;
        ch->tx.timer_ms = uds_platform_time_ms();
    }
}

/* ── Poll (call periodically) ─────────────────────────────────────── */

void isotp_poll(isotp_channel_t *ch) {
    /* RX timeout check */
    if (ch->rx.state == ISOTP_RX_WAIT_CF) {
        if (uds_platform_time_expired(ch->rx.timer_ms, ISOTP_CF_TIMEOUT_MS)) {
            ch->rx.state = ISOTP_RX_IDLE; /* Timeout -- abort */
        }
    }

    /* TX timeout check */
    if (ch->tx.state == ISOTP_TX_WAIT_FC) {
        if (uds_platform_time_expired(ch->tx.timer_ms, ISOTP_FC_TIMEOUT_MS)) {
            ch->tx.state = ISOTP_TX_IDLE; /* Timeout -- abort */
        }
    }

    /* TX: send CFs respecting STmin */
    if (ch->tx.state == ISOTP_TX_SEND_CF) {
        if (uds_platform_time_elapsed(ch->tx.timer_ms) >= ch->tx.stmin) {
            tx_send_cf(ch);
        }
    }
}

/* ── Begin a new transmission ─────────────────────────────────────── */

isotp_result_t isotp_send(isotp_channel_t *ch, const uint8_t *data, uint16_t len) {
    if (ch->tx.state != ISOTP_TX_IDLE) {
        return ISOTP_BUSY;
    }

    if (len == 0 || len > ISOTP_TX_BUF_SIZE) {
        return ISOTP_ERROR;
    }

    if (len <= 7) {
        /* Single Frame */
        uint8_t buf[8];
        buf[0] = (ISOTP_PCI_SF << 4) | (len & 0x0F);
        memcpy(&buf[1], data, len);
        pad_frame(buf, 1 + len, 8);
        if (!send_can_frame(ch, ch->tx_id, buf, 8)) {
            return ISOTP_ERROR;
        }
        /* TX remains idle -- SF is fire-and-forget */
        return ISOTP_OK;
    }

    /* Multi-frame: First Frame */
    memcpy(ch->tx.buf, data, len);
    ch->tx.msg_len  = len;

    uint8_t buf[8];
    buf[0] = (ISOTP_PCI_FF << 4) | ((len >> 8) & 0x0F);
    buf[1] = len & 0xFF;
    uint8_t first_bytes = (len > 6) ? 6 : (uint8_t)len;
    memcpy(&buf[2], data, first_bytes);
    pad_frame(buf, 2 + first_bytes, 8);

    if (!send_can_frame(ch, ch->tx_id, buf, 8)) {
        return ISOTP_ERROR;
    }

    ch->tx.offset    = first_bytes;
    ch->tx.seq       = 1;
    ch->tx.state     = ISOTP_TX_WAIT_FC;
    ch->tx.wft_count = 0;
    ch->tx.timer_ms  = uds_platform_time_ms();

    return ISOTP_OK;
}

/* ── Query helpers ────────────────────────────────────────────────── */

bool isotp_rx_ready(const isotp_channel_t *ch) {
    return (ch->rx.state == ISOTP_RX_IDLE &&
            ch->rx.offset > 0 &&
            ch->rx.offset >= ch->rx.msg_len);
}

const uint8_t *isotp_rx_data(const isotp_channel_t *ch, uint16_t *len) {
    if (len) *len = ch->rx.msg_len;
    return ch->rx.buf;
}

void isotp_rx_done(isotp_channel_t *ch) {
    ch->rx.offset  = 0;
    ch->rx.msg_len = 0;
}

bool isotp_tx_idle(const isotp_channel_t *ch) {
    return ch->tx.state == ISOTP_TX_IDLE;
}
