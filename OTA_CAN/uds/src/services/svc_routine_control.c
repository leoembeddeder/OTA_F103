#include "svc_routine_control.h"
#include "uds_session.h"
#include "uds_callbacks.h"

#define ROUTINE_ERASE_MEMORY 0xFF00U
#define ROUTINE_CHECK_MEMORY 0xFF01U
#define ROUTINE_ACTIVATE 0x0203U
#define ROUTINE_ROLLBACK 0x0204U

typedef enum {
	ROUTINE_STATE_IDLE = 0x00,
	ROUTINE_STATE_RUNNING = 0x01,
	ROUTINE_STATE_COMPLETED = 0x02,
	ROUTINE_STATE_FAILED = 0x03
} RoutineExecState;

static RoutineExecState s_routine_state = ROUTINE_STATE_IDLE;
static uint16_t s_active_routine_id = 0U;

void svc_routine_control(const uds_request_t *req, uds_response_t *resp) {
	/* 1. Request length check: subfunction (1 byte) + routineIdentifier (2 bytes) = 3 bytes */
	if (req->data_len < 3U) {
	resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
	resp->data[1] = req->sid;
	resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
	resp->len = 3U;
	return;
	}

	/* 2. Decode subfunction and SPRMIB */
	uint8_t raw_sub = req->data[0];
	uint8_t sub = raw_sub & 0x7FU;
	bool suppress = (raw_sub & 0x80U) != 0U;
	uint16_t routine_id = ((uint16_t)req->data[1] << 8U) | (uint16_t)req->data[2];

	/* 3. Validate supported routine IDs */
	if ((routine_id != ROUTINE_ERASE_MEMORY) &&
	    (routine_id != ROUTINE_CHECK_MEMORY) &&
	    (routine_id != ROUTINE_ACTIVATE) &&
	    (routine_id != ROUTINE_ROLLBACK)) {
	    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
	    resp->data[1] = req->sid;
	    resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
	    resp->len = 3U;
	    return;
	}

	uint8_t routine_status_info = 0x00U;

	/* 4. Subfunction state machine */
	switch (sub) {
	case 0x01: { /* startRoutine */
	    s_active_routine_id = routine_id;
	    /* Trigger action based on routine_id (e.g. erase flash slot) */
	    s_routine_state = ROUTINE_STATE_COMPLETED;
	    routine_status_info = (uint8_t)s_routine_state;
	    break;
	}

	case 0x02: { /* stopRoutine */
	    if (s_active_routine_id != routine_id) {
	        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
	        resp->data[1] = req->sid;
	        resp->data[2] = NRC_REQUEST_SEQUENCE_ERROR;
	        resp->len = 3U;
	        return;
	    }
	    s_routine_state = ROUTINE_STATE_IDLE;
	    routine_status_info = (uint8_t)s_routine_state;
	    break;
	}

	case 0x03: { /* requestRoutineResults */
	    if (s_active_routine_id != routine_id) {
	        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
	        resp->data[1] = req->sid;
	        resp->data[2] = NRC_REQUEST_SEQUENCE_ERROR;
	        resp->len = 3U;
	        return;
	    }
	    routine_status_info = (uint8_t)s_routine_state;
	    break;
	}

	default:
	    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
	    resp->data[1] = req->sid;
	    resp->data[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
	    resp->len = 3U;
	    return;
	}

	/* 5. Handle SPRMIB */
	if (suppress) {
	    resp->suppress = true;
	    resp->len = 0U;
	    return;
	}

	/* 6. Positive response: 0x71 + subfunction + routineID (2 bytes) + routineInfo */
	resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid); /* 0x71 */
	resp->data[1] = sub;
	resp->data[2] = (uint8_t)(routine_id >> 8U);
	resp->data[3] = (uint8_t)(routine_id & 0xFFU);
	resp->data[4] = routine_status_info;
	resp->len = 5U;
}
