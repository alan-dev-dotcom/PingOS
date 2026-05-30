/* drivers/keyboard.c - PS/2 Keyboard driver for PingOS */
#include "io.h"
#include "drivers.h"

// Scancode mapping state variables
static int shift_pressed = 0;
static int caps_lock = 0;

/* Initialize keyboard registers */
void keyboard_init() {
// Empty keyboard buffer by reading dummy data
while (inb(0x64) & 1) {
inb(0x60);
}
shift_pressed = 0;
caps_lock = 0;
}

/* Wait for key and process layout mapping */
char keyboard_get_char() {
// Check if there is actually data waiting in the keyboard buffer
// Doing a non-blocking check first prevents the system from getting stuck if called rapidly
if ((inb(0x64) & 1) == 0) {
return 0;
}

unsigned char scancode = inb(0x60);

// Handle key releases (scancodes with high bit set)
if (scancode & 0x80) {
    unsigned char released = scancode & 0x7F;
    if (released == 0x2A || released == 0x36) { // Shift key released
        shift_pressed = 0;
    }
    return 0; // Don't return character on key release
}

// Handle key presses for modifiers and control characters
switch (scancode) {
    case 0x2A: // Left Shift pressed
    case 0x36: // Right Shift pressed
        shift_pressed = 1;
        return 0;
    case 0x3A: // Caps Lock pressed
        caps_lock = !caps_lock;
        return 0;
    case 0x0E: // Backspace pressed
        return '\b';
    case 0x1C: // Enter
        return '\n';
    case 0x39: // Spacebar
        return ' ';
    default: break;
}

// Standard character map (Lowercase/Uppercase)
const char lowercase[] = {
    0,  0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

const char uppercase[] = {
    0,  0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
    0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0, 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

// Verify scancode is within our translation tables
if (scancode < sizeof(lowercase)) {
    char ch = 0;
    
    // Select lowercase or uppercase based on shift/caps lock rules
    if (lowercase[scancode] >= 'a' && lowercase[scancode] <= 'z') {
        if (shift_pressed ^ caps_lock) {
            ch = uppercase[scancode];
        } else {
            ch = lowercase[scancode];
        }
    } else {
        ch = shift_pressed ? uppercase[scancode] : lowercase[scancode];
    }

    // Only return the character if it is a valid, printable character!
    // This stops null-characters (0) from slipping into our command buffer.
    if (ch != 0) {
        return ch;
    }
}

return 0;


}