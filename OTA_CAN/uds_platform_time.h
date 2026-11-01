#ifndef UDS_PLATFORM_TIME_H
#define UDS_PLATFORM_TIME_H


#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t uds_platform_time_ms(void);
bool     uds_platform_time_expired(uint32_t start_ms, uint32_t timeout_ms);
uint32_t uds_platform_time_elapsed(uint32_t start_ms);

#ifdef __cplusplus
}
#endif

#endif /* UDS_PLATFORM_TIME_H */
