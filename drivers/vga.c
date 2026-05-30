/* drivers/vga.c - VGA video card driver for PingOS */
#include "io.h"
#include "drivers.h"

#define VIDEO_ADDRESS 0xB8000
#define SCREEN_ROWS 25
#define SCREEN_COLS 80
#define COLOR_WHITE_ON_BLACK 0x0F

static int cursor_row = 0;
static int cursor_col = 0;

/* Update the blinking hardware text cursor by programming the VGA controller */
void vga_update_cursor(int row, int col) {
    unsigned short position = (row * SCREEN_COLS) + col;

    // Send the high byte of the cursor offset to CRTC register index 14 (0x0F)
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position & 0xFF));
    // Send the low byte of the cursor offset to CRTC register index 15 (0x0E)
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((position >> 8) & 0xFF));
}

/* Clear the screen text memory */
void vga_clear_screen() {
    char* vidmem = (char*) VIDEO_ADDRESS;
    for (int i = 0; i < SCREEN_ROWS * SCREEN_COLS * 2; i += 2) {
        vidmem[i] = ' ';
        vidmem[i + 1] = COLOR_WHITE_ON_BLACK;
    }
    cursor_row = 0;
    cursor_col = 0;
    vga_update_cursor(cursor_row, cursor_col);
}

/* Put a single character with custom styling */
void vga_put_char(char c, char color) {
    char* vidmem = (char*) VIDEO_ADDRESS;

    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\r') {
        cursor_col = 0;
    } else if (c == '\b') {
        // Handle Backspace: Move cursor back and replace character with space
        if (cursor_col > 0) {
            cursor_col--;
        } else if (cursor_row > 0) {
            cursor_row--;
            cursor_col = SCREEN_COLS - 1;
        }
        int offset = (cursor_row * SCREEN_COLS + cursor_col) * 2;
        vidmem[offset] = ' '; // Clear character visually
        vidmem[offset + 1] = color;
    } else {
        int offset = (cursor_row * SCREEN_COLS + cursor_col) * 2;
        vidmem[offset] = c;
        vidmem[offset + 1] = color;
        cursor_col++;
    }

    if (cursor_col >= SCREEN_COLS) {
        cursor_col = 0;
        cursor_row++;
    }

    // Scroll if we hit bottom
    if (cursor_row >= SCREEN_ROWS) {
        for (int r = 1; r < SCREEN_ROWS; r++) {
            for (int col = 0; col < SCREEN_COLS; col++) {
                int src = (r * SCREEN_COLS + col) * 2;
                int dst = ((r - 1) * SCREEN_COLS + col) * 2;
                vidmem[dst] = vidmem[src];
                vidmem[dst + 1] = vidmem[src + 1];
            }
        }
        int last_row = ((SCREEN_ROWS - 1) * SCREEN_COLS) * 2;
        for (int i = 0; i < SCREEN_COLS * 2; i += 2) {
            vidmem[last_row + i] = ' ';
            vidmem[last_row + i + 1] = COLOR_WHITE_ON_BLACK;
        }
        cursor_row = SCREEN_ROWS - 1;
    }
    vga_update_cursor(cursor_row, cursor_col);
}

void vga_print_string(const char* str, char color) {
    for (int i = 0; str[i] != '\0'; i++) {
        vga_put_char(str[i], color);
    }
}