#ifndef SVC_DYN_DID_H
#define SVC_DYN_DID_H

#include "uds_types.h"

void svc_dyn_define_did(const uds_request_t *req, uds_response_t *resp);

/** Read a dynamically defined DID.  Returns true if found and composited. */
bool ddid_read(uint16_t ddid, uint8_t *out, uint8_t *out_len, uint8_t max_len);

#endif /* SVC_DYN_DID_H */
