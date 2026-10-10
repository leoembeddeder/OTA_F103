#include "uds_platform_time.h"
#include "main.h"


uint32_t uds_platform_time_ms(void) {
    return HAL_GetTick();
}

bool uds_platform_time_expired(uint32_t start_ms, uint32_t timeout_ms) {
    return (uds_platform_time_ms() - start_ms) >= timeout_ms;
}

uint32_t uds_platform_time_elapsed(uint32_t start_ms) {
    return uds_platform_time_ms() - start_ms;
}

