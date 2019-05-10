
#ifndef __KREE_INT_H__
#define __KREE_INT_H__

#include "tz_cross/ree_service.h"


/* Maximum temp memory parameter size. */
#define TEE_PARAM_MEM_LIMIT   (4096)


TZ_RESULT KREE_InitTZ(void);

void tz_test(void);

TZ_RESULT KREE_TeeServiceCallNoCheck(KREE_SESSION_HANDLE handle, uint32_t command,
				     uint32_t paramTypes, MTEEC_PARAM param[4]);

typedef TZ_RESULT(*KREE_REE_Service_Func) (u32 op, u8 uparam[REE_SERVICE_BUFFER_SIZE]);


/* REE Services function prototype */
TZ_RESULT KREE_ServRequestIrq(u32 op, u8 uparam[REE_SERVICE_BUFFER_SIZE]);
TZ_RESULT KREE_ServEnableIrq(u32 op, u8 uparam[REE_SERVICE_BUFFER_SIZE]);

TZ_RESULT KREE_ServEnableClock(u32 op, u8 uparam[REE_SERVICE_BUFFER_SIZE]);
TZ_RESULT KREE_ServDisableClock(u32 op, u8 uparam[REE_SERVICE_BUFFER_SIZE]);


#endif				/* __KREE_INT_H__ */
