# netsim_manager

The `netsim_manager` process is responsible for:

 - Interfacing with the vehicle simulators
     - Maintaining a record of the current position of the simulated vehicles
     - Writing simulated vehicle position data as a fake GPS serial stream
 - Forwarding various messages to the correct environment in the Virtual Ocean simulator
     - Channel impulse responses (netsim::groups::impulse_request / netsim::groups::impulse_response)
     - Predicted performance metric data (netsim::groups::performance_request / netsim::groups::performance_response)
     - BELLHOP model requests (netsim::groups::bellhop_request / netsim::groups::bellhop_response): used by Liaison tab for visualizing predicted transmission loss.

For detail on the specific publish/subscribe interfaces, see the [Architecture](page10_architecture.md) page.

## Configuration

Configuration is given as a Protobuf TextFormat file for the netsim::protobuf::NetSimManagerConfig message.

All available configuration values can be obtained by the `-e` flag to `netsim_manager`.

For example,
```
netsim_manager -e
```

yields

```
app { ... } # standard Goby3 app configuration
interprocess { ... } # standard Goby3 interprocess configuration (for connecting to gobyd)
sim_env_pair {  # repeated for each expected simulated vehicle 
                # (repeated)
  modem_tcp_port:   # TCP port used for connecting to the modem 
                    # data interface (62000, 62001, etc. on 
                    # audioserver). This is used as a unique 
                    # identifier for a given vehicle simulator 
                    # (required)
  endpoint_ip_address: ""  # IP address for connecting remote 
                           # endpoint (required)
  environment_id:   # unique id used to map to env_bounds{} 
                    # (required)
}
env_bounds {  # validity bounding region of the ocean for each 
              # Virtual Ocean environment (repeated)
  environment_id:   # unique id for this environment (required)
  lat_min:   # minimum latitude in decimal degrees (required)
  lon_min:   # minimum longitude in decimal degrees (required)
  lat_max:   # maximum latitude in decimal degrees (required)
  lon_max:   # maximum longitude in decimal degrees (required)
  depth_min: 0  # minimum depth in meters (optional) (default=0)
  depth_max:   # maximum depth in meters (required)
}
gps_out {  # Set if a synthesized $GPRMC GPS message is desired 
           # for a given simulated vehicle (repeated)
  modem_tcp_port:   # Which vehicle to output, as referenced by 
                    # its modem TCP port set in sim_env_pair{} 
                    # (required)
  serial {  # Output serial settings (required)
    port: "/dev/ttyUSB0"  # Serial port path (required)
    baud: 57600  # Serial baud (required)
    end_of_line: "\n"  # End of line string. Can also be a 
                       # std::regex (optional) (default="\n")
    flow_control: NONE  # Flow control: NONE, SOFTWARE (aka 
                        # XON/XOFF), HARDWARE (aka RTS/CTS) (NONE, 
                        # SOFTWARE, HARDWARE) (optional) 
                        # (default=NONE)
  }
}
```

Each vehicle simulator is indexed using the TCP server port that it uses to connect to `socat` (which forwards the TCP data to the actual modem interface over serial, TCP, UDP, etc.). On the *audioserver* these values are 62000-62007. `netsim_manager` itself uses TCP port 61999 by default for the interface with the vehicle simulators (see [Vehicle Simulator Connections](page15_vehicle_sim_connections.md) page).

Use the `sim_env_pair` field, each vehicle simulator must be mapped to a desired *environment* (region of the ocean) that is operating in. These environments are defined by the bounds set in the corresponding `env_bounds` field. These bounds should be set to ensure that the underlying models used to produce the impulse responses, etc. are valid within them.


