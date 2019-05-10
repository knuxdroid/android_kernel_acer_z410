#ifndef __DDP_DEBUG_H__
#define __DDP_DEBUG_H__

#include <linux/kernel.h>
#include "ddp_mmp.h"
#include "ddp_dump.h"

void ddp_debug_init(void);
void ddp_debug_exit(void);

unsigned int  ddp_debug_analysis_to_buffer(void);
unsigned int  ddp_debug_dbg_log_level(void);
unsigned int  ddp_debug_irq_log_level(void);

int ddp_mem_test(void);
int ddp_lcd_test(void);

/*Acer 20141029 add for color engine debug*/
extern bool sunlight_content_color_engine_enable;
#endif /* __DDP_DEBUG_H__ */
