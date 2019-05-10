#!/bin/bash

SIMCOM_CUST_BUILD_BASEBAND=TRUE
SIMCOM_CUST_PROJECT=simcom6752_lwt_cu
if [ $# = 1 ]
then
   SIMCOM_CUST_PROJECT=$1   
fi


SIMCOM_MODEM_STRING=`cat mediatek/config/$SIMCOM_CUST_PROJECT/ProjectConfig.mk | grep "SIMCOM_ACER_MODEM_VER_STRING"  | sed 's/SIMCOM_ACER_MODEM_VER_STRING//g' | sed 's/=//g' | sed s/\ //g`

sed -i 's/{ \"ro.boot.baseband\", \"ro.baseband\", \".*\", },/{ \"ro.boot.baseband\", \"ro.baseband\", \"'$SIMCOM_MODEM_STRING'\", },/g' ./system/core/init/init.c

echo $SIMCOM_MODEM_STRING
grep "{ \"ro.boot.baseband\", \"ro.baseband\", "  system/core/init/init.c
