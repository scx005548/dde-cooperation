#!/bin/bash
#echo $* > /tmp/tmp.txt

path=""
for var in $@
do
        path="$path$var"","
done
path=${path%?}

dbus-send --print-reply --dest=org.deepin.dde.Cooperation1 /org/deepin/dde/Cooperation1 org.deepin.dde.Cooperation1.SendFile array:string:$path int32:2
