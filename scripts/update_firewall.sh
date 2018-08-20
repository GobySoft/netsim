#!/bin/bash
 
ip=$1

for i in `seq 61999 62007`; do
    for j in `sudo ufw status numbered | grep $i/tcp | cut -b 2-3`; do sudo ufw --force delete $j; done
    sudo ufw allow proto tcp from $1 to 172.19.21.10 port $i
done

sudo systemctl restart netsim_socat@{0..7}

sudo ufw status
