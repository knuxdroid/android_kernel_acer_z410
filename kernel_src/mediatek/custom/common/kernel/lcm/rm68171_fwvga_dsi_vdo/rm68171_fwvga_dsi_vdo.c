#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>       ///for printk
#endif

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(480)
#define FRAME_HEIGHT 										(854)

//punk, fe->fd
#define REGFLAG_DELAY             							0XFD
#define REGFLAG_END_OF_TABLE      							0xFF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

static struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {
	
	/*
	Note :

	Data ID will depends on the following rule.
	
		count of parameters > 1	=> Data ID = 0x39
		count of parameters = 1	=> Data ID = 0x15
		count of parameters = 0	=> Data ID = 0x05

	Structure Format :

	{DCS command, count of parameters, {parameter list}}
	{REGFLAG_DELAY, milliseconds of time, {}},

	...

	Setting ending by predefined flag
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	*/

	{0xF0,5,{0x55,0xAA,0x52,0x08,0x02}},
	{0xFE,5,{0x00,0x80,0x09,0x18,0xA0}},
	{0xF6,1,{0x60}},
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x00}},
	{0xC9,5,{0xD3,0x02,0x38,0x38,0x38}},
	{0xB7,2,{0x77,0x77}},
	{0xE8,2,{0x00,0xB3}},
	{0xB0,2,{0x00,0x0E}},
	{0xB5,1,{0x6E}},
	{0xB4,1,{0x10}},
	{0xB8,4,{0x01,0x03,0x03,0x03}},
	{0xBC,3,{0x02,0x02,0x02}},
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x01}},
	{0xB0,3,{0x0B,0x0B,0x0B}},
	{0xB1,3,{0x0B,0x0B,0x0B}},
	{0xB5,3,{0x08,0x08,0x08}},
	{0xB6,3,{0x34,0x34,0x34}},
	{0xB7,3,{0x34,0x34,0x34}},
	{0xB8,3,{0x24,0x24,0x24}},
	{0xB9,3,{0x34,0x34,0x34}},
	{0xBA,3,{0x14,0x14,0x14}},
	{0xBC,3,{0x00,0x78,0x13}},
	{0xBD,3,{0x00,0x78,0x13}},
	{0xBE,2,{0x00,0x60}},
	{0xD1,52,{0x00,0x00,0x00,0x02,0x00,0x0E,0x00,0x2B,0x00,0x50,0x00,0xA3,0x00,0xDF,0x01,0x39,0x01,0x74,0x01,0xC2,0x01,0xF7,0x02,0x3F,0x02,0x76,0x02,0x78,0x02,0xA8,0x02,0xD8,0x02,0xF2,0x03,0x11,0x03,0x24,0x03,0x3D,0x03,0x4D,0x03,0x63,0x03,0x72,0x03,0x89,0x03,0xB4,0x03,0xFC}},
	{0xD2,52,{0x00,0x00,0x00,0x02,0x00,0x0E,0x00,0x2B,0x00,0x50,0x00,0xA3,0x00,0xDF,0x01,0x39,0x01,0x74,0x01,0xC2,0x01,0xF7,0x02,0x3F,0x02,0x76,0x02,0x78,0x02,0xA8,0x02,0xD8,0x02,0xF2,0x03,0x11,0x03,0x24,0x03,0x3D,0x03,0x4D,0x03,0x63,0x03,0x72,0x03,0x89,0x03,0xB4,0x03,0xFC}},
	{0xD3,52,{0x00,0x00,0x00,0x02,0x00,0x0E,0x00,0x2B,0x00,0x50,0x00,0xA3,0x00,0xDF,0x01,0x39,0x01,0x74,0x01,0xC2,0x01,0xF7,0x02,0x3F,0x02,0x76,0x02,0x78,0x02,0xA8,0x02,0xD8,0x02,0xF2,0x03,0x11,0x03,0x24,0x03,0x3D,0x03,0x4D,0x03,0x63,0x03,0x72,0x03,0x89,0x03,0xB4,0x03,0xFC}},
	{0xD4,52,{0x00,0x00,0x00,0x02,0x00,0x0E,0x00,0x2B,0x00,0x50,0x00,0xA3,0x00,0xDF,0x01,0x39,0x01,0x74,0x01,0xC2,0x01,0xF7,0x02,0x3F,0x02,0x76,0x02,0x78,0x02,0xA8,0x02,0xD8,0x02,0xF2,0x03,0x11,0x03,0x24,0x03,0x3D,0x03,0x4D,0x03,0x63,0x03,0x72,0x03,0x89,0x03,0xB4,0x03,0xFC}},
	{0xD5,52,{0x00,0x00,0x00,0x02,0x00,0x0E,0x00,0x2B,0x00,0x50,0x00,0xA3,0x00,0xDF,0x01,0x39,0x01,0x74,0x01,0xC2,0x01,0xF7,0x02,0x3F,0x02,0x76,0x02,0x78,0x02,0xA8,0x02,0xD8,0x02,0xF2,0x03,0x11,0x03,0x24,0x03,0x3D,0x03,0x4D,0x03,0x63,0x03,0x72,0x03,0x89,0x03,0xB4,0x03,0xFC}},
	{0xD6,52,{0x00,0x00,0x00,0x02,0x00,0x0E,0x00,0x2B,0x00,0x50,0x00,0xA3,0x00,0xDF,0x01,0x39,0x01,0x74,0x01,0xC2,0x01,0xF7,0x02,0x3F,0x02,0x76,0x02,0x78,0x02,0xA8,0x02,0xD8,0x02,0xF2,0x03,0x11,0x03,0x24,0x03,0x3D,0x03,0x4D,0x03,0x63,0x03,0x72,0x03,0x89,0x03,0xB4,0x03,0xFC}},
	{0x36,1,{0x00}},
	{0x3A,1,{0x77}},
	{0x11,0,{0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x29,0,{0x00}},
	{REGFLAG_DELAY, 100, {}},

};



