#!/bin/bash
Ultra=$(/usr/bin/aplay -l | /bin/grep F8R | cut -d ":" -f 1 | cut -d " " -f 2)

i=0
J=0
for i in $(seq 8); do
for j in $(seq 8); do
if [ "$i" != "$j" ]; then
amixer -c ${Ultra} set "DIn$i - Out$j" 0% > /dev/null
else
amixer -c ${Ultra} set "DIn$i - Out$j" 100% > /dev/null
fi
amixer -c ${Ultra} set "AIn$i - Out$j" 0% > /dev/null
done
amixer -c ${Ultra} set "Effect Send AIn$i" 0% > /dev/null
amixer -c ${Ultra} set "Effect Send DIn$i" 0% > /dev/null
done
i=0
for i in $(seq 4); do
amixer -c ${Ultra} set "Effect Return $i" 0% > /dev/null
done
