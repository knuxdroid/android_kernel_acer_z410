#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <cust_alsps.h>

static struct alsps_hw cust_alsps_hw = {
    .i2c_num    = 1,
    .polling_mode_ps =0,
    .polling_mode_als =1,
    .power_id   = MT65XX_POWER_NONE,    /*LDO is not used*/
    .power_vol  = VOL_DEFAULT,          /*LDO is not used*/
    //.i2c_addr   = {0x0C, 0x48, 0x78, 0x00},
    //.als_level  = {0, 471, 1058, 1703, 3883, 10171, 10443, 15445, 28499, 35153, 41421, 59194, 65535, 65535, 65535},
    //.als_value  = {0, 133, 303, 501, 1002, 2001, 3355, 5001, 8008, 10010, 12000, 16010, 20010, 20010, 20010, 20010},
    .als_level  = { 0, 133, 215, 215, 335, 717, 717, 1354, 2213, 4253, 5647, 8659, 10803, 10922, 10922},
    .als_value  = { 130, 130, 130, 250, 250, 250, 735, 1410, 2230, 4260, 5750, 9050, 11000, 12250, 12250, 12250},
    .ps_threshold_high =14,//32,
    .ps_threshold_low = 7,//22,
    .is_batch_supported_ps = true,
    .is_batch_supported_als = true,
};
struct alsps_hw *get_cust_alsps_hw(void) {
    return &cust_alsps_hw;
}

