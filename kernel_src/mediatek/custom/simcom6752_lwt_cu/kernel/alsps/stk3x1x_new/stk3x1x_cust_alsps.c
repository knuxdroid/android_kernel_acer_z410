/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/*
#include <linux/types.h>
#include <cust_alsps.h>
#ifdef MT6573
#include <mach/mt6573_pll.h>
#endif
#ifdef MT6575
#include <mach/mt6575_pm_ldo.h>
#endif
#ifdef MT6577
#include <mach/mt6577_pm_ldo.h>
#endif
#ifdef MT6589
#include <mach/mt_pm_ldo.h>
#endif
*/
#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <stk3x1x_cust_alsps.h>



static struct alsps_hw cust_alsps_hw = {
	.i2c_num    = 1,		//i2c bus number, for mt657x, default=0. For mt6589, default=3 
	//.polling_mode =1,
	.polling_mode_ps =0,
	.polling_mode_als =1,
	.power_id   = MT65XX_POWER_NONE , //MT6323_POWER_LDO_VGP1,    //LDO is not used
    .power_vol  = VOL_DEFAULT,          /*LDO is not used*/
//	.i2c_addr   = {0x90, 0x00, 0x00, 0x00},	//STK3x1x
//	.als_level  = {5,  9, 36, 59, 82, 132, 205, 273, 500, 845, 1136, 1545, 2364, 4655, 6982},	//als_code 
//	.als_value  = {0, 10, 40, 65, 90, 145, 225, 300, 550, 930, 1250, 1700, 2600, 5120, 7680, 10240},   //lux 
/*
   	.als_level  = { 0,  1,  1,   7,  15,  15,  100, 1000, 2000,  3000,  6000, 10000, 14000, 18000, 20000},
    .als_value  = {40, 40, 90,  90, 160, 160,  225,  320,  640,  1280,  1280,  2600,  2600, 2600,  10240, 10240},*/
     //for aal modify-mly
//   	.als_level  = { 0, 135, 210, 210, 315, 678, 678, 1353, 2190, 4011, 5486, 8627, 10505, 11699, 11699},
//        .als_value  = { 90, 90, 90, 130, 130, 400, 710, 1426, 2290, 4280, 5745, 9034, 11000, 12250, 12250, 12250},
//pei_modify 3
   	.als_level  = { 0, 10, 40, 70, 100, 200, 300, 1353, 2190, 4011, 5486, 8627, 10505, 11699, 11699},
    .als_value  = { 30, 100, 200, 600, 1500, 2000, 2500, 2800, 3000, 4280, 5745, 9034, 11000, 12250, 12250, 12250},
   	.state_val = 0x0,		/* disable all */
	.psctrl_val = 0x31,		/* ps_persistance=1, ps_gain=64X, PS_IT=0.391ms */
	.alsctrl_val = 0x2A,	/* als_persistance=1, als_gain=16X, ALS_IT=200ms */
	.ledctrl_val = 0xFF,	/* 100mA IRDR, 64/64 LED duty */
	.wait_val = 0x7,		/* 50 ms */

	.ps_high_thd_val = 1700,
	.ps_low_thd_val = 1500,
};


/*
static struct alsps_hw cust_alsps_hw = {
	.i2c_num    = 2,
	.polling_mode_ps =0,
	.polling_mode_als =1,
	.power_id   = MT6323_POWER_LDO_VGP1,    //LDO is not used
	.power_vol  = VOL_2800,          //LDO is not used
	.i2c_addr   = {0x72, 0x48, 0x78, 0x00},
	//enovo-sw chenlj2 add 2011-06-03,modify parameter below two lines

	.als_level  = { 5, 10,  25,   50,  100, 150,  200, 400, 1000,  1500, 2000, 3000, 5000, 8000, 10000},
	.als_value  = {10, 50,  100,  150, 200, 250,  280,  280, 1600,  1600,  1600,  6000,  6000, 9000,  10240, 10240},
	//.als_level  = { 4, 40,  80,   120,   160, 250,  400, 800, 1200,  1600, 2000, 3000, 5000, 10000, 65535},
	//.als_value  = {10, 20,20,  120, 120, 280,  280,  280, 1600,  1600,  1600,  6000,  6000, 9000,  10240, 10240},  
	.ps_threshold_high = 300,
	.ps_threshold_low = 200,
	.ps_threshold = 900,
};
*/
struct alsps_hw *stk3x1x_get_cust_alsps_hw(void) {
    return &cust_alsps_hw;
}


