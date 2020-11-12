# Configuration

The configuration of the *audioserver* reference system forms a starting point for custom configurations of the netsim software.

## systemd services

Systemd service definitions are defined in the files `netsim/configs/audioserver/systemd`, which will initiate the various processes in the netsim system in the correct order.


### jackd.service

The Jack Daemon launch file (`jackd.service`) initiates the Ultra 8R sound card on the *audioserver* test rig. This sound card driver has a bug that enables all channels in ALSA (including the internal crossover channels that feed audio from one input to another output) at maximum volume. Thus, this service first runs a script (`fix_fast_track_levels.sh`) that turns off these crossover channels. When using a different sound card, this will not likely be required.

### configuring service files

For more information on configuring systemd service files, see the official documentation: <https://www.freedesktop.org/software/systemd/man/systemd.unit.html>.

## process configuration

Configuration for the *netsim* processes is stored in `netsim/missions` in the various `*.pb.cfg` files (in Protobuf TextFormat).

To see the available configuration from any of the Goby processes, run it with the `-e` flag, for example: `netsim_manager -e`.