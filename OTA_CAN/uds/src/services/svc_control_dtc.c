#include "svc_control_dtc.h"
#include "uds_session.h"
#include "uds_callbacks.h"

static bool s_dtc_setting_enabled = true;

bool dtc_is_setting_enabled(void) {
	return s_dtc_setting_enabled;
}

void svc_control_dtc(const uds_request_t *req, uds_response_t *resp) {
	/* 1. Validate request length (minimum 1 byte: subfunction) */
	if (req->data_len < 1U) {
		resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
		resp->data[1] = req->sid;
		resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
		resp->len = 3U;
		return;
	}

	/* 2. Extract subfunction and SPRMIB */
	uint8_t raw_sub = req->data[0];
	uint8_t sub = raw_sub & 0x7FU;
	bool suppress = (raw_sub & 0x80U) != 0U;

	/* 3. Validate supported subfunctions (0x01 = ON, 0x02 = OFF) */
	if ((sub != 0x01U) && (sub != 0x02U)) {
	    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
	    resp->data[1] = req->sid;
	    resp->data[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
	    resp->len = 3U;
	    return;
	}

	/* 4. Apply state */
	s_dtc_setting_enabled = (sub == 0x01U);

	/* 5. Handle SPRMIB */
	if (suppress) {
	    resp->suppress = true;
	    resp->len = 0U;
	    return;
	}

	/* 6. Positive response: 0xC5 + echoed subfunction */
	resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid); /* 0xC5 */
	resp->data[1] = sub;
	resp->len = 2U;
}
