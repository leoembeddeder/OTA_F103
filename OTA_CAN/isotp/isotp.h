/**
 * @file isotp.h
 * @brief ISO 15765-2 ISO-TP Transport Layer API (supporting CAN 2.0B & CAN-FD 64-byte)
 */

#ifndef ISOTP_H
#define ISOTP_H

#include <stdint.h>
#include <stdbool.h>
#include "can_frame_fd.h"
#include "isotp_fd.h"
#include "uds_platform_time.h"

/* Type aliases for seamless compatibility */
typedef isotp_fd_channel_t isotp_channel_t;
typedef isotp_fd_tx_fn      isotp_tx_fn;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise an ISO-TP channel with CAN IDs and TX callback (classic CAN 2.0B).
 */
static inline void isotp_init(isotp_channel_t *ch, uint32_t rx_id, uint32_t tx_id,
                              isotp_tx_fn tx_cb, void *tx_user) {
    isotp_fd_init(ch, rx_id, tx_id, false, tx_cb, tx_user);
}

/**
 * @brief Initialise an ISO-TP channel with optional CAN-FD 64-byte support.
 */
static inline void isotp_init_fd(isotp_channel_t *ch, uint32_t rx_id, uint32_t tx_id,
                                 bool use_fd, isotp_tx_fn tx_cb, void *tx_user) {
    isotp_fd_init(ch, rx_id, tx_id, use_fd, tx_cb, tx_user);
}

/**
 * @brief Feed a received CAN / CAN-FD frame into the ISO-TP state machine.
 */
static inline void isotp_on_rx(isotp_channel_t *ch, const can_frame_t *frame) {
    isotp_fd_on_rx(ch, frame, uds_platform_time_ms());
}

/**
 * @brief Run periodic tasks (timeout checks, CF transmission).
 */
static inline void isotp_poll(isotp_channel_t *ch) {
    isotp_fd_poll(ch, uds_platform_time_ms());
}

/**
 * @brief Begin sending an ISO-TP message (up to 4096 bytes or CAN-FD).
 */
static inline isotp_result_t isotp_send(isotp_channel_t *ch, const uint8_t *data, uint16_t len) {
    return isotp_fd_send(ch, data, (uint32_t)len, uds_platform_time_ms());
}

/**
 * @brief Check if a complete message has been received.
 */
static inline bool isotp_rx_ready(const isotp_channel_t *ch) {
    return isotp_fd_rx_ready(ch);
}

/**
 * @brief Get pointer and length of the received message.
 */
static inline const uint8_t *isotp_rx_data(const isotp_channel_t *ch, uint16_t *len) {
    uint32_t l = 0U;
    const uint8_t *ptr = isotp_fd_rx_data(ch, &l);
    if (len != NULL) {
        *len = (uint16_t)l;
    }
    return ptr;
}

/**
 * @brief Release the RX buffer after processing the received message.
 */
static inline void isotp_rx_done(isotp_channel_t *ch) {
    isotp_fd_rx_done(ch);
}

/**
 * @brief Check if the TX channel is idle.
 */
static inline bool isotp_tx_idle(const isotp_channel_t *ch) {
    return isotp_fd_tx_idle(ch);
}

#ifdef __cplusplus
}
#endif

#endif /* ISOTP_H */
