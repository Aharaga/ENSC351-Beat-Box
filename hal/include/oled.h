#ifndef OLED_H
#define OLED_H

// Initialize the OLED display
int oled_init(void);

// Clear the OLED display
void oled_clear(void);

// Close the OLED connection
void oled_close(void);

// Display text on the OLED
void oled_print(const char *text);

// Display text on a specific row (0-7)
void oled_print_at(int row, const char *text);

#endif