/**
 * @file svc_ecu_reset.h
 * @brief UDS Service 0x11 ECUReset
 */
#ifndef SVC_ECU_RESET_H
#define SVC_ECU_RESET_H

#include "uds_types.h"

#define RESET_HARD 0x01U                         /**< Hard reset */
#define RESET_KEY_OFF_ON 0x02U                   /**< Key off/on reset */
#define RESET_SOFT 0x03U                         /**< Soft reset */
#define RESET_ENABLE_RAPID_POWER_SHUTDOWN 0x04U  /**< Enable rapid power shutdown */
#define RESET_DISABLE_RAPID_POWER_SHUTDOWN 0x05U /**< Disable rapid power shutdown */

void svc_ecu_reset(const uds_request_t *req, uds_response_t *resp);
void uds_reset_poll(void);
void on_ecu_reset(uint8_t reset_type);

#endif /* SVC_ECU_RESET_H */
