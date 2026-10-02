#ifndef ISOTP_H
#define ISOTP_H

#include "isotp_types.h"
#include "can_frame.h"

/**
 * Initialise an ISO-TP channel with the given CAN IDs and TX callback.
 */
void isotp_init(isotp_channel_t *ch, uint32_t rx_id, uint32_t tx_id,
                isotp_tx_fn tx_cb, void *tx_user);

/**
 * Feed a received CAN frame into the ISO-TP state machine.
 * Call this from the CAN RX polling loop for every frame whose ID
 * matches ch->rx_id.
 */
void isotp_on_rx(isotp_channel_t *ch, const can_frame_t *frame);

/**
 * Run periodic tasks (timeout checks, CF transmission).
 * Call this regularly from the CAN I/O loop.
 */
void isotp_poll(isotp_channel_t *ch);

/**
 * Begin sending a multi-byte ISO-TP message.
 * The data is copied into the channel's TX buffer.
 * Returns ISOTP_OK if started, ISOTP_BUSY if a TX is in progress.
 */
isotp_result_t isotp_send(isotp_channel_t *ch, const uint8_t *data, uint16_t len);

/**
 * Check if a complete message has been received.
 * Returns true if a full message is available.
 */
bool isotp_rx_ready(const isotp_channel_t *ch);

/**
 * Get pointer and length of the received message.
 * Only valid when isotp_rx_ready() returns true.
 */
const uint8_t *isotp_rx_data(const isotp_channel_t *ch, uint16_t *len);

/**
 * Release the RX buffer after processing the received message.
 * Must be called after consuming the data.
 */
void isotp_rx_done(isotp_channel_t *ch);

/**
 * Check if the TX channel is idle (ready to send a new message).
 */
bool isotp_tx_idle(const isotp_channel_t *ch);

#endif /* ISOTP_H */
