#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include "cmdq_record.h"
#include "ddp_reg.h"
#include "ddp_drv.h"
#include "ddp_aal.h"
#include "ddp_pwm.h"
#include "ddp_path.h"
#include <mach/mt_clkmgr.h>

// To enable debug log:
// # echo aal_dbg:1 > /sys/kernel/debug/dispsys
int aal_dbg_en = 0;

/*20141031 Acer add for color engine*/
#include "ddp_debug.h"
#include "acer_color_engine_MTK.h"
#include "acer_color_profile_MTK.h"
static int previous_color_sunlight_engine_enable =0;
static int previous_acer_content_aware_profile = 0;
static int acer_content_aware_profile=0;
static int acer_sunlight_readable_profile=0;
static int acer_sunlight_content_profile =0;
static int previous_acer_sunlight_content_profile =-1;
static int MTK_combine_Sunlight_CurrentFlag=0;
static int MTK_combine_Content_CurrentFlag=0;
static int MTK_combine_Content_PreviousFlag =-1;
static int MTK_combine_Sunlight_PreviousFlag =-1;
static int *acer_final_luma_curve;
static int acer_auto_backlight_enable = 0;
static int acer_combine_luma_gain [29]; 
extern int acer_get_als_mode();
/*20141031 Acer add for color engine*/

