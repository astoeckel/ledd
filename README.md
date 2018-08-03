# LED Blinking Daemon

Blinks LEDs on Linux systems based whenever the content of a "monitor" file changes.

### Compile and install

Simply run `make` in the root directory of this repository.

```
git clone https://github.com/astoeckel/ledd
cd ledd
make
```

To install, copy to a directory that is in the path, e.g. `/usr/local/bin/`.

### How to use

Run the following command to blink an LED whenever there is network activity on the specified network interface.

```
ledd --led \
	'/sys/devices/platform/leds_apu/leds/apu2:green:1/brightness' \ # LED file
	'/sys/class/net/enp3s0/statistics/rx_bytes' \ # Blink whenever the number of received bytes changes
	'/sys/class/net/enp3s0/statistics/tx_bytes'   # Blink whenever the number of transmitted bytes changes
```

You can list up to 16 monitor files per LED and define up to 16 LEDs.
