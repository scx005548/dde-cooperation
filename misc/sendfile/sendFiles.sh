#!/bin/bash
#echo $* > /tmp/tmp.txt

path=""
for var in $@
do
        path="$path$var"","
done
path=${path%?}

dbus-send --print-reply --dest=com.deepin.Cooperation /com/deepin/Cooperation com.deepin.Cooperation.SendFile array:string:$path int32:2