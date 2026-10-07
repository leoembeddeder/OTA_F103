#include <string.h>
#include "uds_server.h"
#include "uds_session.h"
#include "uds_config.h"

/* Service handler headers (library-provided) */
#include "svc_diag_session.h"
#include "svc_ecu_reset.h"
#include "svc_tester_present.h"
#include "svc_security_access.h"
#include "svc_read_did.h"
#include "svc_write_did.h"
#include "svc_read_dtc.h"
#include "svc_clear_dtc.h"
#include "svc_download.h"
#include "svc_upload.h"
#include "svc_periodic_did.h"
#include "svc_dyn_did.h"
#include "svc_io_control.h"
#include "svc_link_ctrl.h"
#include "svc_routine_control.h"
#include "svc_comm_control.h"
#include "svc_control_dtc.h"
#include "svc_read_address.h"
#include "svc_write_address.h"


/* ── App config storage ───────────────────────────────────────────── */

static const uds_app_config_t *s_app_config = NULL;

const uds_app_config_t *uds_get_app_config(void) {
    return s_app_config;
}

/* ── Dynamic service dispatch table ───────────────────────────────── */

static uds_service_entry_t s_service_table[UDS_MAX_SERVICES];
static uint8_t s_service_count = 0;

static void register_service(uint8_t sid, uds_service_handler_t handler,
                             uint8_t session_mask, bool requires_security) {
    if (s_service_count >= UDS_MAX_SERVICES) return;
    uds_service_entry_t *e = &s_service_table[s_service_count++];
    e->sid = sid;
    e->handler = handler;
    e->session_mask = session_mask;
    e->requires_security = requires_security;
}

/* ── Helper: build negative response ──────────────────────────────── */

static void build_nrc(uds_response_t *resp, uint8_t sid, uint8_t nrc) {
    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
    resp->data[1] = sid;
    resp->data[2] = nrc;
    resp->len = 3;
    resp->suppress = false;
}

/* ── Lookup ───────────────────────────────────────────────────────── */

static const uds_service_entry_t *find_service(uint8_t sid) {
    /* Search in reverse so app-registered overrides win */
    for (int i = (int)s_service_count - 1; i >= 0; i--) {
        if (s_service_table[i].sid == sid) {
            return &s_service_table[i];
        }
    }
    return NULL;
}

/* ── Public API ───────────────────────────────────────────────────── */

void uds_server_init(const uds_app_config_t *config) {
    s_app_config = config;
    s_service_count = 0;

    /* Install base library services */
    /*       SID                handler                  session_mask                                   security */
    register_service(0x10, svc_diag_session_control,    SESSION_MASK_ALL,                                 false);
    register_service(0x11, svc_ecu_reset,               SESSION_MASK_ALL,                                 false);
    register_service(0x14, svc_clear_dtc,               SESSION_MASK_NON_DEFAULT,                         false);
    register_service(0x19, svc_read_dtc_info,           SESSION_MASK_ALL,                                 false);
    register_service(0x22, svc_read_data_by_id,         SESSION_MASK_ALL,                                 false);
	register_service(0x23, svc_read_memory_by_address,  SESSION_MASK_ALL,                                 false);
    register_service(0x27, svc_security_access,         SESSION_MASK_NON_DEFAULT,                         false);
	register_service(0x28, svc_comm_control,            SESSION_MASK_ALL,                                 false);
    register_service(0x2A, svc_read_periodic_id,        SESSION_MASK_EXTENDED | SESSION_MASK_ENGINEERING, false);
    register_service(0x2C, svc_dyn_define_did,          SESSION_MASK_EXTENDED | SESSION_MASK_ENGINEERING, false);
    register_service(0x2E, svc_write_data_by_id,        SESSION_MASK_NON_DEFAULT,                         true);
    register_service(0x2F, svc_io_control,              SESSION_MASK_EXTENDED | SESSION_MASK_ENGINEERING, false);
    register_service(0x31, svc_routine_control,         SESSION_MASK_ALL,                                 false);
    register_service(0x34, svc_request_download,        SESSION_MASK_PROGRAMMING,                         true);
    register_service(0x35, svc_request_upload,          SESSION_MASK_PROGRAMMING | SESSION_MASK_EXTENDED, true);
    register_service(0x36, svc_transfer_data,           SESSION_MASK_PROGRAMMING | SESSION_MASK_EXTENDED, true);
    register_service(0x37, svc_transfer_exit,           SESSION_MASK_PROGRAMMING | SESSION_MASK_EXTENDED, true);
    register_service(0x3E, svc_tester_present,          SESSION_MASK_ALL,                                 false);
	register_service(0x3D, svc_write_memory_by_address, SESSION_MASK_ALL,                                 false);
	register_service(0x85, svc_control_dtc,             SESSION_MASK_ALL,                                 false);
    register_service(0x87, svc_link_control,            SESSION_MASK_EXTENDED | SESSION_MASK_ENGINEERING, false);

    uds_state_init();
}

bool uds_server_register(const uds_service_entry_t *entry) {
    if (s_service_count >= UDS_MAX_SERVICES) return false;
    s_service_table[s_service_count++] = *entry;
    return true;
}

bool uds_server_process(const uint8_t *data, uint16_t len,
                        bool functional, uds_response_t *resp) {
    if (len < 1) return false;

    uint8_t sid = data[0];
    memset(resp, 0, sizeof(*resp));

    /* Refresh S3 timer on any valid request */
    uds_session_refresh();

    /* Find service handler */
    const uds_service_entry_t *entry = find_service(sid);
    if (!entry) {
        if (functional) return false; /* No NRC for unknown SID on functional */
        build_nrc(resp, sid, NRC_SERVICE_NOT_SUPPORTED);
        return true;
    }

    /* Session gate */
    uds_session_t cur = uds_session_get();
    if (!(entry->session_mask & uds_session_to_mask(cur))) {
        if (functional) return false;
        build_nrc(resp, sid, NRC_SERVICE_NOT_SUPPORTED_IN_SESSION);
        return true;
    }

    /* Security gate */
    if (entry->requires_security && !uds_security_is_unlocked()) {
        if (functional) return false;
        build_nrc(resp, sid, NRC_SECURITY_ACCESS_DENIED);
        return true;
    }

    /* Build request struct */
    uds_request_t req;
    req.sid        = sid;
    req.data       = (len > 1) ? &data[1] : NULL;
    req.data_len   = (len > 1) ? (len - 1) : 0;
    req.functional = functional;

    /* Call service handler */
    entry->handler(&req, resp);

    /* Check suppress-positive-response bit (sub-function bit 7) */
    if (resp->suppress) {
        return false;
    }

    return (resp->len > 0);
}

void uds_server_poll(void) {
    uds_session_check_timeout();
    uds_security_poll();
	uds_reset_poll();
}
