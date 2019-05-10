#include <linux/types.h>
#include <cust_acc.h>
#include <mach/mt_pm_ldo.h>

/*---------------------------------------------------------------------------*/
static int cust_acc_power(struct acc_hw *hw, unsigned int on, char* devname)
{
#ifndef FPGA_EARLY_PORTING
    if (hw->power_id == MT65XX_POWER_NONE)
        return 0;
    if (on)
        return hwPowerOn(hw->power_id, hw->power_vol, devname);
    else
        return hwPowerDown(hw->power_id, devname); 
#else
    return 0;
#endif
}
/*---------------------------------------------------------------------------*/
static struct acc_hw mc3xxx_cust_acc =
{
    .i2c_num = 1,
#if defined(ACER_E800) || defined(ACER_JADEL) || defined(ACER_Z410)
    .direction = 7,
#else
    .direction = 1,
#endif
    .power_id = MT65XX_POWER_NONE,  /*!< LDO is not used */
    .power_vol= VOL_DEFAULT,        /*!< LDO is not used */
    .firlen = 16,                   /*!< don't enable low pass fileter */
    .power = cust_acc_power,        
    .is_batch_supported = false,
};
/*---------------------------------------------------------------------------*/
struct acc_hw* mc3xxx_get_cust_acc_hw(void) 
{
    return (&mc3xxx_cust_acc);
}
