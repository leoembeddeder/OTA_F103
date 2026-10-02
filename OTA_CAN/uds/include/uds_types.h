#ifndef UDS_TYPES_H
#define UDS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_config.h"

/* ── UDS Service Identifiers (SID) ────────────────────────────────── */
#define UDS_SID_DIAG_SESSION_CTRL       0x10
#define UDS_SID_ECU_RESET               0x11
#define UDS_SID_CLEAR_DTC               0x14
#define UDS_SID_READ_DTC_INFO           0x19
#define UDS_SID_READ_DATA_BY_ID         0x22
#define UDS_SID_SECURITY_ACCESS         0x27
#define UDS_SID_READ_PERIODIC_ID        0x2A
#define UDS_SID_DYN_DEFINE_DID          0x2C
#define UDS_SID_WRITE_DATA_BY_ID        0x2E
#define UDS_SID_IO_CONTROL              0x2F
#define UDS_SID_ROUTINE_CONTROL         0x31
#define UDS_SID_REQUEST_DOWNLOAD        0x34
#define UDS_SID_REQUEST_UPLOAD          0x35
#define UDS_SID_TRANSFER_DATA           0x36
#define UDS_SID_TRANSFER_EXIT           0x37
#define UDS_SID_TESTER_PRESENT          0x3E
#define UDS_SID_LINK_CONTROL            0x87

/* Positive response = SID + 0x40 */
#define UDS_POSITIVE_RESPONSE(sid)      ((sid) + 0x40)

/* Negative response SID */
#define UDS_SID_NEGATIVE_RESPONSE       0x7F

/* ── Negative Response Codes (NRC) ────────────────────────────────── */
#define NRC_GENERAL_REJECT                  0x10
#define NRC_SERVICE_NOT_SUPPORTED           0x11
#define NRC_SUBFUNCTION_NOT_SUPPORTED       0x12
#define NRC_INCORRECT_MSG_LEN_OR_FORMAT     0x13
#define NRC_RESPONSE_TOO_LONG               0x14
#define NRC_BUSY_REPEAT_REQUEST             0x21
#define NRC_CONDITIONS_NOT_CORRECT          0x22
#define NRC_REQUEST_SEQUENCE_ERROR          0x24
#define NRC_REQUEST_OUT_OF_RANGE            0x31
#define NRC_SECURITY_ACCESS_DENIED          0x33
#define NRC_INVALID_KEY                     0x35
#define NRC_EXCEEDED_NUMBER_OF_ATTEMPTS     0x36
#define NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED 0x37
#define NRC_UPLOAD_DOWNLOAD_NOT_ACCEPTED    0x70
#define NRC_TRANSFER_DATA_SUSPENDED         0x71
#define NRC_GENERAL_PROGRAMMING_FAILURE     0x72
#define NRC_WRONG_BLOCK_SEQUENCE_COUNTER    0x73
#define NRC_SERVICE_NOT_SUPPORTED_IN_SESSION 0x7F

/* ── Session types ────────────────────────────────────────────────── */
typedef enum {
    UDS_SESSION_DEFAULT     = 0x01,
    UDS_SESSION_PROGRAMMING = 0x02,
    UDS_SESSION_EXTENDED    = 0x03,
    UDS_SESSION_ENGINEERING = 0x60,
} uds_session_t;

/* Session bit masks for service dispatch table (use bit index mapping) */
#define SESSION_BIT(s)            (1u << ((s) <= 3 ? (s) : 4))
#define SESSION_MASK_DEFAULT      SESSION_BIT(UDS_SESSION_DEFAULT)
#define SESSION_MASK_PROGRAMMING  SESSION_BIT(UDS_SESSION_PROGRAMMING)
#define SESSION_MASK_EXTENDED     SESSION_BIT(UDS_SESSION_EXTENDED)
#define SESSION_MASK_ENGINEERING  SESSION_BIT(UDS_SESSION_ENGINEERING)
#define SESSION_MASK_ALL          (SESSION_MASK_DEFAULT | SESSION_MASK_PROGRAMMING | SESSION_MASK_EXTENDED | SESSION_MASK_ENGINEERING)
#define SESSION_MASK_NON_DEFAULT  (SESSION_MASK_PROGRAMMING | SESSION_MASK_EXTENDED | SESSION_MASK_ENGINEERING)

/* Convert session enum to bit mask for comparison */
static inline uint8_t uds_session_to_mask(uds_session_t s) {
    return (uint8_t)SESSION_BIT(s);
}

/* ── UDS request/response structures ──────────────────────────────── */
typedef struct {
    uint8_t  sid;
    const uint8_t *data;     /* Points past SID byte */
    uint16_t data_len;       /* Length excluding SID  */
    bool     functional;     /* True if received on functional CAN ID */
} uds_request_t;

typedef struct {
    uint8_t  data[ISOTP_TX_BUF_SIZE];
    uint16_t len;
    bool     suppress;       /* Suppress positive response */
} uds_response_t;

/* ── Service handler function type ────────────────────────────────── */
typedef void (*uds_service_handler_t)(const uds_request_t *req, uds_response_t *resp);

/* ── Service dispatch table entry ─────────────────────────────────── */
typedef struct {
    uint8_t                 sid;
    uds_service_handler_t   handler;
    uint8_t                 session_mask;     /* Bit mask of allowed sessions */
    bool                    requires_security; /* Needs unlocked security?    */
} uds_service_entry_t;

#endif /* UDS_TYPES_H */