static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 0, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
	{0x29, 0, {0x00}},
    {REGFLAG_DELAY, 100, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 0, {0x00}},

    // Sleep Mode On
	{0x10, 0, {0x00}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};




static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
				//UDELAY(5);//soso add or it will fail to send register
       	}
    }
	
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


		params->dsi.mode   = SYNC_EVENT_VDO_MODE;
	
		// enable tearing-free
		params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED;
		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
		
		// DSI
		/* Command mode setting */
		params->dsi.LANE_NUM				= LCM_TWO_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Highly depends on LCD driver capability.
		// Not support in MT6573
		params->dsi.packet_size=256;

		// Video mode setting		
		params->dsi.intermediat_buffer_num = 2;

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

		params->dsi.vertical_sync_active				= 4;
		params->dsi.vertical_backporch					= 12;
		params->dsi.vertical_frontporch					= 12;
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 8;
		params->dsi.horizontal_backporch				= 64;
		params->dsi.horizontal_frontporch				= 64;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

		//punk
#ifndef FPGA_EARLY_PORTING
        params->dsi.PLL_CLOCK = 220; //this value must be in MTK suggested table
#else
		xxx
		// Bit rate calculation
		params->dsi.pll_div1=37;		// fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
		params->dsi.pll_div2=1; 		// div2=0~15: fout=fvo/(2*div2)
#endif
}

static unsigned int lcm_compare_id(void);

static void lcm_init(void)
{
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(6);//Must > 5ms
    SET_RESET_PIN(1);
    MDELAY(50);//Must > 50ms

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	static int cnt=0;
	unsigned int data_array[2];
#ifndef BUILD_LK
    printk("tek.xing lcm_suspend rm68171\n");

    data_array[0]=0x00011500;
    dsi_set_cmdq(&data_array,1,1);
    MDELAY(50);
    data_array[0]=0x00280500;
    dsi_set_cmdq(&data_array,1,1);
    MDELAY(50);
    data_array[0]=0x00100500;
    dsi_set_cmdq(&data_array,1,1);
    MDELAY(50);

      //  push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);   //wqtao. enable
#endif

}


static void lcm_resume(void)
{
#if !defined(BUILD_LK)
	printk("tek.xing lcm_suspend rm68171\n");
#endif
	lcm_init();
	
//	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}
static unsigned int lcm_compare_id(void)
{
    unsigned int id0 = 0, id1 = 0;
    unsigned char buffer[2];

    unsigned int data_array[16];
    
	SET_RESET_PIN(1);  //NOTE:should reset LCM firstly
	MDELAY(6);
	SET_RESET_PIN(0);
	MDELAY(6);
	SET_RESET_PIN(1);
	MDELAY(120);

    data_array[0]=0x00063902;
    data_array[1]=0x52AA55F0;
    data_array[2]=0x00000108;
    dsi_set_cmdq(data_array, 3, 1);
    MDELAY(10);

    data_array[0] = 0x00023700;// read id return two byte,version and id
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);

    read_reg_v2(0xC5, buffer, 2);
    id0 = buffer[0]; //we only need ID
    id1 = buffer[1]; //we test buffer 1
    //id0 = (id0<<8);
    //id0 += id1;
#ifdef BUILD_LK
    printf("rm68171 id = 0x%08x\n", id0);
#else
    printk("rm68171 id = 0x%08x\n", id0);
#endif
    
    return (id0 == 0x71)?1:0;
}


LCM_DRIVER rm68171_dsi_vdo_lcm_drv = 
{
    .name			= "rm68171_dsi_vdo_lcm_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id   	= lcm_compare_id,
};

