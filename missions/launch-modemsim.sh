#!/bin/bash


function launch()
{
    name=$1
    command=$2
    slpsec=$3
    screen -L -D -m -S $name $command &
    PID="$! $PID"
    sleep $slpsec
}

launch fixlevels "./fix_fast_track_levels.sh" 0
launch gobyd "gobyd" 1
launch jackd "jackd -dalsa -dhw:2 -r96000 -p1024 -n3 -H -M" 0
launch goby_moos_gateway "goby_moos_gateway --plugin_library /home/toby/opensource/netsim/build/lib/libnetsim_moos_plugin.so --moos_port 9400 -vvv" 0


trap "kill $PID;" EXIT

while [ 1 ]; do
    screen -ls
    sleep 1
done;
