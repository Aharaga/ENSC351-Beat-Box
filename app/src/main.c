
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <signal.h>

#include "audio.h"
#include "display.h"
#include "button.h"

#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_SPEED 1000000
#define ADC_MAX 4095.0f

static volatile sig_atomic_t running = 1;

// Stop the program when Ctrl+C is pressed
static void stopProgram(int sig)
{
    (void)sig;
    running = 0;
}

// Read a channel from the MCP3208
static int readADC(int fd, int channel)
{
    uint8_t tx[3] = {
        0x06 | ((channel & 0x04) >> 2),
        (channel & 0x03) << 6,
        0x00
    };

    uint8_t rx[3] = {0};

    struct spi_ioc_transfer transfer = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = SPI_SPEED,
        .bits_per_word = 8
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &transfer) < 0) {
        perror("SPI transfer failed");
        return -1;
    }

    return ((rx[1] & 0x0F) << 8) | rx[2];
}

int main(void)
{
    signal(SIGINT, stopProgram);

    printf("Starting DJ Mixer...\n");

    // Open SPI connection to the ADC
    int fd = open(SPI_DEVICE, O_RDWR);

    if (fd < 0) {
        perror("Failed to open SPI");
        return 1;
    }

    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = SPI_SPEED;

    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0 ||
        ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
        ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("SPI setup failed");
        close(fd);
        return 1;
    }

    // Initialize the four buttons
    if (!button_init()) {
        printf("Button initialization failed\n");
        close(fd);
        return 1;
    }

    // Start the OLED
    display_init();

    // Start Amanda's audio engine
    if (!Audio_init("tracks/trackA.wav", "tracks/trackB.wav")) {
        button_cleanup();
        display_close();
        close(fd);
        return 1;
    }

    int displayCounter = 0;
    // Both tracks start playing
    bool playingA = true;
    bool playingB = true;

    while (running) {

        // Check for new button presses
        if (button_was_pressed(BUTTON_A_PLAY)) {
            playingA = !playingA;
            printf("Track A: %s\n", playingA ? "Playing" : "Paused");
        }

        if (button_was_pressed(BUTTON_A_NEXT)) {
            printf("Button A: Next Track pressed\n");
        }

        if (button_was_pressed(BUTTON_B_PLAY)) {
            playingB = !playingB;
            printf("Track B: %s\n", playingB ? "Playing" : "Paused");
        }

        if (button_was_pressed(BUTTON_B_NEXT)) {
            printf("Button B: Next Track pressed\n");
        }

        // Read all three physical faders
        int adcA = readADC(fd, 0);
        int adcB = readADC(fd, 1);
        int adcCross = readADC(fd, 2);

        if (adcA < 0 || adcB < 0 || adcCross < 0) {
            break;
        }

        // Convert ADC readings to values between 0 and 1
        float volumeA = adcA / ADC_MAX;
        float volumeB = adcB / ADC_MAX;
        float crossfade = adcCross / ADC_MAX;

        // Update audio using the physical faders
        Audio_setVolumeA(volumeA);
        Audio_setVolumeB(volumeB);
        Audio_setCrossfade(crossfade);

        // Convert volumes to percentages for the OLED
        int percentA = (int)(volumeA * 100);
        int percentB = (int)(volumeB * 100);

        // Update the OLED less often than the buttons
        if (displayCounter == 0) {
            display_update("TRACK A", "TRACK B",
                percentA, percentB, playingA, playingB,
                (int)(crossfade * 100));
        }

        displayCounter = (displayCounter + 1) % 10;

        usleep(5000);
    }

    // Clean up everything
    Audio_cleanup();
    button_cleanup();
    display_close();
    close(fd);

    printf("DJ Mixer stopped.\n");

    return 0;
}
