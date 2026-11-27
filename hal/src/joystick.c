#include "joystick.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

static bool is_initialized = false;
static int spi_fd = -1;

// MCP3208 channel read helper function using SPI ioctl
static int read_adc_channel(int ch)
{
    if (spi_fd < 0) return 2048; // Fallback midpoint if not open

    // MCP3208 3-byte command format for single-ended mode
    uint8_t tx[3] = {
        (uint8_t)(0x06 | ((ch & 0x04) >> 2)),
        (uint8_t)((ch & 0x03) << 6),
        0x00
    };
    uint8_t rx[3] = {0};

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = 250000,
        .bits_per_word = 8,
        .cs_change = 0
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        return 2048; // Fallback on error
    }

    // Combine 12-bit result from rx bytes
    return ((rx[1] & 0x0F) << 8) | rx[2];
}

void joystick_init(void)
{
    printf("Joystick & SPI ADC - Initializing\n");
    assert(!is_initialized);
    is_initialized = true;

    // Open SPI device
    const char *dev = "/dev/spidev0.0";
    spi_fd = open(dev, O_RDWR);
    if (spi_fd < 0) {
        perror("Joystick SPI Open Error (Check if running with sudo)");
    } else {
        uint8_t mode = 0;
        uint8_t bits = 8;
        uint32_t speed = 250000;
        ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
        ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
        ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    }
}

enum JoyDirection joystick_get_direction(void)
{
    assert(is_initialized);

    // Read X from Channel 0, Y from Channel 1
    int x_val = read_adc_channel(0);
    int y_val = read_adc_channel(1);

    // printf("DEBUG - X_raw (CH0): %d | Y_raw (CH1): %d\n", x_val, y_val);

    // 12-bit ADC range: 0 to 4095 (midpoint ~2048)
    // Adjusted thresholds to prevent false triggers around the resting center
    const int HIGH_THRESHOLD = 3000;
    const int LOW_THRESHOLD = 1000;

    if (y_val > HIGH_THRESHOLD) {
        return JOY_UP;
    } else if (y_val < LOW_THRESHOLD) {
        return JOY_DOWN;
    } else if (x_val > HIGH_THRESHOLD) {
        return JOY_RIGHT;
    } else if (x_val < LOW_THRESHOLD) {
        return JOY_LEFT;
    }

    return JOY_NONE;
}

void joystick_cleanup(void)
{
    printf("Joystick - Cleanup\n");
    assert(is_initialized);

    if (spi_fd >= 0) {
        close(spi_fd);
        spi_fd = -1;
    }
    is_initialized = false;
}