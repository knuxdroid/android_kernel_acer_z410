/*******************************************************************************
 *
 * Filename:
 * ---------
 *   CFG_PRODUCT_INFO_File.h
 *
 * Project:
 * --------
 *   YuSu
 *
 * Description:
 * ------------
 *    header file of main function
 *
 * Author:
 * -------
 *   Yuchi Xu(MTK81073)
 *
 *------------------------------------------------------------------------------
 *
 *******************************************************************************/



#ifndef _CFG_PRODUCT_INFO_FILE_H
#define _CFG_PRODUCT_INFO_FILE_H


// the record structure define of PRODUCT_INFO nvram file
typedef struct
{
    unsigned char imei[8];
    unsigned char svn;
    unsigned char pad;
} nvram_ef_imei_imeisv_struct;

typedef struct{
		unsigned char barcode[64];
		nvram_ef_imei_imeisv_struct IMEI[4];
		unsigned char reserved[1024-40-64];
}PRODUCT_INFO;

//the record size and number of PRODUCT_INFO nvram file
#define CFG_FILE_PRODUCT_INFO_SIZE    sizeof(PRODUCT_INFO)
#define CFG_FILE_PRODUCT_INFO_TOTAL   1

#ifdef GSENSOR_CALI_BACKUP_TO_PRO_INFO
//pei_add_start
#include "../cfgfileinc/CFG_HWMON_File.h"

//the new record for the hwmon sensor backup data
#define	CFG_HWMON_BACKUP_RESERVED_SIZE	(1024-CFG_FILE_HWMON_ACC_REC_SIZE)// -CFG_FILE_HWMON_GYRO_REC_SIZE  //-CFG_FILE_HWMON_PS_REC_SIZE)

typedef struct{
	NVRAM_HWMON_ACC_STRUCT	hwmon_acc;
	//NVRAM_HWMON_GYRO_STRUCT	hwmon_gyro;
	//NVRAM_HWMON_PS_STRUCT	hwmon_ps;     //pei_mask
	unsigned char reserved[CFG_HWMON_BACKUP_RESERVED_SIZE];
}HWMON_BACKUP_STRUCT;

#define	CFG_FILE_HWMON_BACKUP_SIZE		sizeof(HWMON_BACKUP_STRUCT)
#define	CFG_FILE_HWMON_BACKUP_TOTAL		1

//pei_add_end
#endif
#endif /* _CFG_PRODUCT_INFO_FILE_H */
