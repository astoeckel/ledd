# LED Blinking Daemon

Causes LEDs made available through the Linux LED subsystem to blink whenever the content of a "monitor" file changes. This is useful to indicate network activity on embedded devices.

## Compile and install

Simply run `make` in the root directory of this repository. Assumes that you have `gcc` and `make` installed. This program has no dependencies apart from the `C` standard library.

```
git clone https://github.com/astoeckel/ledd
cd ledd
make
```

To install, copy to a directory that is in the path, e.g. `/usr/local/bin/`.

## How to use

Run the following command to blink the first LED of the PC Engines APU2 board whenever there is network activity on the specified network interface.

```
ledd --led \
	'/sys/devices/platform/leds_apu/leds/apu2:green:1/brightness' \
	'/sys/class/net/enp1s0/statistics/rx_bytes' \
	'/sys/class/net/enp1s0/statistics/tx_bytes'
```

You can list up to 16 monitor files per LED and define up to 16 LEDs. The `ledd` daemon will write `0\n` to the LED file (the first file after each `--led` argument) whenever the LED should be off and `1\n` whenever it should be active. Correspondingly, you can also use this program to toggle GPIO pins exposed via the Linux GPIO subsystem.

See `ledd.service` for an example `systemd` unit file that starts the daemon whenever the system boots.

## License

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