#define AAL_ERR(fmt, arg...) printk(KERN_ERR "[AAL] " fmt "\n", ##arg)
#define AAL_DBG(fmt, arg...) \
    do { if (aal_dbg_en) printk(KERN_DEBUG "[AAL] " fmt "\n", ##arg); } while (0)

static int disp_aal_write_param_to_reg(cmdqRecHandle cmdq, const DISP_AAL_PARAM *param);

static DECLARE_WAIT_QUEUE_HEAD(g_aal_hist_wq);
static DEFINE_SPINLOCK(g_aal_hist_lock);
static DISP_AAL_HIST g_aal_hist, g_aal_hist_db;
static ddp_module_notify g_ddp_notify = NULL;
static volatile int g_aal_hist_available = 0;
static volatile int g_aal_dirty_frame_retrieved = 1;


static int disp_aal_init(DISP_MODULE_ENUM module, int width, int height, void* cmdq)
{
#ifdef MTK_AAL_SUPPORT
    /* Enable AAL histogram, engine */
    DISP_REG_MASK(cmdq, DISP_AAL_CFG, 0x3 << 1, (0x3 << 1) | 0x1);
#endif

    g_aal_hist_available = 0;
    g_aal_dirty_frame_retrieved = 1;

    return 0;
}


static void disp_aal_trigger_refresh(void)
{
    if (g_ddp_notify != NULL)
       g_ddp_notify(DISP_MODULE_AAL, DISP_PATH_EVENT_TRIGGER);
}


static void disp_aal_set_interrupt(int enabled)
{
#ifdef MTK_AAL_SUPPORT
    if (enabled) {
        if (DISP_REG_GET(DISP_AAL_EN) == 0) {
            AAL_DBG("[WARNING] DISP_AAL_EN not enabled!");
        }
    
        /* Enable output frame end interrupt */
        DISP_CPU_REG_SET(DISP_AAL_INTEN, 0x2);
        AAL_DBG("Interrupt enabled");
    } else {
        if (g_aal_dirty_frame_retrieved) {
            DISP_CPU_REG_SET(DISP_AAL_INTEN, 0x0);
            AAL_DBG("Interrupt disabled");
        } else { /* Dirty histogram was not retrieved */
            /* Only if the dirty hist was retrieved, interrupt can be disabled.
               Continue interrupt until AALService can get the latest histogram. */
        }
    }
    
#else
    AAL_ERR("AAL not enabled");
#endif
}


static void disp_aal_notify_frame_dirty(void)
{
#ifdef MTK_AAL_SUPPORT
    unsigned long flags;

    AAL_DBG("disp_aal_notify_frame_dirty()");

    spin_lock_irqsave(&g_aal_hist_lock, flags);
    /* Interrupt can be disabled until dirty histogram is retrieved */
    g_aal_dirty_frame_retrieved = 0;
    spin_unlock_irqrestore(&g_aal_hist_lock, flags);
    
    disp_aal_set_interrupt(1);
#endif
}


static int disp_aal_wait_hist(unsigned long timeout)
{
    int ret = 0;

    AAL_DBG("disp_aal_wait_hist: available = %d", g_aal_hist_available);
    
    if (!g_aal_hist_available) {
        ret = wait_event_interruptible(g_aal_hist_wq, (g_aal_hist_available != 0));
        AAL_DBG("disp_aal_wait_hist: waken up, ret = %d", ret);
    } else {
        /* If g_aal_hist_available is already set, means AALService was delayed */
    }

    return ret;
}


void disp_aal_on_end_of_frame(void)
{
    unsigned int intsta;
    int i;
    unsigned long flags;

    intsta = DISP_REG_GET(DISP_AAL_INTSTA);

    AAL_DBG("disp_aal_on_end_of_frame: intsta: 0x%x", intsta);
    if (intsta & 0x2) { /* End of frame */
        if (spin_trylock_irqsave(&g_aal_hist_lock, flags)) {
            DISP_CPU_REG_SET(DISP_AAL_INTSTA, (intsta & ~0x3));
        
            for (i = 0; i < AAL_HIST_BIN; i++) {
                g_aal_hist.maxHist[i] = DISP_REG_GET(DISP_AAL_STATUS_00 + (i << 2));
            }
            g_aal_hist_available = 1;

            /* Allow to disable interrupt */
            g_aal_dirty_frame_retrieved = 1;
            
            spin_unlock_irqrestore(&g_aal_hist_lock, flags);

            wake_up_interruptible(&g_aal_hist_wq);
        } else {
            /*
             * Histogram was not be retrieved, but it's OK.
             * Another interrupt will come until histogram available
             * See: disp_aal_set_interrupt()
             */
        }
    }
}


static int disp_aal_copy_hist_to_user(DISP_AAL_HIST __user *hist)
{
    unsigned long flags;
    int ret = -EFAULT;

    /* We assume only one thread will call this function */

    spin_lock_irqsave(&g_aal_hist_lock, flags);
    memcpy(&g_aal_hist_db, &g_aal_hist, sizeof(DISP_AAL_HIST));
    g_aal_hist_available = 0;
    spin_unlock_irqrestore(&g_aal_hist_lock, flags);

    if (copy_to_user(hist, &g_aal_hist_db, sizeof(DISP_AAL_HIST)) == 0) {
        ret = 0;
    }

    AAL_DBG("disp_aal_copy_hist_to_user: %d", ret);

    return ret;
}


#define CABC_GAINLMT(v0, v1, v2) (((v2) << 20) | ((v1) << 10) | (v0))

static DISP_AAL_PARAM g_aal_param;

static int disp_aal_set_init_reg(DISP_AAL_INITREG __user *user_regs, void *cmdq)
{
    int ret = -EFAULT;
    DISP_AAL_INITREG *init_regs;

    init_regs = (DISP_AAL_INITREG*)kmalloc(sizeof(DISP_AAL_INITREG), GFP_KERNEL);
    if (init_regs == NULL) {
        AAL_ERR("disp_aal_set_init_reg: insufficient memory");
        return -EFAULT;
    }

    ret = copy_from_user(init_regs, user_regs, sizeof(DISP_AAL_INITREG));
    if (ret == 0) {
        int i, j;
        int *gain;

        DISP_REG_MASK(cmdq, DISP_AAL_DRE_MAPPING_00, (init_regs->dre_map_bypass << 4), 1 << 4);

        gain = init_regs->cabc_gainlmt;
        j = 0;
        for (i = 0; i <= 10; i++) {
            DISP_REG_SET(cmdq, DISP_AAL_CABC_GAINLMT_TBL(i),
                CABC_GAINLMT(gain[j], gain[j + 1], gain[j + 2]));
            j += 3;
        }
    } else {
        AAL_ERR("disp_aal_set_init_reg: copy_from_user() failed");
    }

    AAL_DBG("disp_aal_set_init_reg: %d", ret);

    kfree(init_regs);

    return ret;
}


int disp_aal_set_param(DISP_AAL_PARAM __user *param, void *cmdq)
{
    int ret = -EFAULT;
    int backlight_value = 0;

    /* Not need to protect g_aal_param, since only AALService
       can set AAL parameters. */
    if (copy_from_user(&g_aal_param, param, sizeof(DISP_AAL_PARAM)) == 0) {
        ret = disp_aal_write_param_to_reg(cmdq, &g_aal_param);
        backlight_value = g_aal_param.FinalBacklight;
    }

    if (ret == 0) {
        ret |= disp_pwm_set_backlight_cmdq(DISP_PWM0, backlight_value, cmdq);
    }

    AAL_DBG("disp_aal_set_param(CABC = %d, DRE[0,8] = %d,%d): ret = %d", 
        g_aal_param.cabc_fltgain_force, g_aal_param.DREGainFltStatus[0],
        g_aal_param.DREGainFltStatus[8], ret);

    disp_aal_trigger_refresh();

    return ret;
}


#define DRE_REG_2(v0, off0, v1, off1)           (((v1) << (off1)) | ((v0) << (off0)))
#define DRE_REG_3(v0, off0, v1, off1, v2, off2) (((v2) << (off2)) | (v1 << (off1)) | ((v0) << (off0)))

static int disp_aal_write_param_to_reg(cmdqRecHandle cmdq, const DISP_AAL_PARAM *param)
{
    int i;
    const int *gain;

    if(sunlight_content_color_engine_enable) /*20141031 Acer add for color engine*/
    {
	set_acer_color_profile(cmdq);
    }
    else
    {
	gain = param->DREGainFltStatus;
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_GAIN_FILTER_00, 1 << 8, 1 << 8); /* dre_gain_force_en */
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(0), DRE_REG_2(gain[0], 0, gain[1], 12), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(1), DRE_REG_2(gain[2], 0, gain[3], 12), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(2), DRE_REG_2(gain[4], 0, gain[5], 11), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(3), DRE_REG_3(gain[6], 0, gain[7], 11, gain[8], 21), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(4), DRE_REG_3(gain[9], 0, gain[10], 10, gain[11], 20), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(5), DRE_REG_3(gain[12], 0, gain[13], 10, gain[14], 20), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(6), DRE_REG_3(gain[15], 0, gain[16], 10, gain[17], 20), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(7), DRE_REG_3(gain[18], 0, gain[19], 9, gain[20], 18), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(8), DRE_REG_3(gain[21], 0, gain[22], 9, gain[23], 18), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(9), DRE_REG_3(gain[24], 0, gain[25], 9, gain[26], 18), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(10), DRE_REG_2(gain[27], 0, gain[28], 9), ~0);
    }

    /*
    gain = param->DREGainFltStatus;
    DISP_REG_MASK(cmdq, DISP_AAL_DRE_GAIN_FILTER_00, 1 << 8, 1 << 8); // dre_gain_force_en
    DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(0), DRE_REG_2(gain[0], 0, gain[1], 12), ~0);
    DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(1), DRE_REG_2(gain[2], 0, gain[3], 12), ~0);
    DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(2), DRE_REG_2(gain[4], 0, gain[5], 11), ~0);
    DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(3), DRE_REG_3(gain[6], 0, gain[7], 11, gain[8], 21), ~0);
    DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(4), DRE_REG_3(gain[9], 0, gain[10], 10, gain[11], 20), ~0);
    DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(5), DRE_REG_3(gain[12], 0, gain[13], 10, gain[14], 20), ~0);
    DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(6), DRE_REG_3(gain[15], 0, gain[16], 10, gain[17], 20), ~0);
    DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(7), DRE_REG_3(gain[18], 0, gain[19], 9, gain[20], 18), ~0);
    DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(8), DRE_REG_3(gain[21], 0, gain[22], 9, gain[23], 18), ~0);
    DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(9), DRE_REG_3(gain[24], 0, gain[25], 9, gain[26], 18), ~0);
    DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(10), DRE_REG_2(gain[27], 0, gain[28], 9), ~0);
	*/
    DISP_REG_MASK(cmdq, DISP_AAL_CABC_00, 1 << 31, 1 << 31);
    DISP_REG_MASK(cmdq, DISP_AAL_CABC_02, ((1 << 26) | param->cabc_fltgain_force), ((1 << 26) | 0x3ff));
    

    gain = param->cabc_gainlmt;
    for (i = 0; i <= 10; i++) {
        DISP_REG_MASK(cmdq, DISP_AAL_CABC_GAINLMT_TBL(i),
            CABC_GAINLMT(gain[0], gain[1], gain[2]), ~0);
        gain += 3;
    }

    return 0;
}


