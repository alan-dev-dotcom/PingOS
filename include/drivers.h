/* include/drivers.h - Driver function declarations for PingOS */
#ifndef DRIVERS_H
#define DRIVERS_H

// --- VGA Driver ---
void vga_clear_screen();
void vga_print_string(const char* str, char color);
void vga_put_char(char c, char color);
void vga_update_cursor(int row, int col);

// --- Keyboard Driver ---
void keyboard_init();
char keyboard_get_char();

// --- Mouse Driver ---
void mouse_init();
void mouse_get_status(int* x_out, int* y_out, int* left_click, int* right_click);

// --- Intel Graphics Driver ---
void intel_gpu_init();

// --- USB Driver ---
void usb_init();

#endif