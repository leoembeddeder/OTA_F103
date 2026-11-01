#include "svc_diag_session.h"
#include "uds_session.h"
#include "uds_config.h"

/*
 * 0x10 DiagnosticSessionControl
 * Request:  [10 sub]        sub = session type (bit 7 = suppressPosRsp)
 * Response: [50 sub P2hi P2lo P2*hi P2*lo]
 */
static bool isValidSessionTransition(uds_session_t current, uint8_t target);

void svc_diag_session_control(const uds_request_t *req, uds_response_t *resp) {
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

    /* Validate session type */
    if (sub != UDS_SESSION_DEFAULT &&
        sub != UDS_SESSION_PROGRAMMING &&
        sub != UDS_SESSION_EXTENDED &&
        sub != UDS_SESSION_ENGINEERING) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
        resp->len = 3;
        return;
    }
	else if (!isValidSessionTransition(uds_session_get(), sub))
	{
		resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
		resp->data[1] = req->sid;
		resp->data[2] = NRC_CONDITIONS_NOT_CORRECT;
		resp->len = 3;
		return;
	}

    /* Switch session */
    uds_session_set((uds_session_t)sub);

    /* Positive response */
    uint16_t p2     = UDS_P2_SERVER_MS;
    uint16_t p2star = UDS_P2_STAR_MS / 10; /* P2* in units of 10ms */

    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->data[1] = sub;
    resp->data[2] = (uint8_t)(p2 >> 8);
    resp->data[3] = (uint8_t)(p2 & 0xFF);
    resp->data[4] = (uint8_t)(p2star >> 8);
    resp->data[5] = (uint8_t)(p2star & 0xFF);
    resp->len = 6;
    resp->suppress = suppress;
}


static bool isValidSessionTransition(uds_session_t current, uint8_t target)
{
    if ((uint8_t)current == target)
    {
        return true;
    }

    switch (current)
    {
    case UDS_SESSION_DEFAULT:
        if ((target == (uint8_t)UDS_SESSION_EXTENDED) ||
            (target == (uint8_t)UDS_SESSION_ENGINEERING))
        {
            return true;
        }
        /* Direct transition Default -> Programming is forbidden (NRC 0x22) */
        return false;

    case UDS_SESSION_EXTENDED:
        if ((target == (uint8_t)UDS_SESSION_DEFAULT) ||
            (target == (uint8_t)UDS_SESSION_PROGRAMMING) ||
            (target == (uint8_t)UDS_SESSION_ENGINEERING))
        {
            return true;
        }
        return false;

    case UDS_SESSION_PROGRAMMING:
        if (target == (uint8_t)UDS_SESSION_DEFAULT)
        {
            return true;
        }
        /* Programming directly to Extended is forbidden */
        return false;

    case UDS_SESSION_ENGINEERING:
        if ((target == (uint8_t)UDS_SESSION_DEFAULT) ||
            (target == (uint8_t)UDS_SESSION_EXTENDED))
        {
            return true;
        }
        return false;

    default:
        return (target == (uint8_t)UDS_SESSION_DEFAULT);
    }
}

