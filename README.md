# LED Blinking Daemon

Blinks LEDs on Linux systems based whenever the content of a "monitor" file changes. This is useful for example to blink and LED whenever there is network activity on a certain network interface.

### Compile and install

Simply run `make` in the root directory of this repository. This program has no dependencies apart from the standard library.

```
git clone https://github.com/astoeckel/ledd
cd ledd
make
```

To install, copy to a directory that is in the path, e.g. `/usr/local/bin/`.

### How to use

Run the following command to blink the first LED of the PC Engines APU2 boardwhenever there is network activity on the specified network interface.

```
ledd --led \
	'/sys/devices/platform/leds_apu/leds/apu2:green:1/brightness' \
	'/sys/class/net/enp1s0/statistics/rx_bytes' \
	'/sys/class/net/enp1s0/statistics/tx_bytes'
```

You can list up to 16 monitor files per LED and define up to 16 LEDs. See `ledd.service` for an example `systemd` unit file that starts the daemon whenever the system boots.
