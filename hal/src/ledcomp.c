#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <gpiod.h>
#include "ledcomp.h"
#include "rotary.h"

#define GPIO_CHIP_PATH "/dev/gpiochip3"
#define LED_PIN 11  // Line 11 ("GPIO18")

static pthread_t ledCompThreadID;
static bool isRunning = false;

static void* ledCompThreadFunc(void* arg) {
    (void)arg;

    struct gpiod_chip *chip = gpiod_chip_open(GPIO_CHIP_PATH);
    if (!chip) {
        perror("LedComp: Open chip failed");
        return NULL;
    }

    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    if (!settings) {
        perror("LedComp: Failed to create line settings");
        gpiod_chip_close(chip);
        return NULL;
    }
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offset = LED_PIN;
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "ledcomp-pin");

    struct gpiod_line_request *request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);

    if (!request) {
        char errBuf[256];
        snprintf(errBuf, sizeof(errBuf), "LedComp: Request lines failed (errno %d)", errno);
        perror(errBuf);
        gpiod_chip_close(chip);
        return NULL;
    }

    int lastFrequency = -1;
    bool ledState = false;

    while (isRunning) {
        int currentFreq = Rotary_getFrequency();

        if (currentFreq <= 0) {
            gpiod_line_request_set_value(request, LED_PIN, GPIOD_LINE_VALUE_INACTIVE);
            usleep(10000);
            lastFrequency = currentFreq;
            continue;
        }

        if (currentFreq != lastFrequency) {
            lastFrequency = currentFreq;
        }

        useconds_t halfPeriodUs = 1000000 / (2 * currentFreq);

        ledState = !ledState;
        gpiod_line_request_set_value(request, LED_PIN, ledState ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
        usleep(halfPeriodUs);
    }

    gpiod_line_request_set_value(request, LED_PIN, GPIOD_LINE_VALUE_INACTIVE);
    gpiod_line_request_release(request);
    gpiod_chip_close(chip);
    return NULL;
}

void LedComp_init(void) {
    isRunning = true;
    pthread_create(&ledCompThreadID, NULL, &ledCompThreadFunc, NULL);
}

void LedComp_cleanup(void) {
    isRunning = false;
    pthread_join(ledCompThreadID, NULL);
}