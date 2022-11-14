#!/bin/bash

DESKTOP_TEMP_FILE=misc/config/compressor-singlefile.config.tmp
DESKTOP_SOURCE_FILE=misc/config/compressor-singlefile.config

DESKTOP_TS_DIR=translations/config/

/usr/bin/deepin-policy-ts-convert ts2policy $DESKTOP_SOURCE_FILE $DESKTOP_TS_DIR $DESKTOP_TEMP_FILE
# mv $DESKTOP_TEMP_FILE $DESKTOP_SOURCE_FILE
