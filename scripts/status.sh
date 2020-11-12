#!/bin/bash

systemctl status netsim_core goby_logger netsim_manager netsim_moos-atlantic netsim_moos_gateway-atlantic netsim_moos-arctic netsim_moos_gateway-arctic gobyd jackd netsim_socat@{0..7} netsim_postprocess netsim_liaison

