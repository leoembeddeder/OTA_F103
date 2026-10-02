#ifndef SVC_ROUTINE_CONTROL_H
#define SVC_ROUTINE_CONTROL_H

#include "uds_types.h"

/* ── 0x31 RoutineControl handler ───────────────────────────────────
 *
 *  0xFF00 (eraseMemory)        — prepare phase, erase inactive slot
 *  0xF001 (vendor: activate)   — STAGED → FUB-reboot to new slot in TBYB
 *  0xF002 (vendor: commit)     — TRIAL → clear trial_state, mark OK
 *  0xF003 (vendor: rollback)   — TRIAL → invalidate own image_def + reboot
 */
#define ROUTINE_ERASE_MEMORY 0xFF00
#define ROUTINE_ACTIVATE     0xF001
#define ROUTINE_COMMIT       0xF002
#define ROUTINE_ROLLBACK     0xF003


void svc_routine_control(const uds_request_t *req, uds_response_t *resp);

#endif

