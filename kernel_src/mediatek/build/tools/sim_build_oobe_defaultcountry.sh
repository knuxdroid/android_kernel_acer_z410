#!/bin/bash
. ~/.bashrc

SIMCOM_CUST_PROJECT=simcom6752_lwt_cu
if [ $# = 1 ]
then
   SIMCOM_CUST_PROJECT=$1   
fi


SIMCOM_OOBE_DEFAULT_COUNTRY=`cat mediatek/config/$SIMCOM_CUST_PROJECT/ProjectConfig.mk | grep "SIMCOM_ACER_OOBE_COUNTRY"  | sed 's/SIMCOM_ACER_OOBE_COUNTRY//g' | sed 's/=//g' | sed s/\ //g`

if [ "x"$SIMCOM_OOBE_DEFAULT_COUNTRY == "x" ]
then
SIMCOM_OOBE_DEFAULT_COUNTRY=GBR
fi

SIMCOM_OOBE_FILE_NAME=vendor/mediatek/$SIMCOM_CUST_PROJECT/artifacts/out/target/product/$SIMCOM_CUST_PROJECT/system/etc/oobe_country.xml


SIMCOM_OOBE_DEFAULT_COUNTRY_STRING1="<?xml version=\"1.0\" encoding=\"utf-8\"?>"
SIMCOM_OOBE_DEFAULT_COUNTRY_STRING2="<country default=\"$SIMCOM_OOBE_DEFAULT_COUNTRY\"/>"
echo $SIMCOM_OOBE_DEFAULT_COUNTRY_STRING1 > $SIMCOM_OOBE_FILE_NAME
echo $SIMCOM_OOBE_DEFAULT_COUNTRY_STRING2 >> $SIMCOM_OOBE_FILE_NAME
echo "Country Code:" $SIMCOM_OOBE_DEFAULT_COUNTRY  "project:" $SIMCOM_CUST_PROJECT
echo "gen oobe file:"$SIMCOM_OOBE_FILE_NAME

