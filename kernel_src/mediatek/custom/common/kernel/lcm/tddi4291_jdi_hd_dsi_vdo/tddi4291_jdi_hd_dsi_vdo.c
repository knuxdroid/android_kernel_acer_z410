#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_pm_ldo.h>
    #include <mach/mt_gpio.h>
#endif
#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  dprintf(CRITICAL,fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#define LCD_LOG(fmt, args...)    printk(fmt, ##args)
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1280)

#define LCM_ID_SSD2075 (0x2075)
//#define GPIO_LCD_RST_EN      (GPIO112 | 0x80000000)
//#define GPIO_LCD_PWR_EN      (GPIO7 | 0x80000000)

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xFD//gxf 0xFF   // END OF REGISTERS MARKER


// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)   {mt_set_gpio_mode(GPIO_LCM_RST,GPIO_MODE_00);mt_set_gpio_dir(GPIO_LCM_RST,GPIO_DIR_OUT); mt_set_gpio_out(GPIO_LCM_RST,v);}// (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

#define dsi_set_cmdq_HQ(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_HQ(cmd, count, ppara, force_update)
#define   LCM_DSI_CMD_MODE							0

static struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
}; 


static void lcm_init_power(void)
{
#ifndef FPGA_EARLY_PORTING
#ifdef BUILD_LK
	mt6325_upmu_set_rg_vgp1_en(1);
    mt6325_upmu_set_rg_vgp1_vosel(7);
#else
	printk("%s, begin\n", __func__);
	hwPowerOn(MT6325_POWER_LDO_VGP1, VOL_3300, "LCM_DRV");	
	printk("%s, end\n", __func__);
#endif
	MDELAY(10);
#endif
}

static void lcm_suspend_power(void)
{
#ifndef FPGA_EARLY_PORTING
#ifdef BUILD_LK
	mt6325_upmu_set_rg_vgp1_en(0);
#else
	printk("%s, begin\n", __func__);
	hwPowerDown(MT6325_POWER_LDO_VGP1, "LCM_DRV");	
	printk("%s, end\n", __func__);
#endif
	MDELAY(10);
#endif
}

static void lcm_resume_power(void)
{
#ifndef FPGA_EARLY_PORTING
#ifdef BUILD_LK
	mt6325_upmu_set_rg_vgp1_vosel(7);
	mt6325_upmu_set_rg_vgp1_en(1);
#else
	printk("%s, begin\n", __func__);
	hwPowerOn(MT6325_POWER_LDO_VGP1, VOL_3300, "LCM_DRV");	
	printk("%s, end\n", __func__);
#endif
	MDELAY(10);
#endif
}


static void init_lcm_registers(void)
{
	unsigned int data_array[16];

	/* data_array[0] = 0x00010500; */
	/* dsi_set_cmdq(data_array, 1, 1); */
	/* MDELAY(120); */
	data_array[0] = 0x00010500; // Sleep Out
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
	data_array[0] = 0x00DE0500; // Sleep Out
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x32B41500; // Sleep Out
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x70B31500; // Sleep Out
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x10211500; // Sleep Out
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0] = 0x00DF0500; // Sleep Out
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00110500; // Sleep Out
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	//data_array[0] = 0x606E1500; // Sleep Out
	//dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00290500; // Display On
	dsi_set_cmdq(data_array, 1, 1); 
	MDELAY(20);
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{

	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

       // 1 SSD2075 has no TE Pin
	// enable tearing-free
	params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED;
	//params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode   = BURST_VDO_MODE; //SYNC_EVENT_VDO_MODE;
#endif
	params->physical_width= 62;
	params->physical_height= 110;

	// DSI
	/* Command mode setting */
	//1 Three lane or Four lane
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size=256;

	//video mode timing
	params->dsi.intermediat_buffer_num = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage

	params->dsi.PS						= LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.word_count=720*3;

	params->dsi.vertical_sync_active				= 4;
	params->dsi.vertical_backporch					= 4;
	params->dsi.vertical_frontporch					= 12;
	params->dsi.vertical_active_line				= FRAME_HEIGHT;
	params->dsi.horizontal_sync_active				= 40;
	params->dsi.horizontal_backporch				= 72;
	params->dsi.horizontal_frontporch				= 96;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;


	params->dsi.HS_PRPR=3;
	params->dsi.CLK_HS_POST=22;
	params->dsi.DA_HS_EXIT=20;

	//clock 
        params->dsi.PLL_CLOCK = 255; //this value must be in MTK suggested table
        
#if 0
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0a;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
#endif

}

static void lcm_init(void)
{
	unsigned int data_array[16];

	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(20);
	SET_RESET_PIN(1);
	MDELAY(120);      
	init_lcm_registers();
}


static void lcm_suspend(void)
{
	unsigned int data_array[16];
    #ifdef BUILD_LK
	printf(" tengdq %s\n", __func__);
   #else
	printk(" tengdq %s\n", __func__);
   #endif

	data_array[0]=0x00280500; // Display Off
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);

	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
	
	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	//SET_RESET_PIN(1);

	/* lcd_power_en(0); */
}

static void lcm_resume(void)
{
#if 1
   //1 do lcm init again to solve some display issue
    #ifdef BUILD_LK
	printf(" tengdq %s\n", __func__);
   #else
	printk(" tengdq %s\n", __func__);
   #endif

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	
	SET_RESET_PIN(1);
	MDELAY(120);      

	init_lcm_registers();
#else
	lcm_init();
#endif
}
         
#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	
	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x00290508; //HW bug, so need send one HS packet
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif

static unsigned int lcm_compare_id(void)
{
    unsigned int id,id1,id2=0;
	unsigned char buffer[2];
	unsigned int array[16];  

    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(150);
    SET_RESET_PIN(1);
    MDELAY(200);

	 array[0]=0x00de0500;
       dsi_set_cmdq(array, 1, 1);

	// array[0]=0x00da0500;
    //   dsi_set_cmdq(array, 1, 1);
	   
	array[0] = 0x00023700;// return byte number
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(10);

	read_reg_v2(0xda, buffer, 2);
	id = buffer[0]; 
	   
	array[0] = 0x00023700;// return byte number
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(10);

	read_reg_v2(0xdb, buffer, 2);
	id1 = buffer[0]; 
		   
	array[0] = 0x00023700;// return byte number
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(10);

	read_reg_v2(0xdc, buffer, 2);
	id2 = buffer[0]; 
	
    #ifdef BUILD_LK
	printf("%s, LK ssd2075 id0 = 0x%08x\n", __func__, id);
	printf("%s, LK ssd2075 id1 = 0x%08x\n", __func__, id1);
	printf("%s, LK ssd2075 id = 0x%08x\n", __func__, id2);
   #else
	printk("%s, Kernel ssd2075 id0 = 0x%08x\n", __func__, id);
	printk("%s, Kernel ssd2075 id1 = 0x%08x\n", __func__, id1);
	printk("%s, Kernel ssd2075 id = 0x%08x\n", __func__, id2);
   #endif
	 array[0]=0x00df0500;
       dsi_set_cmdq(array, 1, 1);
  /* return (LCM_ID_SSD2075 == id)?1:0; */
	return 1;


}

LCM_DRIVER tddi4291_jdi_hd_dsi_vdo_lcm_drv = 
{
    .name			= "tddi4291_jdi_hd_dsi_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
//	.compare_id     = lcm_compare_id,
	.init_power		= lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
#if (LCM_DSI_CMD_MODE)
	.update         = lcm_update,
#endif
};
