/* See LICENSE file for copyright and license details. */
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../util.h"

const char *
battery_perc(const char *bat)
{
	int perc;
	char path[PATH_MAX];

	snprintf(path, sizeof(path), "%s%s%s", "/sys/class/power_supply/", bat, "/capacity");
	return (pscanf(path, "%i", &perc) == 1) ?
	       bprintf("%d", perc) : NULL;
}

const char *
battery_power(const char *bat)
{
	int watts;
	char path[PATH_MAX];

	snprintf(path, sizeof(path), "%s%s%s", "/sys/class/power_supply/", bat, "/power_now");
	return (pscanf(path, "%i", &watts) == 1) ?
	       bprintf("%d", (watts + 500000) / 1000000) : NULL;
}

const char *
battery_state(const char *bat)
{
	struct {
		char *state;
		char *symbol;
	} map[] = {
		{ "Charging",    "+" },
		{ "Discharging", "-" },
		{ "Full",        "=" },
		{ "Unknown",     "/" },
	};
	size_t i;
	char path[PATH_MAX], state[12];

	snprintf(path, sizeof(path), "%s%s%s", "/sys/class/power_supply/", bat, "/status");
	if (pscanf(path, "%12s", state) != 1) {
		return NULL;
	}

	for (i = 0; i < LEN(map); i++) {
		if (!strcmp(map[i].state, state)) {
			break;
		}
	}
	return (i == LEN(map)) ? "?" : map[i].symbol;
}

const char *
battery_rem(const char *bat)
{
    static float rems[FILTER_LEN];
    static int curr;
    static const char *last_status = NULL;
    static float last_rem;

    char path[PATH_MAX];
    const char *status = battery_state(bat);

    if (status == NULL) {
        return NULL;
    }

    if (!strcmp(status, "/")) {
        return " est ";
    }

    if (!strcmp(status, "=")) {
        return " pwr ";
    }

    if (last_status != NULL && strcmp(status, last_status)) {
        memset(rems, 0.0, sizeof(rems));
        curr = 0;
    }
    last_status = status;

    int charge_full, charge_now, current_now;
    float rem;

    snprintf(path, sizeof(path), "%s%s%s", "/sys/class/power_supply/", bat, "/charge_now");
    if (pscanf(path, "%d", &charge_now) != 1) {
        return NULL;
    }

    snprintf(path, sizeof(path), "%s%s%s", "/sys/class/power_supply/", bat, "/current_now");
    if (pscanf(path, "%d", &current_now) != 1) {
        return NULL;
    }

    if (!strcmp(status, "+")) {
        snprintf(path, sizeof(path), "%s%s%s", "/sys/class/power_supply/", bat, "/charge_full");
        if (pscanf(path, "%d", &charge_full) != 1) {
            return NULL;
        }

        rem = (float) (charge_full - charge_now) / current_now;
    } else {
        // Ah / A = h
        rem = (float) charge_now / current_now;
    }

    rems[curr++ % FILTER_LEN] = rem;

    if (curr >= FILTER_LEN) {
        if (curr % FILTER_LEN == 0) {
            last_rem = 0;
            for (int i=0; i < FILTER_LEN; i++) last_rem += rems[i];
            last_rem = last_rem / FILTER_LEN;
        }

        int hours = (int) last_rem;
        int mins = ((int)(last_rem * 60)) % 60;

        return bprintf("%02d:%02d", hours, mins);
    } else {
        // estimating
        return " ... ";
    }
}
