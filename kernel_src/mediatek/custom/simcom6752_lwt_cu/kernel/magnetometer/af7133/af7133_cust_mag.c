#include <linux/types.h>
//#include <mach/mt6577_pm_ldo.h>
#include <mach/mt_pm_ldo.h>
#include <cust_mag.h>


static struct mag_hw cust_mag_hw = {
    .i2c_num = 1,  //MT6589 ,3
    .direction = 0,// 0
    .power_id = MT65XX_POWER_NONE,  /*!< LDO is not used */
    .power_vol= VOL_DEFAULT,        /*!< LDO is not used */
};

#if 1//defined(MTK_AUTO_DETECT_MAGNETOMETER)
struct mag_hw* af7133_get_cust_mag_hw(void) 
#else
struct mag_hw* get_cust_mag_hw(void) 
#endif
{
    return &cust_mag_hw;
}
