/* drivers/mouse.c - PS/2 Mouse driver for PingOS */
#include "io.h"
#include "drivers.h"

// PS2 Auxiliary mouse registers
#define MOUSE_PORT_CMD  0x64
#define MOUSE_PORT_DATA 0x60

static int mouse_x_offset = 0;
static int mouse_y_offset = 0;
static int button_left = 0;
static int button_right = 0;

/* Helper functions to wait for ready states in PS2 controller */
static void mouse_wait_write() {
    while ((inb(MOUSE_PORT_CMD) & 2) != 0);
}

static void mouse_wait_read() {
    while ((inb(MOUSE_PORT_CMD) & 1) == 0);
}

/* Write command byte to the PS2 controller */
static void mouse_write(unsigned char data) {
    mouse_wait_write();
    outb(MOUSE_PORT_CMD, 0xD4); // Signal we are writing to secondary auxiliary device
    mouse_wait_write();
    outb(MOUSE_PORT_DATA, data);
}

/* Read response from controller */
static unsigned char mouse_read() {
    mouse_wait_read();
    return inb(MOUSE_PORT_DATA);
}

/* Initialize PS/2 Mouse device */
void mouse_init() {
    unsigned char status;

    // 1. Enable auxiliary mouse connector port
    mouse_wait_write();
    outb(MOUSE_PORT_CMD, 0xA8);

    // 2. Read PS2 Controller configuration byte
    mouse_wait_write();
    outb(MOUSE_PORT_CMD, 0x20);
    mouse_wait_read();
    status = inb(MOUSE_PORT_DATA) | 2; // Enable interrupts for secondary device (mouse)

    // 3. Write modified controller configuration byte back
    mouse_wait_write();
    outb(MOUSE_PORT_CMD, 0x60);
    mouse_wait_write();
    outb(MOUSE_PORT_DATA, status);

    // 4. Reset mouse to default state
    mouse_write(0xF6);
    mouse_read(); // Acknowledge byte (0xFA)

    // 5. Enable data streaming
    mouse_write(0xF4);
    mouse_read(); // Acknowledge byte (0xFA)
}

/* Poll mouse activity packets */
void mouse_get_status(int* x_out, int* y_out, int* left_click, int* right_click) {
    // Check if mouse controller has data in output buffer (Bit 0 set, Aux Bit 5 set)
    if ((inb(MOUSE_PORT_CMD) & 0x21) == 0x21) {
        unsigned char byte1 = inb(MOUSE_PORT_DATA);
        mouse_wait_read();
        unsigned char byte2 = inb(MOUSE_PORT_DATA);
        mouse_wait_read();
        unsigned char byte3 = inb(MOUSE_PORT_DATA);

        // Decode packet details
        button_left = byte1 & 1;
        button_right = (byte1 >> 1) & 1;

        // Process Sign extensions for relative movements
        int x_sign = (byte1 >> 4) & 1;
        int y_sign = (byte1 >> 5) & 1;

        mouse_x_offset = (x_sign) ? (int)(byte2 - 256) : (int)byte2;
        mouse_y_offset = (y_sign) ? (int)(byte3 - 256) : (int)byte3;
    } else {
        mouse_x_offset = 0;
        mouse_y_offset = 0;
    }

    *x_out = mouse_x_offset;
    *y_out = mouse_y_offset;
    *left_click = button_left;
    *right_click = button_right;
}