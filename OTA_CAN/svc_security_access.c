#include "svc_security_access.h"
#include "uds_session.h"
#include "uds_config.h"

#include <string.h>

/*
 * 0x27 SecurityAccess
 * Odd sub-functions (0x01, 0x03, ...) = requestSeed
 *   -> response [67 sub seed[0..N-1]]
 * Even sub-functions (0x02, 0x04, ...) = sendKey
 *   -> response [67 sub]
 *
 * Key algorithm: key[i] = seed[i] XOR secret[i % secret_len]
 */

void svc_security_access(const uds_request_t *req, uds_response_t *resp) {
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

    security_state_t sec_state = uds_security_get_state();

    /* Check lockout */
    if (sec_state == SECURITY_LOCKED_OUT) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED;
        resp->len = 3;
        return;
    }

    bool is_request_seed = (sub & 0x01) != 0; /* Odd = requestSeed */

    if (is_request_seed) {
        /* Request Seed (sub 0x01, 0x03, 0x05, ...) */
        const uint8_t *seed = uds_security_request_seed();
        uint8_t seed_len = uds_security_seed_len();

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        memcpy(&resp->data[2], seed, seed_len);
        resp->len = 2 + seed_len;
        resp->suppress = suppress;
    } else {
        /* Send Key (sub 0x02, 0x04, 0x06, ...) */
        if (sec_state != SECURITY_SEED_SENT) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_SEQUENCE_ERROR;
            resp->len = 3;
            return;
        }

        uint8_t seed_len = uds_security_seed_len();
        if (req->data_len < 1 + seed_len) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3;
            return;
        }

        if (uds_security_send_key(&req->data[1], seed_len)) {
            resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
            resp->data[1] = sub;
            resp->len = 2;
            resp->suppress = suppress;
        } else {
            uint8_t nrc = NRC_INVALID_KEY;
            if (uds_security_get_state() == SECURITY_LOCKED_OUT) {
                nrc = NRC_EXCEEDED_NUMBER_OF_ATTEMPTS;
            }
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = nrc;
            resp->len = 3;
        }
    }
}
