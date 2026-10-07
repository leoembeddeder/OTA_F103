#ifndef SVC_READ_DTC_H
#define SVC_READ_DTC_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_types.h"


void svc_read_dtc_info(const uds_request_t *req, uds_response_t *resp);

#endif /* SVC_READ_DTC_H */
