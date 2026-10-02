#include "svc_ecu_reset.h"
#include "uds_session.h"
#include "uds_callbacks.h"

/*
 * 0x11 ECUReset
 * Sub 01=hardReset, 02=keyOff/On, 03=softReset
 *
 * Reset via callback — no Pico SDK / watchdog dependency in library.
 */

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

    if (sub < 1 || sub > 3) {
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

    /* Clear periodic transmissions */
    uds_periodic_clear_all();

    /* Reset session/security */
    uds_session_reset();

    /* Call app reset hook (e.g. watchdog reboot for hard reset) */
    const uds_app_config_t *app = uds_get_app_config();
    if (app && app->ecu_reset_hook) {
        app->ecu_reset_hook(sub);
    }
}
