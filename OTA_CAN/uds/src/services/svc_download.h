#ifndef SVC_DOWNLOAD_H
#define SVC_DOWNLOAD_H

#include "uds_types.h"

void svc_request_download(const uds_request_t *req, uds_response_t *resp);
void svc_transfer_data(const uds_request_t *req, uds_response_t *resp);
void svc_transfer_exit(const uds_request_t *req, uds_response_t *resp);

#endif
