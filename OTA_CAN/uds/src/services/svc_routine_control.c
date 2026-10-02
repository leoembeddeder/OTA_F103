#include "svc_routine_control.h"
#include "uds_session.h"
#include "uds_callbacks.h"



void svc_routine_control(const uds_request_t *req, uds_response_t *resp) 
{
    if (req->data_len < 3) 
	{
        resp->len = 3; 
		resp->data[0] = 0x7F;
        resp->data[1] = req->sid; 
		resp->data[2] = 0x13;
        return;
    }
    uint8_t  sub        = req->data[0];
    uint16_t routine_id = ((uint16_t)req->data[1] << 8) | req->data[2];

    if (routine_id != ROUTINE_ERASE_MEMORY &&
        routine_id != ROUTINE_ACTIVATE     &&
        routine_id != ROUTINE_COMMIT       &&
        routine_id != ROUTINE_ROLLBACK) 
    {
        resp->len = 3; 
		resp->data[0] = 0x7F;
        resp->data[1] = req->sid; 
		resp->data[2] = 0x31; 
        return;
    }

    switch (sub) 
	{
	    case 0x01:  /* startRoutine — kick off erase, return immediately. */
		    switch (routine_id) /* sub 0x01 = startRoutine. Per-ID behavior: */
			{
			    case ROUTINE_ERASE_MEMORY:
			        
			        break;

			    case ROUTINE_ACTIVATE:
			        
			        break;

			    case ROUTINE_COMMIT:
			        
			        break;

			    case ROUTINE_ROLLBACK:
					
			        break;
		    }
		
	        resp->len = 4;
	        resp->data[0] = 0x71;        /* RoutineControl positive resp */
	        resp->data[1] = 0x01;
	        resp->data[2] = (uint8_t)(routine_id >> 8);
	        resp->data[3] = (uint8_t)(routine_id & 0xFF);
	        return;
	    case 0x03: 
		{ /* requestRoutineResults — return current state. */
	        resp->len = 5;
	        resp->data[0] = 0x71;
	        resp->data[1] = 0x03;
	        resp->data[2] = (uint8_t)(routine_id >> 8);
	        resp->data[3] = (uint8_t)(routine_id & 0xFF);
	        //resp->data[4] = (uint8_t)s_state_chars[s_ota_state];
	        return;
	    }
	    default:
	        resp->len = 3; 
			resp->data[0] = 0x7F;
	        resp->data[1] = req->sid; 
			resp->data[2] = 0x12; /* sub-fn NS */
	        return;
    }
}



