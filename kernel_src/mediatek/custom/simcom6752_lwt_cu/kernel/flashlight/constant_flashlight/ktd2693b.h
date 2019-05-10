#ifndef __KTD2693B__ 
#define __KTD2693B__

#include <linux/delay.h>
#include <linux/time.h>
#include "kd_camera_hw.h"
#include <cust_gpio_usage.h>

#define KTD_INPUT_START {mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE_PIN,GPIO_OUT_ONE); udelay(20);}
#define KTD_INPUT_STOP {mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE_PIN,GPIO_OUT_ZERO); \
udelay(20);\
mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE_PIN,GPIO_OUT_ONE);\
udelay(700);\
}
#define KTD_INPUT_HIGHT {mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE_PIN,GPIO_OUT_ZERO);\
udelay(5);\
mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE_PIN,GPIO_OUT_ONE);\
udelay(15);\
}

#define KTD_INPUT_LOW {mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE_PIN,GPIO_OUT_ZERO);\
udelay(15);\
mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE_PIN,GPIO_OUT_ONE);\
udelay(5);\
}
#define KTD_CTRL_RESET {mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE_PIN,GPIO_OUT_ZERO);udelay(2000);}

#define KTD_FLASH_MODE 0xa2

#define KTD_TORCH_MODE 0xa1

#define KTD_LED_OFF 0xa0

#define KTD_FLASH_CURRENT_BASE 0x80
#define KTD_TORCH_CURRENT   0x60



#endif
