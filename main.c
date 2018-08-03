/*
 *  LEDD -- LED blinking daemon.
 *  Copyright (C) 2018  Andreas Stöckel
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _DEFAULT_SOURCE

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_N_LEDS 16
#define MAX_N_MONITORS 16
#define BLINK_INTERVAL 200 /* In ms */

/**
 * The led_t structures captures the status of a single LED as well as all
 * monitors attached to it.
 */
typedef struct {
	/* File used for toggling the LED on or off */
	char const *led_file;

	/* Set to true if the LED file is OK to only print a warning once. */
	bool led_file_ok;

	/* Pointers at the strings corresponding to the monitor files */
	char const *monitor_files[MAX_N_MONITORS];

	/* Last integer content of each of the monitored files */
	uint64_t monitor_hash[MAX_N_MONITORS];

	/* Set to false if the corresponding monitor file could not be opened the
	   last time. This is used to only print a warning once. */
	bool monitor_files_ok[MAX_N_MONITORS];

	/* Time of the last LED status change expressed in main loop iterations */
	uint64_t last_active_change;

	/* Number of monitor files attached to the LED */
	int n_monitors;

	/* Flag indicating whether the LED ought to be active or note */
	bool active;
} led_t;

/**
 * Writes one or zero to the given file based on the status.
 */
static void file_write_status(const char *file, bool *file_ok, bool status)
{
	/* Open the file */
	int fd = open(file, O_WRONLY | O_TRUNC);
	if (fd < 0) {
		if (*file_ok) {
			fprintf(stderr, "Warning: cannot open led file %s!\n", file);
		}
		*file_ok = false;
	}

	/* Write one or zero */
	const char s_one[] = "1\n";
	const char s_zero[] = "0\n";
	const char *buf = status ? s_one : s_zero;
	ssize_t buf_len = status ? sizeof(s_one) : sizeof(s_zero);
	if (write(fd, buf, buf_len) != buf_len) {
		if (*file_ok) {
			fprintf(stderr, "Warning: error while writing to led file %s\n",
			        file);
		}
		*file_ok = false;
	}

	/* Close the file */
	close(fd);
	*file_ok = true;
}

/**
 * Computes a hash of the contents of the given file.
 */
static uint64_t file_compute_hash(const char *file, bool *file_ok)
{
	/* Open the file */
	int fd = open(file, O_RDONLY);
	if (fd < 0) {
		if (*file_ok) {
			fprintf(stderr, "Warning: cannot open monitor file %s!\n", file);
		}
		*file_ok = false;
		return 0U;
	}
	*file_ok = true;

	/* Compute the hash */
	uint8_t buf[4096];
	ssize_t n_bytes;
	uint64_t hash = 0U;
	while ((n_bytes = read(fd, buf, 4096)) > 0) {
		for (int i = 0; i < n_bytes; i++) {
			hash += buf[i];
			hash += hash << 10;
			hash ^= hash >> 6;
		}
	}
	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;

	/* Close the file */
	close(fd);

	return hash;
}

/**
 * Resets the LED specification.
 */
static void led_reset(led_t *led)
{
	led->led_file = NULL;
	led->led_file_ok = true;
	for (int i = 0; i < MAX_N_MONITORS; i++) {
		led->monitor_files[i] = NULL;
		led->monitor_hash[i] = 0U;
		led->monitor_files_ok[i] = true;
	}
	led->n_monitors = 0;
	led->active = true; /* Force toggle to zero */
	led->last_active_change = 0U;
}

/**
 * Parses the command line flags for a single LED specification.
 */
static bool led_parse(char const ***arg, char const **arg_end, led_t *led)
{
	led_reset(led); /* Reset the LED status change */
	while (*arg < arg_end) {
		if (strncmp("--", **arg, 2) == 0) {
			break;
		}
		if (!led->led_file) {
			led->led_file = **arg;
		}
		else {
			if (led->n_monitors >= MAX_N_MONITORS) {
				fprintf(stderr, "Exceeded maximum number of monitors.\n");
				return false;
			}
			led->monitor_files[led->n_monitors++] = **arg;
		}
		(*arg)++;
	}
	return led->led_file && led->n_monitors;
}

/**
 * Computes the hash of each monitor files. If the hash of at least one monitor
 * file changes, blinks the led.
 */
static void led_update(led_t *led, uint64_t it)
{
	/* Update the file hashes for all monitored files. */
	bool new_active = false;
	for (int i = 0; i < led->n_monitors; i++) {
		uint64_t new_hash =
		    file_compute_hash(led->monitor_files[i], &led->monitor_files_ok[i]);
		new_active = new_active || (new_hash != led->monitor_hash[i]);
		led->monitor_hash[i] = new_hash;
	}

	/* If the LED status changed, write the corresponding status to the LED
	   file */
	if (new_active != led->active) {
		file_write_status(led->led_file, &led->led_file_ok, new_active);
		led->active = new_active;
		led->last_active_change = it;
		return;
	}

	/* Otherwise, if the LED is active, let it blink! */
	if (led->active) {
		file_write_status(led->led_file, &led->led_file_ok,
		                  (it - led->last_active_change + 1) & 1);
	}
}

/**
 * Prints the "usage" command line.
 */
static void print_usage(const char *argv[])
{
	fprintf(stderr, "Usage: %s --led <LED FILE> <MONITOR 1> ... <MONITOR n>\n",
	        argv[0]);
}

int main(int argc, const char *argv[])
{
	int n_leds = 0;
	led_t leds[MAX_N_LEDS];

	/* Parse the command line arguments */
	const char **arg_begin = argv + 1;
	const char **arg_end = argv + argc;
	char const **arg = arg_begin;
	while (arg < arg_end) {
		const char *cur_arg =
		    *(arg++); /* Remember current arg and advance arg */
		if (arg == arg_begin &&
		    (strcmp("-h", cur_arg) || strcmp("--help", cur_arg) == 0)) {
			print_usage(argv);
			return 1;
		}
		else if (strcmp("--led", cur_arg) == 0) {
			if (n_leds >= MAX_N_LEDS) {
				fprintf(stderr, "Exceeded maximum number of LEDs.\n");
				return 1;
			}
			if (!led_parse(&arg, arg_end, &leds[n_leds++])) {
				fprintf(stderr, "Error parsing LED specification.\n");
				return 1;
			}
		}
		else {
			fprintf(stderr, "Unexpected command line argument \"%s\".\n",
			        cur_arg);
			return 1;
		}
	}

	/* Abort if there were no LEDs */
	if (n_leds == 0) {
		fprintf(stderr, "Must provide at least one LED specification.\n");
		print_usage(argv);
		return 1;
	}

	/* Dump the LED specification */
	for (int i = 0; i < n_leds; i++) {
		fprintf(stderr, "- LED %d:\n", i);
		for (int j = 0; j < leds[i].n_monitors; j++) {
			fprintf(stderr, "    - Monitor %d: \"%s\"\n", j,
			        leds[i].monitor_files[j]);
		}
	}

	/* Main loop -- read content of the monitor files and toggle the LED state
	   accordingly. */
	for (uint64_t it = 1; true; it++) {
		for (int i = 0; i < n_leds; i++) {
			led_update(&leds[i], it); /* Update each LED */
		}
		usleep(BLINK_INTERVAL * 1000);
	}
}
