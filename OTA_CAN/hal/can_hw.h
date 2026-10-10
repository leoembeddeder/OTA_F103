#ifndef UDS_TINY_CAN_HW_H
#define UDS_TINY_CAN_HW_H



#include "can_frame_fd.h"

#include "uds_server.h"
#include "uds_session.h"
#include "uds_callbacks.h"
#include "uds_types.h"
#include "isotp_fd.h"
#include "did_store.h"
#include "dtc_store.h"
#include "store_types.h"
#include "uds_wear_leveling.h"
#include "dtc_nvm_adapter.h"
#include "stm32f103_flash_port.h"

#define ISOTP_RX_ID       0x7E0
#define ISOTP_TX_ID       0x7E8
#define ISOTP_FUNC_RX_ID  0x7DF


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Transmit one CAN frame. Blocks until the frame has been queued
 * into the controller's TX FIFO; does NOT wait for the bus to ack.
 *
 * Returns true if the frame was queued; false if the TX buffer was
 * full or a hardware error was reported.
 */
bool can_hw_send(const can_frame_t *frame);

/**
 * Non-blocking RX. If a frame is available, copy it into *frame and
 * return true. Otherwise return false without modifying *frame.
 */
bool can_hw_receive(can_frame_t *frame);

void UDS_APP(void);


#ifdef __cplusplus
}
#endif

#endif 
