sudo systemctl daemon-reload
sudo systemctl stop jackd
sleep 2
sudo systemctl start jackd
sudo systemctl restart netsim_core netsim_manager gobyd goby_logger netsim_socat@0 netsim_socat@1 netsim_socat@2 netsim_socat@3 netsim_postprocess netsim_liaison
sudo systemctl restart netsim_moos-atlantic netsim_moos_gateway-atlantic
sleep 2
sudo systemctl restart netsim_moos-arctic netsim_moos_gateway-arctic 
