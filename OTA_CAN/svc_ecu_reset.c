#include "svc_ecu_reset.h"
#include "uds_session.h"
#include "uds_callbacks.h"
#include "uds_platform_time.h"
#include "main.h"

/*
 * 0x11 ECUReset
 * Sub 01=hardReset, 02=keyOff/On, 03=softReset
 *
 * Reset via callback — no Pico SDK / watchdog dependency in library.
 */
uds_reset_t ecu_reset;
static void ecu_reset_fun(uint8_t sub_function);

void svc_ecu_reset(const uds_request_t *req, uds_response_t *resp) {
    if (req->data_len < 1) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3;
        return;
    }

    uint8_t sub_raw = req->data[0];
    bool suppress = (sub_raw & 0x80) != 0;
    uint8_t sub = sub_raw & 0x7F;

    if (sub < RESET_HARD || sub > RESET_DISABLE_RAPID_POWER_SHUTDOWN) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
        resp->len = 3;
        return;
    }

    /* Build response before reset */
    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->data[1] = sub;
    resp->len = 2;
    resp->suppress = suppress;
    ecu_reset.reset_type_requested = sub;
    ecu_reset.reset_wait_elapsed_ms = uds_platform_time_ms();

    /* Clear periodic transmissions */
    uds_periodic_clear_all();

    /* Reset session/security */
    uds_session_reset();

}

/* ── ECUReset hook ───────────────────────────────────────────────── */

static void ecu_reset_fun(uint8_t sub_function) {
	switch(sub_function)
	{
		case RESET_HARD:
    		NVIC_SystemReset(); /* Runs 50 ms after 0x51 frame is sent */
			break;
		case RESET_KEY_OFF_ON:
    		NVIC_SystemReset(); /* Runs 50 ms after 0x51 frame is sent */
			break;
		case RESET_SOFT:
    		NVIC_SystemReset(); /* Runs 50 ms after 0x51 frame is sent */
			break;
		case RESET_ENABLE_RAPID_POWER_SHUTDOWN:
    		NVIC_SystemReset(); /* Runs 50 ms after 0x51 frame is sent */
			break;
		case RESET_DISABLE_RAPID_POWER_SHUTDOWN:
    		NVIC_SystemReset(); /* Runs 50 ms after 0x51 frame is sent */
			break;
		default:
        	NVIC_SystemReset();
        	break;
	}
}

void uds_reset_poll(void) {
	if (ecu_reset.reset_type_requested > 0U)
	{
		if (uds_platform_time_ms() - ecu_reset.reset_wait_elapsed_ms > DEFAULT_RESET_TX_WAIT_MS)
		{
			uint8_t reset_type = ecu_reset.reset_type_requested;
			ecu_reset.reset_type_requested = 0U;
			ecu_reset.reset_wait_elapsed_ms = 0U;
			ecu_reset_fun(reset_type);
		}
	}
}

