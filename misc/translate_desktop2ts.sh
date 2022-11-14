#!/bin/bash

DESKTOP_SOURCE_FILE=config/compressor-singlefile.conf
DESKTOP_TS_DIR=../translations/config/

/usr/bin/deepin-desktop-ts-convert desktop2ts $DESKTOP_SOURCE_FILE $DESKTOP_TS_DIR
