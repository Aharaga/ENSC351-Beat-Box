#include "oled.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define OLED_ADDRESS 0x3C
#define I2C_BUS "/dev/i2c-1"

static int oled_fd = -1;

// Send a command to the OLED
static void oled_command(unsigned char command)
{
    unsigned char data[2] = {0x00, command};

    if (write(oled_fd, data, 2) != 2) {
        perror("OLED I2C write failed");
    }
}

// Initialize the OLED
int oled_init(void)
{
    oled_fd = open(I2C_BUS, O_RDWR);

    if (oled_fd < 0) {
        perror("Failed to open I2C bus");
        return -1;
    }

    if (ioctl(oled_fd, I2C_SLAVE, OLED_ADDRESS) < 0) {
        perror("Failed to connect to OLED");
        close(oled_fd);
        oled_fd = -1;
        return -1;
    }

    // Basic SSD1306 initialization
    oled_command(0xAE); // Display off
    oled_command(0xD5); // Set display clock
    oled_command(0x80);
    oled_command(0xD9); // Set pre-charge period
    oled_command(0xF1);
    oled_command(0xDB); // Set VCOMH level
    oled_command(0x40);
    oled_command(0xA8); // Set multiplex ratio
    oled_command(0x3F); // 64 rows
    oled_command(0xD3); // Display offset
    oled_command(0x00);
    oled_command(0x40); // Start line
    oled_command(0xA1); // Segment remap
    oled_command(0xC8); // COM scan direction
    oled_command(0xDA); // COM pins
    oled_command(0x12);
    oled_command(0x81); // Contrast
    oled_command(0x7F);
    oled_command(0xA4); // Resume RAM display
    oled_command(0xA6); // Normal display
    oled_command(0x8D); // Charge pump configuration
    oled_command(0x14); // Enable internal charge pump
    oled_command(0xAF); // Display on
    oled_command(0xA4); // Display pixels from memory
    printf("OLED initialized successfully!\n");
    return 0;
}

// Clear the display
void oled_clear(void)
{
    for (int page = 0; page < 8; page++) {
        oled_command(0xB0 + page);
        oled_command(0x00);
        oled_command(0x10);

        for (int col = 0; col < 128; col++) {
            unsigned char data[2] = {0x40, 0x00};
            write(oled_fd, data, 2);
        }
    }
}

// Close I2C connection
void oled_close(void)
{
    if (oled_fd >= 0) {
        close(oled_fd);
        oled_fd = -1;
    }
}
// Display text using a simple 5x7 pixel font

// 5x7 font for uppercase letters, numbers, and symbols
void oled_print_at(int row, const char *text)
{
    if (row < 0 || row > 7 || text == NULL) {
        return;
    }
    static const unsigned char letters[26][5] = {
        {0x7E,0x11,0x11,0x11,0x7E}, // A
        {0x7F,0x49,0x49,0x49,0x36}, // B
        {0x3E,0x41,0x41,0x41,0x22}, // C
        {0x7F,0x41,0x41,0x22,0x1C}, // D
        {0x7F,0x49,0x49,0x49,0x41}, // E
        {0x7F,0x09,0x09,0x09,0x01}, // F
        {0x3E,0x41,0x49,0x49,0x7A}, // G
        {0x7F,0x08,0x08,0x08,0x7F}, // H
        {0x00,0x41,0x7F,0x41,0x00}, // I
        {0x20,0x40,0x41,0x3F,0x01}, // J
        {0x7F,0x08,0x14,0x22,0x41}, // K
        {0x7F,0x40,0x40,0x40,0x40}, // L
        {0x7F,0x02,0x0C,0x02,0x7F}, // M
        {0x7F,0x04,0x08,0x10,0x7F}, // N
        {0x3E,0x41,0x41,0x41,0x3E}, // O
        {0x7F,0x09,0x09,0x09,0x06}, // P
        {0x3E,0x41,0x51,0x21,0x5E}, // Q
        {0x7F,0x09,0x19,0x29,0x46}, // R
        {0x46,0x49,0x49,0x49,0x31}, // S
        {0x01,0x01,0x7F,0x01,0x01}, // T
        {0x3F,0x40,0x40,0x40,0x3F}, // U
        {0x1F,0x20,0x40,0x20,0x1F}, // V
        {0x7F,0x20,0x18,0x20,0x7F}, // W
        {0x63,0x14,0x08,0x14,0x63}, // X
        {0x03,0x04,0x78,0x04,0x03}, // Y
        {0x61,0x51,0x49,0x45,0x43}  // Z
    };

 
static const unsigned char numbers[10][5] = {
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x01,0x71,0x09,0x07}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}  // 9
};


    // Start at the selected row, column 0
    oled_command(0xB0 + row);
    oled_command(0x00);
    oled_command(0x10);

    while (*text) {
        const unsigned char *glyph = NULL;
        unsigned char symbol[5] = {0};

        if (*text >= 'A' && *text <= 'Z') {
            glyph = letters[*text - 'A'];
        }
        else if (*text >= '0' && *text <= '9') {
            glyph = numbers[*text - '0'];
        }
        else {
            switch (*text) {
                case ':':
                    symbol[2] = 0x36;
                    break;
                case '-':
                    symbol[0] = 0x08;
                    symbol[1] = 0x08;
                    symbol[2] = 0x08;
                    symbol[3] = 0x08;
                    symbol[4] = 0x08;
                    break;
                case '%':
                    symbol[0] = 0x63;
                    symbol[1] = 0x13;
                    symbol[2] = 0x08;
                    symbol[3] = 0x64;
                    symbol[4] = 0x63;
                    break;
            }
            glyph = symbol;
        }

        // Write 5 font columns + 1 blank spacing column
        for (int i = 0; i < 6; i++) {
            unsigned char pixel = (i < 5) ? glyph[i] : 0x00;
            unsigned char data[2] = {0x40, pixel};

            if (write(oled_fd, data, 2) != 2) {
                perror("OLED text write failed");
                return;
            }
        }

        text++;
    }
}

// Display text on the first row
void oled_print(const char *text)
{
    oled_print_at(0, text);
}