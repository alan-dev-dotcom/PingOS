/* =======================================================
 * PingOS Kernel (PingKernel) - 32-bit Freestanding Kernel
 * =======================================================
 */
#include "drivers.h"

// Monochrome Color Schemes (VGA Text Mode Attributes)
#define COLOR_WHITE_ON_BLACK 0x0F  // Bright white text on black background
#define COLOR_GREY_ON_BLACK  0x07  // Standard light grey text on black background

/* String comparison helper function */
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/* Helper: Wait for a keypress using the non-blocking keyboard driver */
char wait_for_key() {
    char key = 0;
    while (1) {
        key = keyboard_get_char();
        if (key != 0) {
            return key;
        }
    }
}

/* PingOS Simple Interactive Shell */
void run_shell() {
    char input_buffer[64];
    int input_index = 0;

    vga_print_string("pingos> ", COLOR_WHITE_ON_BLACK);

    while (1) {
        char key = wait_for_key();

        if (key == '\n') {
            vga_put_char('\n', COLOR_GREY_ON_BLACK);
            input_buffer[input_index] = '\0'; // Null-terminate user input

            if (strcmp(input_buffer, "help") == 0) {
                vga_print_string("Commands: help, info, ping, clear, uname, uname -r, devices\n", COLOR_GREY_ON_BLACK);
            } else if (strcmp(input_buffer, "info") == 0) {
                vga_print_string("PingOS: Written from scratch. Version 0.1 (x86-32)\n", COLOR_GREY_ON_BLACK);
            } else if (strcmp(input_buffer, "ping") == 0) {
                vga_print_string("PONG! Kernel is responsive and processing hardware loops!\n", COLOR_GREY_ON_BLACK);
            } else if (strcmp(input_buffer, "clear") == 0) {
                vga_clear_screen();
            } else if (strcmp(input_buffer, "uname") == 0) {
                vga_print_string("PingOS\n", COLOR_GREY_ON_BLACK);
            } else if (strcmp(input_buffer, "uname -r") == 0) {
                vga_print_string("0.1.0-testing\n", COLOR_GREY_ON_BLACK);
            } else if (strcmp(input_buffer, "devices") == 0) {
                vga_print_string("Probing system devices...\n", COLOR_GREY_ON_BLACK);
                usb_init();
                intel_gpu_init();
            } else if (input_index > 0) {
                vga_print_string("Error: Unknown command. Type 'help' for options.\n", COLOR_WHITE_ON_BLACK);
            }

            // Reset shell prompt
            input_index = 0;
            vga_print_string("pingos> ", COLOR_WHITE_ON_BLACK);
        } else if (key == '\b') {
            // Support Backspace deletion in input buffer
            if (input_index > 0) {
                input_index--;
                vga_put_char('\b', COLOR_GREY_ON_BLACK);
            }
        } else {
            // Write normal key into shell buffer if it fits
            if (input_index < 63) {
                input_buffer[input_index++] = key;
                vga_put_char(key, COLOR_GREY_ON_BLACK);
            }
        }
    }
}

/* Kernel Entry Point */
void main() {
    // 1. Clear VGA display memory
    vga_clear_screen();

    // 2. Initialize Hardware Keyboards
    keyboard_init();
    mouse_init();

    // 3. Draw Updated Splash Screen (Monochrome layout)
    vga_print_string("================================================================================\n", COLOR_GREY_ON_BLACK);
    vga_print_string("PingOS\n", COLOR_WHITE_ON_BLACK);
    vga_print_string("OS made mostly for testing as a hobby project. Have fun!\n", COLOR_GREY_ON_BLACK);
    vga_print_string("If you want to contribute, then go to https://github.com/alan-dev-dotcom/PingOS/\n", COLOR_GREY_ON_BLACK);
    vga_print_string("For a list of commands, type \"help\"\n", COLOR_GREY_ON_BLACK);
    vga_print_string("To check the kernel version, type \"uname -r\"\n", COLOR_GREY_ON_BLACK);
    vga_print_string("To check OS version, type \"uname\"\n", COLOR_GREY_ON_BLACK);
    vga_print_string("=================================================================================\n\n", COLOR_GREY_ON_BLACK);

    // 4. Fire up the shell
    run_shell();
}