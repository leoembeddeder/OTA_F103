/**
 * @file svc_ecu_reset.c
 * @brief UDS Service 0x11 ECUReset Implementation (ISO 14229-1)
 */

#include "svc_ecu_reset.h"
#include "uds_session.h"
#include "uds_platform_time.h"
#include "main.h"
#include <stddef.h>
#include <stdio.h>

#define RESET_DELAY_TICKS  30U /* 30 ms delay to allow CAN transmission of response */

static uint8_t s_pending_reset_type = 0U;
static uint32_t s_reset_countdown = 0U;

void svc_ecu_reset(const uds_request_t *req, uds_response_t *resp) {
    if (resp == NULL) {
        return;
    }
    if ((req == NULL) || (req->data_len != 1U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = (req != NULL) ? req->sid : 0x11U;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    uint8_t raw_sub = req->data[0];
    uint8_t sub = raw_sub & 0x7FU;
    bool suppress = ((raw_sub & 0x80U) != 0U);

    if ((sub < 0x01U) || (sub > 0x05U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
        resp->len = 3U;
        return;
    }

    /* Arm pending reset with countdown to ensure positive response completes transmission */
    s_pending_reset_type = sub;
    s_reset_countdown = uds_platform_time_ms();

    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->data[1] = sub;
    resp->len = 2U;

    if (sub == 0x04U) {
        /* enableRapidPowerShutDown: returns powerDownTime (0 seconds) */
        resp->data[2] = 0x00U;
        resp->len = 3U;
    }

    if (suppress) {
        resp->suppress = true;
        resp->len = 0U;
    }
}

void uds_reset_poll(void) {
    if (s_pending_reset_type > 0U) {
        if (uds_platform_time_ms() - s_reset_countdown > RESET_DELAY_TICKS) {
            uint8_t reset_type = s_pending_reset_type;
			s_pending_reset_type = 0U;
			s_reset_countdown = 0U;
            uds_session_reset();
            on_ecu_reset(reset_type);
        }
    }
}

void on_ecu_reset(uint8_t reset_type)
{
	switch(reset_type)
	{
		case RESET_HARD:
			printf("on hardReset\r\n");
    		NVIC_SystemReset(); /* Runs 50 ms after 0x51 frame is sent */
			break;
		case RESET_KEY_OFF_ON:
			printf("on keyOffOnReset\r\n");
    		NVIC_SystemReset(); /* Runs 50 ms after 0x51 frame is sent */
			break;
		case RESET_SOFT:
			printf("on softReset\r\n");
    		NVIC_SystemReset(); /* Runs 50 ms after 0x51 frame is sent */
			break;
		case RESET_ENABLE_RAPID_POWER_SHUTDOWN:
			printf("on enableRapidPowerShutDown\r\n");
    		NVIC_SystemReset(); /* Runs 50 ms after 0x51 frame is sent */
			break;
		case RESET_DISABLE_RAPID_POWER_SHUTDOWN:
			printf("on disableRapidPowerShutDown\r\n");
    		NVIC_SystemReset(); /* Runs 50 ms after 0x51 frame is sent */
			break;
		default:
        	NVIC_SystemReset();
        	break;
	}
}

