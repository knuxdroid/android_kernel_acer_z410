#ifndef _CUST_BATTERY_METER_H
#define _CUST_BATTERY_METER_H

#include <mach/mt_typedefs.h>

// ============================================================
// define
// ============================================================
//#define SOC_BY_AUXADC
#define SOC_BY_HW_FG
//#define SOC_BY_SW_FG
//#define HW_FG_FORCE_USE_SW_OCV

//#define CONFIG_DIS_CHECK_BATTERY
//#define FIXED_TBAT_25

/* ADC Channel Number */
#if 0
#define CUST_TABT_NUMBER 17
#define VBAT_CHANNEL_NUMBER      7
#define ISENSE_CHANNEL_NUMBER	 6
#define VCHARGER_CHANNEL_NUMBER  4
#define VBATTEMP_CHANNEL_NUMBER  5
#endif
/* ADC resistor  */
#define R_BAT_SENSE 4					
#define R_I_SENSE 4						
#define R_CHARGER_1 330
#define R_CHARGER_2 39

#define TEMPERATURE_T0             110
#define TEMPERATURE_T1             0
#define TEMPERATURE_T2             25
#define TEMPERATURE_T3             50
#define TEMPERATURE_T              255 // This should be fixed, never change the value

#if defined(ACER_JADEL)
#define FG_METER_RESISTANCE 	20
#else
#define FG_METER_RESISTANCE 	0
#endif

#if defined(ACER_JADEL)
/* Qmax for battery  */
#define Q_MAX_POS_50	2283
#define Q_MAX_POS_25	2317
#define Q_MAX_POS_0		2302
#define Q_MAX_NEG_10	2312

#define Q_MAX_POS_50_H_CURRENT	2272
#define Q_MAX_POS_25_H_CURRENT	2296
#define Q_MAX_POS_0_H_CURRENT	1811
#define Q_MAX_NEG_10_H_CURRENT	1193
#elif defined(ACER_Z410)
/* Qmax for battery  */
#define Q_MAX_POS_50	2097
#define Q_MAX_POS_25	2136
#define Q_MAX_POS_0		2069
#define Q_MAX_NEG_10	2145

#define Q_MAX_POS_50_H_CURRENT	2050
#define Q_MAX_POS_25_H_CURRENT	2082
#define Q_MAX_POS_0_H_CURRENT	1925
#define Q_MAX_NEG_10_H_CURRENT	1393
#else
/* Qmax for battery  */
#define Q_MAX_POS_50	2500
#define Q_MAX_POS_25	2432
#define Q_MAX_POS_0		2244
#define Q_MAX_NEG_10	1654

#define Q_MAX_POS_50_H_CURRENT	2500
#define Q_MAX_POS_25_H_CURRENT	2390
#define Q_MAX_POS_0_H_CURRENT	1814
#define Q_MAX_NEG_10_H_CURRENT	696
#endif

/* Discharge Percentage */
#define OAM_D5		 1		//  1 : D5,   0: D2


/* battery meter parameter */
#define CHANGE_TRACKING_POINT
#if defined(ACER_JADEL)
#define CUST_TRACKING_POINT  14
#else
#define CUST_TRACKING_POINT  1
#endif
//punk,68->56
#define CUST_R_SENSE         56
#define CUST_HW_CC 		    0
#define AGING_TUNING_VALUE   103
#define CUST_R_FG_OFFSET    0

#define OCV_BOARD_COMPESATE	0 //mV 
#define R_FG_BOARD_BASE		1000
#define R_FG_BOARD_SLOPE	1000 //slope
#if defined(ACER_Z410)
#define CAR_TUNE_VALUE		95 //97 //1.00
#elif  defined(ACER_JADEL)
#define CAR_TUNE_VALUE		96 //1.00
#else
#define CAR_TUNE_VALUE		97 //1.00
#endif

/* HW Fuel gague  */
#define CURRENT_DETECT_R_FG	10  //1mA
#define MinErrorOffset       1000
#define FG_VBAT_AVERAGE_SIZE 18
#define R_FG_VALUE 			10 // mOhm, base is 20

#if defined(ACER_Z410)
#define CUST_POWERON_DELTA_CAPACITY_TOLRANCE	10
#define CUST_POWERON_LOW_CAPACITY_TOLRANCE		5
#define CUST_POWERON_MAX_VBAT_TOLRANCE			90
#define CUST_POWERON_DELTA_VBAT_TOLRANCE		10
#else
#define CUST_POWERON_DELTA_CAPACITY_TOLRANCE	25
#define CUST_POWERON_LOW_CAPACITY_TOLRANCE		5
#define CUST_POWERON_MAX_VBAT_TOLRANCE			90
#define CUST_POWERON_DELTA_VBAT_TOLRANCE		30
#endif

/* Disable Battery check for HQA */
#ifdef MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
#define FIXED_TBAT_25
#endif

/* Dynamic change wake up period of battery thread when suspend*/
#define VBAT_NORMAL_WAKEUP		3600		//3.6V
#define VBAT_LOW_POWER_WAKEUP		3500		//3.5v
#define NORMAL_WAKEUP_PERIOD		5400 		//90 * 60 = 90 min
#define LOW_POWER_WAKEUP_PERIOD		300		//5 * 60 = 5 min
#define CLOSE_POWEROFF_WAKEUP_PERIOD	30	//30 s

#define INIT_SOC_BY_SW_SOC
//#define SYNC_UI_SOC_IMM			//3. UI SOC sync to FG SOC immediately
#define MTK_ENABLE_AGING_ALGORITHM	//6. Q_MAX aging algorithm
#define MD_SLEEP_CURRENT_CHECK	//5. Gauge Adjust by OCV 9. MD sleep current check
#define Q_MAX_BY_CURRENT		//7. Qmax varient by current loading.

#define DISABLE_RFG_EXIST_CHECK
#endif	//#ifndef _CUST_BATTERY_METER_H
