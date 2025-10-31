#!/bin/bash

#User provided arguments. Expect Full path to these files 
FLASHMAP_MAKEFILE=$1

if [[ ! (-e $FLASHMAP_MAKEFILE) ]];then
    echo "Error: Flashmap Makefile can't be found "
    exit 1
fi

source $FLASHMAP_MAKEFILE

APP_1_BIN_SIZE=$(ls -g -o ./../build/${APP_1_NAME}.bin | awk '{printf $3}')
APP_2_BIN_SIZE=$(ls -g -o ./../build/${APP_2_NAME}.bin | awk '{printf $3}')
APP_3_BIN_SIZE=$(ls -g -o ./../build/${APP_3_NAME}.bin | awk '{printf $3}')

echo "APP_1_BIN_SIZE = ${APP_1_BIN_SIZE}"
echo "APP_2_BIN_SIZE = ${APP_2_BIN_SIZE}"
echo "APP_3_BIN_SIZE = ${APP_3_BIN_SIZE}"

# get size of binary file for components.json
CY_COMPONENTS_JSON_NAME=./../build/components.json
CY_OUTPUT_FILE_NAME_TAR=./ota-update.tar

cd ./../build/

# create "components.json" file
echo "{\"numberOfComponents\":\"4\",\"version\":\"${APP_BUILD_VERSION}\",\"files\":["                    >  $CY_COMPONENTS_JSON_NAME
echo "{\"fileName\":\"components.json\"},"                             >> $CY_COMPONENTS_JSON_NAME
echo "{\"fileName\":\"${APP_1_NAME}.bin\",\"fileID\":\"1\",\"fileStartAdd\": \"$FLASH_AREA_IMG_1_SECONDARY_START\",\"fileSize\":\"$APP_1_BIN_SIZE\"}," >> $CY_COMPONENTS_JSON_NAME
echo "{\"fileName\":\"${APP_2_NAME}.bin\",\"fileID\":\"2\",\"fileStartAdd\": \"$FLASH_AREA_IMG_2_SECONDARY_START\",\"fileSize\":\"$APP_2_BIN_SIZE\"}," >> $CY_COMPONENTS_JSON_NAME
echo "{\"fileName\":\"${APP_3_NAME}.bin\",\"fileID\":\"3\",\"fileStartAdd\": \"$FLASH_AREA_IMG_3_SECONDARY_START\",\"fileSize\":\"$APP_3_BIN_SIZE\"}]}" >> $CY_COMPONENTS_JSON_NAME

# create tarball for OTA
echo "Create tarball"
echo "tar -cf $CY_OUTPUT_FILE_NAME_TAR components.json proj_cm33_s.bin proj_cm33_ns.bin proj_cm55.bin"
tar -cf $CY_OUTPUT_FILE_NAME_TAR components.json proj_cm33_s.bin proj_cm33_ns.bin proj_cm55.bin
if [[ ! $? = 0 ]]; then
    echo "postbuild error: tarball is not generated"
    exit 1
fi