static int aal_config(DISP_MODULE_ENUM module, disp_ddp_path_config* pConfig, void *cmdq)
{
    if (pConfig->dst_dirty) {
        int width, height;
        
        width = pConfig->dst_w;
        height = pConfig->dst_h;
        
        DISP_REG_SET(cmdq, DISP_AAL_SIZE, (width << 16) | height);
        DISP_REG_MASK(cmdq, DISP_AAL_CFG, 0x0, 0x1); /* Disable relay mode */
    
        disp_aal_init(module, width, height, cmdq);

        DISP_REG_MASK(cmdq, DISP_AAL_EN, 0x1, 0x1);
            
        AAL_DBG("AAL_CFG = 0x%x, AAL_SIZE = 0x%x(%d, %d)",
            DISP_REG_GET(DISP_AAL_CFG), DISP_REG_GET(DISP_AAL_SIZE), width, height);
    }
    
    if (pConfig->ovl_dirty || pConfig->rdma_dirty) {
        disp_aal_notify_frame_dirty();
    }
    
    return 0;
}


static int aal_clock_on(DISP_MODULE_ENUM module, void *cmq_handle)
{
    enable_clock(MT_CG_DISP0_DISP_AAL, "DDP");
    AAL_DBG("aal_clock_on CG 0x%x", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
    return 0;
}

static int aal_clock_off(DISP_MODULE_ENUM module, void *cmq_handle)
{
    AAL_DBG("aal_clock_off");
    disable_clock(MT_CG_DISP0_DISP_AAL, "DDP");
    return 0;
}

static int aal_init(DISP_MODULE_ENUM module, void *cmq_handle)
{
    aal_clock_on(module,cmq_handle);
    return 0; 
}

static int aal_deinit(DISP_MODULE_ENUM module, void *cmq_handle)
{
    aal_clock_off(module,cmq_handle);
    return 0;
}

static int aal_set_listener(DISP_MODULE_ENUM module, ddp_module_notify notify)
{
    g_ddp_notify = notify;
    return 0;
}

int aal_bypass(DISP_MODULE_ENUM module, int bypass)
{
    int relay = 0;
    if (bypass)
        relay = 1;
        
    DISP_REG_MASK(NULL, DISP_AAL_CFG, relay, 0x1);

    AAL_DBG("aal_bypass(bypass = %d)", bypass); 

    return 0;
}

static int aal_io(DISP_MODULE_ENUM module, int msg, unsigned long arg, void *cmdq)
{
    int ret = 0;

    switch (msg) {
        case DISP_IOCTL_AAL_EVENTCTL:
        {
            int enabled;
        
            if (copy_from_user(&enabled, (void*)arg , sizeof(enabled))) {
                AAL_ERR("DISP_IOCTL_AAL_EVENTCTL: copy_from_user() failed");
                return -EFAULT;
            }

            disp_aal_set_interrupt(enabled);
            
            if (enabled)
                disp_aal_trigger_refresh();

            break;
        }
        case DISP_IOCTL_AAL_GET_HIST:
        {
            disp_aal_wait_hist(60);

            if (disp_aal_copy_hist_to_user((DISP_AAL_HIST*)arg) < 0) {
                AAL_ERR("DISP_IOCTL_AAL_GET_HIST: copy_to_user() failed");
                return -EFAULT;
            }
            acer_get_display_condition(cmdq); /*20141031 Acer add for color engine*/
            break;            
        }
        case DISP_IOCTL_AAL_INIT_REG:
        {     
            if (disp_aal_set_init_reg((DISP_AAL_INITREG*)arg, cmdq) < 0) {
                AAL_ERR("DISP_IOCTL_AAL_INIT_REG: failed");
                return -EFAULT;
            }   
            break;
        }
        case DISP_IOCTL_AAL_SET_PARAM:
        {
            if (disp_aal_set_param((DISP_AAL_PARAM*)arg, cmdq) < 0) {
                AAL_ERR("DISP_IOCTL_AAL_SET_PARAM: failed");
                return -EFAULT;
            }
            break;
        }
    }

    return ret;
}

DDP_MODULE_DRIVER ddp_driver_aal =
{
    .init            = aal_init,
    .deinit          = aal_deinit,
    .config          = aal_config,
    .start 	         = NULL,
    .trigger         = NULL,
    .stop	         = NULL,
    .reset           = NULL,
    .power_on        = aal_clock_on,
    .power_off       = aal_clock_off,
    .is_idle         = NULL,
    .is_busy         = NULL,
    .dump_info       = NULL,
    .bypass          = aal_bypass,
    .build_cmdq      = NULL,
    .set_lcm_utils   = NULL,
    .set_listener    = aal_set_listener,
    .cmd             = aal_io,
}; 

/*Acer 20141031 add for color engine*/
void acer_get_display_condition(void *cmdq)
{
	if(sunlight_content_color_engine_enable)
    {
		acer_content_aware_profile = acer_color_analysis_histogram(g_aal_hist.maxHist, AAL_HIST_BIN);
		acer_auto_backlight_enable = acer_get_als_mode();
		acer_sunlight_readable_profile = acer_ALS_analysis_function(acer_auto_backlight_enable/*ALS_data_acer_sunlight_readable*/);
		acer_sunlight_content_profile = acer_sunlight_content_analysis_function(acer_content_aware_profile,acer_sunlight_readable_profile );
		set_acer_color_profile(cmdq);
	}
	else if( previous_color_sunlight_engine_enable && (!sunlight_content_color_engine_enable) )
	{
		acer_color_engine_set_saturation(0, cmdq);
		acer_color_initial();
		previous_acer_sunlight_content_profile = -1;
		MTK_combine_Sunlight_PreviousFlag = -1;
		MTK_combine_Content_PreviousFlag = -1; 
		acer_color_engine_set_luma(cmdq,-1,-1);
	}
	previous_color_sunlight_engine_enable = sunlight_content_color_engine_enable;
}

/*Acer 20141030 add for color engine*/
void acer_color_engine_set_saturation(int color_profile, void *cmdq)
{
	DISP_REG_SET(cmdq, DISP_COLOR_G_PIC_ADJ_MAIN_2, (0x200<<16) |acer_saturation_profile[color_profile]);
}

/*Acer 20141102 add for color engine*/
void acer_color_engine_set_luma(void *cmdq, int sunlight_flag, int content_flag)
{
	int i;
	acer_final_luma_curve = Sunlight_Content_acer_Luma_Gain[sunlight_flag][content_flag];

	if (sunlight_flag == -1 && content_flag ==-1)
	{
		for(i=0;i<29;i++)
		{
			acer_combine_luma_gain[i] = 256;
		}
	}
	else
	{
		for(i=0;i<29;i++)
		{
			acer_combine_luma_gain[i] = ( acer_final_luma_curve[i+1] * 256) / ((i+1)*32) ;
 			
			if(acer_combine_luma_gain[i] <256 )
			    	acer_combine_luma_gain[i] = 256;
			else if(acer_combine_luma_gain[i] >1023)
				acer_combine_luma_gain[i] = 1023;
		}
	}
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_GAIN_FILTER_00, 1 << 8, 1 << 8); /* dre_gain_force_en */
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(0), DRE_REG_2(acer_combine_luma_gain[0], 0, acer_combine_luma_gain[1], 12), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(1), DRE_REG_2(acer_combine_luma_gain[2], 0, acer_combine_luma_gain[3], 12), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(2), DRE_REG_2(acer_combine_luma_gain[4], 0, acer_combine_luma_gain[5], 11), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(3), DRE_REG_3(acer_combine_luma_gain[6], 0, acer_combine_luma_gain[7], 11, acer_combine_luma_gain[8], 21), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(4), DRE_REG_3(acer_combine_luma_gain[9], 0, acer_combine_luma_gain[10], 10, acer_combine_luma_gain[11], 20), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(5), DRE_REG_3(acer_combine_luma_gain[12], 0, acer_combine_luma_gain[13], 10, acer_combine_luma_gain[14], 20), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(6), DRE_REG_3(acer_combine_luma_gain[15], 0, acer_combine_luma_gain[16], 10, acer_combine_luma_gain[17], 20), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(7), DRE_REG_3(acer_combine_luma_gain[18], 0, acer_combine_luma_gain[19], 9, acer_combine_luma_gain[20], 18), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(8), DRE_REG_3(acer_combine_luma_gain[21], 0, acer_combine_luma_gain[22], 9, acer_combine_luma_gain[23], 18), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(9), DRE_REG_3(acer_combine_luma_gain[24], 0, acer_combine_luma_gain[25], 9, acer_combine_luma_gain[26], 18), ~0);
	DISP_REG_MASK(cmdq, DISP_AAL_DRE_FLT_FORCE(10), DRE_REG_2(acer_combine_luma_gain[27], 0, acer_combine_luma_gain[28], 9), ~0);
}

/*Acer 20141031 add for color engine*/
void set_acer_color_profile(void *cmdq)
{
	if(previous_acer_sunlight_content_profile  != acer_sunlight_content_profile)
	{
	    acer_color_engine_set_saturation(acer_sunlight_content_profile, cmdq);
	}
	previous_acer_sunlight_content_profile = acer_sunlight_content_profile;

	acer_sunlight_content_smooth(&MTK_combine_Sunlight_CurrentFlag, &MTK_combine_Content_CurrentFlag);
	
	if( (MTK_combine_Sunlight_CurrentFlag != MTK_combine_Sunlight_PreviousFlag) || (MTK_combine_Content_CurrentFlag != MTK_combine_Content_PreviousFlag) )
	{
	    acer_color_engine_set_luma(cmdq,MTK_combine_Sunlight_CurrentFlag,MTK_combine_Content_CurrentFlag);
	}
	MTK_combine_Sunlight_PreviousFlag = MTK_combine_Sunlight_CurrentFlag;
	MTK_combine_Content_PreviousFlag = MTK_combine_Content_CurrentFlag;
}

