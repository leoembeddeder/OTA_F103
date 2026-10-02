#ifndef SVC_ECU_RESET_H
#define SVC_ECU_RESET_H

#include "uds_types.h"

/** @name 0x11 ECUReset Sub-functions (ISO 14229-1) */
/**@{*/
#define RESET_HARD 0x01U                         /**< Hard reset */
#define RESET_KEY_OFF_ON 0x02U                   /**< Key off/on reset */
#define RESET_SOFT 0x03U                         /**< Soft reset */
#define RESET_ENABLE_RAPID_POWER_SHUTDOWN 0x04U  /**< Enable rapid power shutdown */
#define RESET_DISABLE_RAPID_POWER_SHUTDOWN 0x05U /**< Disable rapid power shutdown */
#define DEFAULT_RESET_TX_WAIT_MS 50U /**< Default TX ECU reset wait time in ms */

typedef struct               
{
    uint8_t reset_type_requested;       /**< Pending ECU reset type requested */
	uint32_t reset_wait_elapsed_ms;     /**< Internal reset delay accumulator in ms. */
}uds_reset_t;

extern uds_reset_t ecu_reset;

void svc_ecu_reset(const uds_request_t *req, uds_response_t *resp);
void uds_reset_poll(void);

#endif
