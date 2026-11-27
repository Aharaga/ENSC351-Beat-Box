#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <gpiod.h>
#include "rotary.h"

#define GPIO_CHIP_PATH "/dev/gpiochip3"
#define PIN_A 7   // Line 7 ("GPIO16")
#define PIN_B 8   // Line 8 ("GPIO17")

static pthread_t rotaryThreadID;
static bool isRunning = false;
static int currentFrequency = 10; 
static pthread_mutex_t freqMutex = PTHREAD_MUTEX_INITIALIZER;

int Rotary_getFrequency(void) {
    pthread_mutex_lock(&freqMutex);
    int freq = currentFrequency;
    pthread_mutex_unlock(&freqMutex);
    return freq;
}

static void* rotaryThreadFunc(void* arg) {
    (void)arg;
    
    setbuf(stdout, NULL);

    struct gpiod_chip *chip = gpiod_chip_open(GPIO_CHIP_PATH);
    if (!chip) {
        perror("Rotary ERROR: Open chip failed");
        return NULL;
    }
    printf("DEBUG: Opened chip %s successfully.\n", GPIO_CHIP_PATH);

    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    if (!settings) {
        perror("Rotary ERROR: Failed to create line settings");
        gpiod_chip_close(chip);
        return NULL;
    }
    
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offsets[2] = {PIN_A, PIN_B};
    gpiod_line_config_add_line_settings(line_cfg, offsets, 2, settings);

    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "rotary-encoder");

    struct gpiod_line_request *request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);

    if (!request) {
        char errBuf[256];
        snprintf(errBuf, sizeof(errBuf), "Rotary ERROR: Request lines failed (errno %d)", errno);
        perror(errBuf);
        gpiod_chip_close(chip);
        return NULL;
    }

    int lastValA = gpiod_line_request_get_value(request, PIN_A);

    while (isRunning) {
        int valA = gpiod_line_request_get_value(request, PIN_A);
        int valB = gpiod_line_request_get_value(request, PIN_B);

        if (valA != lastValA) {
            
            if (valA == 1) { 
                pthread_mutex_lock(&freqMutex);
                if (valB == 0) {
                    currentFrequency++;
                    if (currentFrequency > 500) {
                        currentFrequency = 500;
                    }
                } else {
                    currentFrequency--;
                    if (currentFrequency < 0) {
                        currentFrequency = 0;
                    }
                }
                pthread_mutex_unlock(&freqMutex);
            }
            lastValA = valA;
        }
        usleep(1000); 
    }

    gpiod_line_request_release(request);
    gpiod_chip_close(chip);
    return NULL;
}

void Rotary_init(void) {
    isRunning = true;
    pthread_create(&rotaryThreadID, NULL, &rotaryThreadFunc, NULL);
}

void Rotary_cleanup(void) {
    isRunning = false;
    pthread_join(rotaryThreadID, NULL);
}