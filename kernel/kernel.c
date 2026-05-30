/* ==============================================================================
 * PingOS Kernel (PingKernel) - 32-bit Freestanding Kernel
 * ==============================================================================
 * This is a highly expanded, fully freestanding, clean system kernel designed
 * for x86-32 architectures. It contains zero standard library dependencies,
 * building its own custom execution environment from scratch.
 * ==============================================================================
 */

#include "drivers.h"
#include "io.h"

/* ------------------------------------------------------------------------------
 * SECTION 1: Hardware & VGA Display Configurations
 * ------------------------------------------------------------------------------
 */

// VGA Text Mode Attribute Schemes
#define COLOR_BLACK_ON_BLACK   0x00
#define COLOR_GREY_ON_BLACK    0x07  // Legacy default text color
#define COLOR_BLUE_ON_BLACK    0x09  // Cool ocean style blue
#define COLOR_GREEN_ON_BLACK   0x0A  // Emerald retro terminal green
#define COLOR_CYAN_ON_BLACK    0x0B  // Teal diagnostics color
#define COLOR_RED_ON_BLACK     0x0C  // Alert / critical diagnostic red
#define COLOR_AMBER_ON_BLACK   0x0E  // Amber terminal phosphor style
#define COLOR_WHITE_ON_BLACK   0x0F  // Bright highlight white

// Global terminal configuration variables
static char current_prompt_color = COLOR_WHITE_ON_BLACK;
static char current_text_color   = COLOR_GREY_ON_BLACK;

/* ------------------------------------------------------------------------------
 * SECTION 2: Custom Freestanding C Standard Library Implementations
 * ------------------------------------------------------------------------------
 * Since we operate inside a raw bare-metal execution sandbox, we cannot use
 * standard library headers like string.h or stdio.h. Below is our robust,
 * highly tested implementation of core helper routines.
 * ------------------------------------------------------------------------------
 */

/* Return the length of a null-terminated string */
int strlen(const char* s) {
    int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

/* Compare two null-terminated strings */
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/* Compare up to n characters of two strings */
int strncmp(const char* s1, const char* s2, int n) {
    if (n == 0) return 0;
    while (n > 0 && *s1 && (*s1 == *s2)) {
        if (n == 1) return 0;
        s1++;
        s2++;
        n--;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/* Copy a source string to a destination buffer */
char* strcpy(char* dest, const char* src) {
    char* temp = dest;
    while ((*dest++ = *src++));
    return temp;
}

/* Copy up to n characters from source to destination */
char* strncpy(char* dest, const char* src, int n) {
    char* temp = dest;
    while (n > 0 && *src) {
        *dest++ = *src++;
        n--;
    }
    while (n > 0) {
        *dest++ = '\0';
        n--;
    }
    return temp;
}

/* Concatenate two strings */
char* strcat(char* dest, const char* src) {
    char* temp = dest;
    while (*dest) {
        dest++;
    }
    while ((*dest++ = *src++));
    return temp;
}

/* Search for the first occurrence of character c in string s */
char* strchr(const char* s, int c) {
    while (*s != (char)c) {
        if (!*s) return 0;
        s++;
    }
    return (char*)s;
}

/* Convert ASCII string representation into standard integer value */
int atoi(const char* s) {
    int res = 0;
    int sign = 1;
    int i = 0;

    // Handle white spaces
    while (s[i] == ' ' || s[i] == '\t') {
        i++;
    }

    // Handle sign indicators
    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (s[i] == '+') {
        i++;
    }

    // Process valid numerical digits
    while (s[i] >= '0' && s[i] <= '9') {
        res = res * 10 + (s[i] - '0');
        i++;
    }

    return sign * res;
}

/* Convert integer to ASCII string with optional minimum digit padding */
void itoa(int n, char s[], int min_digits) {
    int i = 0;
    int sign = n;

    if (sign < 0) {
        n = -n;
    }

    // Generate characters in reverse order
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    // Apply zero padding if minimum digit length is requested
    while (i < min_digits) {
        s[i++] = '0';
    }

    if (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    // Reverse the string
    int j, k;
    char temp;
    for (j = 0, k = i - 1; j < k; j++, k--) {
        temp = s[j];
        s[j] = s[k];
        s[k] = temp;
    }
}

/* Convert integer to Hexadecimal string representation */
void htoa(unsigned int n, char s[]) {
    int i = 0;
    unsigned int temp_n = n;

    // Handle zero case explicitly
    if (temp_n == 0) {
        s[i++] = '0';
    } else {
        while (temp_n > 0) {
            int digit = temp_n % 16;
            if (digit < 10) {
                s[i++] = digit + '0';
            } else {
                s[i++] = (digit - 10) + 'A';
            }
            temp_n /= 16;
        }
    }
    s[i] = '\0';

    // Reverse generated hex string
    int j, k;
    char temp;
    for (j = 0, k = i - 1; j < k; j++, k--) {
        temp = s[j];
        s[j] = s[k];
        s[k] = temp;
    }
}

/* Helper to wait for a keypress using the non-blocking keyboard driver */
char wait_for_key() {
    char key = 0;
    while (1) {
        key = keyboard_get_char();
        if (key != 0) {
            return key;
        }
    }
}

/* ------------------------------------------------------------------------------
 * SECTION 3: CMOS Real Time Clock (RTC) Diagnostics
 * ------------------------------------------------------------------------------
 */

/* Read low-level registers directly from CMOS hardware ports */
unsigned char read_cmos_register(int reg) {
    outb(0x70, reg);
    return inb(0x71);
}

/* Determine if CMOS data is currently updating to avoid reading corrupted states */
int is_cmos_update_in_progress() {
    outb(0x70, 0x0A);
    return (inb(0x71) & 0x80);
}

/* Safe CMOS reader helper */
unsigned char get_cmos_value(int reg) {
    while (is_cmos_update_in_progress());
    return read_cmos_register(reg);
}

/* Decode and display active RTC real-world date and time */
void print_system_date_time() {
    unsigned char second = get_cmos_value(0x00);
    unsigned char minute = get_cmos_value(0x02);
    unsigned char hour   = get_cmos_value(0x04);
    unsigned char day    = get_cmos_value(0x07);
    unsigned char month  = get_cmos_value(0x08);
    unsigned char year   = get_cmos_value(0x09);
    unsigned char century = 0x20; // Default assumption for the 21st century

    // CMOS register 0x32 contains Century on compatible modern BIOSes
    unsigned char b_century = get_cmos_value(0x32);
    if (b_century != 0) {
        century = b_century;
    }

    // Check if CMOS values are encoded in BCD format (standard on most machines)
    unsigned char registerB = get_cmos_value(0x0B);
    if (!(registerB & 0x04)) {
        second = (second & 0x0F) + ((second >> 4) * 10);
        minute = (minute & 0x0F) + ((minute >> 4) * 10);
        hour   = (hour & 0x0F) + (((hour & 0x70) >> 4) * 10) | (hour & 0x80);
        day    = (day & 0x0F) + ((day >> 4) * 10);
        month  = (month & 0x0F) + ((month >> 4) * 10);
        year   = (year & 0x0F) + ((year >> 4) * 10);
        if (b_century != 0) {
            century = (century & 0x0F) + ((century >> 4) * 10);
        }
    }

    // Adapt 12-hour values if required by bit 1 setting in register B
    if (!(registerB & 0x02) && (hour & 0x80)) {
        hour = ((hour & 0x7F) + 12) % 24;
    }

    char s_sec[3], s_min[3], s_hr[3], s_day[3], s_mth[3], s_yr[5];
    itoa(second, s_sec, 2);
    itoa(minute, s_min, 2);
    itoa(hour, s_hr, 2);
    itoa(day, s_day, 2);
    itoa(month, s_mth, 2);
    
    // Construct year string
    int full_year = (century * 100) + year;
    itoa(full_year, s_yr, 4);

    vga_print_string("System Clock Date: ", current_text_color);
    vga_print_string(s_yr, current_text_color);
    vga_print_string("-", current_text_color);
    vga_print_string(s_mth, current_text_color);
    vga_print_string("-", current_text_color);
    vga_print_string(s_day, current_text_color);
    vga_print_string("  Time: ", current_text_color);
    vga_print_string(s_hr, current_text_color);
    vga_print_string(":", current_text_color);
    vga_print_string(s_min, current_text_color);
    vga_print_string(":", current_text_color);
    vga_print_string(s_sec, current_text_color);
    vga_print_string(" UTC\n", current_text_color);
}

/* ------------------------------------------------------------------------------
 * SECTION 4: CPU Diagnostics (CPUID & Assembly Status registers)
 * ------------------------------------------------------------------------------
 */

/* Assembly wrapper to execute standard CPUID instruction */
static inline void cpuid_get_info(int code, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx) {
    __asm__ volatile("cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(code));
}

/* Fetch and print hardware processor information */
void print_cpu_info() {
    unsigned int eax, ebx, ecx, edx;
    char vendor_str[13];

    // Read CPU Vendor brand string (EAX=0)
    cpuid_get_info(0, &eax, &ebx, &ecx, &edx);
    
    // Extract vendor bytes from EBX, EDX, ECX
    *((unsigned int*)&vendor_str[0]) = ebx;
    *((unsigned int*)&vendor_str[4]) = edx;
    *((unsigned int*)&vendor_str[8]) = ecx;
    vendor_str[12] = '\0';

    vga_print_string("CPU Model Brand Vendor: ", current_text_color);
    vga_print_string(vendor_str, current_prompt_color);
    vga_print_string("\n", current_text_color);

    // Read CPU Features (EAX=1)
    cpuid_get_info(1, &eax, &ebx, &ecx, &edx);

    vga_print_string("Processor Diagnostics: ", current_text_color);
    if (edx & (1 << 0))  vga_print_string("[FPU] ", current_text_color);
    if (edx & (1 << 4))  vga_print_string("[TSC] ", current_text_color);
    if (edx & (1 << 23)) vga_print_string("[MMX] ", current_text_color);
    if (edx & (1 << 25)) vga_print_string("[SSE] ", current_text_color);
    if (edx & (1 << 26)) vga_print_string("[SSE2] ", current_text_color);
    if (ecx & (1 << 0))  vga_print_string("[SSE3] ", current_text_color);
    vga_print_string("\n", current_text_color);
}

/* Print critical x86 Register Address Offsets on screen */
void print_architectural_registers() {
    unsigned int cr0, cr2, cr3, esp, ebp;

    // Read control registers using volatile assembly
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %%esp, %0" : "=r"(esp));
    __asm__ volatile("mov %%ebp, %0" : "=r"(ebp));

    char s_cr0[16], s_cr2[16], s_cr3[16], s_esp[16], s_ebp[16];
    htoa(cr0, s_cr0);
    htoa(cr2, s_cr2);
    htoa(cr3, s_cr3);
    htoa(esp, s_esp);
    htoa(ebp, s_ebp);

    vga_print_string("CR0 Register Space: 0x", current_text_color);
    vga_print_string(s_cr0, current_prompt_color);
    vga_print_string("   CR2: 0x", current_text_color);
    vga_print_string(s_cr2, current_prompt_color);
    vga_print_string("\nCR3 Page Directory Base: 0x", current_text_color);
    vga_print_string(s_cr3, current_prompt_color);
    vga_print_string("\nESP Stack Frame: 0x", current_text_color);
    vga_print_string(s_esp, current_prompt_color);
    vga_print_string("   EBP Frame Pointer: 0x", current_text_color);
    vga_print_string(s_ebp, current_prompt_color);
    vga_print_string("\n", current_text_color);
}

/* ------------------------------------------------------------------------------
 * SECTION 5: Memory Management Virtual Notepad Space
 * ------------------------------------------------------------------------------
 * A safe, modular system feature allowing the user to read, write, and store
 * up to 4 persistent lines of character buffers within kernel RAM.
 * ------------------------------------------------------------------------------
 */

#define NOTE_MAX_SLOTS 4
#define NOTE_LENGTH    64

static char note_database[NOTE_MAX_SLOTS][NOTE_LENGTH];
static int  note_active_slots[NOTE_MAX_SLOTS] = {0, 0, 0, 0};

/* Initialize notebook space database */
void clear_notebook() {
    for (int i = 0; i < NOTE_MAX_SLOTS; i++) {
        note_database[i][0] = '\0';
        note_active_slots[i] = 0;
    }
}

/* Write text to a specific notebook address slot */
void write_note_slot(int slot, const char* content) {
    if (slot >= 0 && slot < NOTE_MAX_SLOTS) {
        strncpy(note_database[slot], content, NOTE_LENGTH - 1);
        note_database[slot][NOTE_LENGTH - 1] = '\0';
        note_active_slots[slot] = 1;
    }
}

/* Display active notepad blocks stored inside the kernel heap */
void display_notebook() {
    int active_found = 0;
    for (int i = 0; i < NOTE_MAX_SLOTS; i++) {
        if (note_active_slots[i] != 0) {
            char num_str[4];
            itoa(i + 1, num_str, 1);
            vga_print_string(" [Notepad Slot ", current_text_color);
            vga_print_string(num_str, current_prompt_color);
            vga_print_string("]: ", current_text_color);
            vga_print_string(note_database[i], current_text_color);
            vga_print_string("\n", current_text_color);
            active_found++;
        }
    }
    if (active_found == 0) {
        vga_print_string("RAM Notebook database is currently empty.\nUse 'note-add' to store simple text logs.\n", current_text_color);
    }
}

/* ------------------------------------------------------------------------------
 * SECTION 6: Arithmetic Performance Evaluation Diagnostic
 * ------------------------------------------------------------------------------
 */

/* Calculate the factorial of a small positive number safely */
int compute_factorial(int val) {
    if (val < 0) return 0;
    if (val == 0 || val == 1) return 1;
    int result = 1;
    for (int i = 2; i <= val; i++) {
        result *= i;
    }
    return result;
}

/* Compute Fibonacci series values */
int compute_fibonacci(int term) {
    if (term <= 0) return 0;
    if (term == 1) return 1;
    int prev2 = 0;
    int prev1 = 1;
    int current = 0;
    for (int i = 2; i <= term; i++) {
        current = prev1 + prev2;
        prev2 = prev1;
        prev1 = current;
    }
    return current;
}

/* Basic Command Line Calculator parser tool */
void execute_calculator_logic(const char* args) {
    char s_num1[16];
    char s_num2[16];
    char op = ' ';
    int i = 0, j = 0;

    // Parse out first integer argument
    while (args[i] == ' ' || args[i] == '\t') i++;
    while (args[i] >= '0' && args[i] <= '9') {
        if (j < 15) s_num1[j++] = args[i];
        i++;
    }
    s_num1[j] = '\0';

    // Parse out Operator
    while (args[i] == ' ' || args[i] == '\t') i++;
    if (args[i] == '+' || args[i] == '-' || args[i] == '*' || args[i] == '/' || args[i] == '!') {
        op = args[i];
        i++;
    } else {
        vga_print_string("Usage: calc <number> <operator> <number>   Operators: +, -, *, /, !\n", current_text_color);
        return;
    }

    // Handled single-operand operations (Factorial !)
    if (op == '!') {
        int val = atoi(s_num1);
        int res = compute_factorial(val);
        char s_res[24];
        itoa(res, s_res, 1);
        vga_print_string("Result: ", current_text_color);
        vga_print_string(s_res, current_prompt_color);
        vga_print_string("\n", current_text_color);
        return;
    }

    // Parse second integer operand
    j = 0;
    while (args[i] == ' ' || args[i] == '\t') i++;
    while (args[i] >= '0' && args[i] <= '9') {
        if (j < 15) s_num2[j++] = args[i];
        i++;
    }
    s_num2[j] = '\0';

    if (strlen(s_num1) == 0 || strlen(s_num2) == 0) {
        vga_print_string("Syntax error. Ex: calc 15 + 32\n", current_text_color);
        return;
    }

    int n1 = atoi(s_num1);
    int n2 = atoi(s_num2);
    int output = 0;

    if (op == '+') {
        output = n1 + n2;
    } else if (op == '-') {
        output = n1 - n2;
    } else if (op == '*') {
        output = n1 * n2;
    } else if (op == '/') {
        if (n2 == 0) {
            vga_print_string("Execution blocked: Division by zero exception.\n", COLOR_RED_ON_BLACK);
            return;
        }
        output = n1 / n2;
    }

    char s_output[24];
    itoa(output, s_output, 1);
    vga_print_string("Result: ", current_text_color);
    vga_print_string(s_output, current_prompt_color);
    vga_print_string("\n", current_text_color);
}

/* ------------------------------------------------------------------------------
 * SECTION 7: Execution Shell History Tracker
 * ------------------------------------------------------------------------------
 */

#define MAX_HISTORY_SLOTS 8
static char shell_history[MAX_HISTORY_SLOTS][64];
static int  shell_history_count = 0;

/* Log commands executed during session loop */
void push_history(const char* cmd) {
    if (strlen(cmd) == 0) return;

    // Shift previous commands upward in order
    for (int i = MAX_HISTORY_SLOTS - 1; i > 0; i--) {
        strncpy(shell_history[i], shell_history[i - 1], 63);
        shell_history[i][63] = '\0';
    }
    strncpy(shell_history[0], cmd, 63);
    shell_history[0][63] = '\0';

    if (shell_history_count < MAX_HISTORY_SLOTS) {
        shell_history_count++;
    }
}

/* Display active shell history stack */
void display_history() {
    vga_print_string("Recent Shell Commands:\n", current_prompt_color);
    for (int i = 0; i < shell_history_count; i++) {
        char idx_str[4];
        itoa(i + 1, idx_str, 1);
        vga_print_string(" [", current_text_color);
        vga_print_string(idx_str, current_text_color);
        vga_print_string("]: ", current_text_color);
        vga_print_string(shell_history[i], current_text_color);
        vga_print_string("\n", current_text_color);
    }
}

/* ------------------------------------------------------------------------------
 * SECTION 8: Low-Level Motherboard Routines (Reboot & Safe Powerdown)
 * ------------------------------------------------------------------------------
 */

/* Safely trigger a CPU reboot using PS/2 Keyboard Controller output line reset */
void sys_reboot() {
    vga_print_string("Sending hardware reset command via PS/2... Goodbye!\n", current_prompt_color);

    // Clear keyboard controller command buffers
    unsigned char temp;
    do {
        temp = inb(0x64);
        if (temp & 1) {
            inb(0x60); // Flush outstanding buffer bytes
        }
    } while (temp & 2);

    // Pulse the reset line (port 0x64 bit 0xFE pulses CPU pipeline reset pin)
    outb(0x64, 0xFE);

    // Assembly fallback safety loop
    while (1) {
        __asm__ volatile("hlt");
    }
}

/* Clear Interrupt Flags and halt the CPU safely */
void sys_halt() {
    vga_clear_screen();
    vga_print_string("\n\n\n", current_text_color);
    vga_print_string("             ==============================================\n", current_prompt_color);
    vga_print_string("                     PingOS has been safely shut down.     \n", current_prompt_color);
    vga_print_string("                     It is now safe to power off your PC.  \n", current_text_color);
    vga_print_string("             ==============================================\n", current_prompt_color);

    __asm__ volatile("cli");
    while (1) {
        __asm__ volatile("hlt");
    }
}

/* ------------------------------------------------------------------------------
 * SECTION 9: Interactive Shell Engine
 * ------------------------------------------------------------------------------
 */

/* PingOS Simple Interactive Shell */
void run_shell() {
    char input_buffer[64];
    int input_index = 0;

    // Clean internal structures initially
    clear_notebook();

    vga_print_string("pingos> ", current_prompt_color);

    while (1) {
        char key = wait_for_key();

        if (key == '\n') {
            vga_put_char('\n', current_text_color);
            input_buffer[input_index] = '\0'; // Null-terminate user input

            // Save valid text to shell history
            push_history(input_buffer);

            // Execute matching shell commands
            if (strcmp(input_buffer, "help") == 0) {
                vga_print_string("Available Commands:\n", current_text_color);
                vga_print_string("  help         - Display this detailed guide\n", current_text_color);
                vga_print_string("  info         - Show microarchitectural project details\n", current_text_color);
                vga_print_string("  devices      - Scan PCI routes (USB & graphics controllers)\n", current_text_color);
                vga_print_string("  time         - Real Time Clock CMOS interface diagnostics\n", current_text_color);
                vga_print_string("  cpu          - Query processor architecture via CPUID\n", current_text_color);
                vga_print_string("  registers    - Hex dump registers CR0, CR2, ESP, EBP\n", current_text_color);
                vga_print_string("  theme        - Cycle color configurations dynamically\n", current_text_color);
                vga_print_string("  calc         - Freestanding calculator (ex: calc 4 * 12 or calc 5 !)\n", current_text_color);
                vga_print_string("  note-add     - Write logs directly to temporary RAM slot\n", current_text_color);
                vga_print_string("  note-view    - View cached lines inside virtual notepad\n", current_text_color);
                vga_print_string("  note-clear   - Flush all notes from system memory\n", current_text_color);
                vga_print_string("  history      - List the stack of commands run during this loop\n", current_text_color);
                vga_print_string("  uname        - Show operating system identification details\n", current_text_color);
                vga_print_string("  uname -r     - Display specific kernel development version\n", current_text_color);
                vga_print_string("  uname -a     - Dump comprehensive OS and system parameters\n", current_text_color);
                vga_print_string("  ping         - Test system execution and responsive hardware loops\n", current_text_color);
                vga_print_string("  clear        - Flush video memory buffer and clean screen\n", current_text_color);
                vga_print_string("  reboot       - Issue CPU reset sequence via motherboard register\n", current_text_color);
                vga_print_string("  halt         - Disengage interrupts and sleep cpu pipeline safely\n", current_text_color);
            } 
            else if (strcmp(input_buffer, "uname") == 0) {
                vga_print_string("PingOS\n", current_text_color);
            } 
            else if (strcmp(input_buffer, "uname -r") == 0) {
                vga_print_string("0.1.0-testing\n", current_text_color);
            } 
            else if (strcmp(input_buffer, "uname -a") == 0) {
                vga_print_string("PingOS local-machine 0.1.0-testing 1995-10-31 x86-32 Intel-Compatible ProtectedMode\n", current_text_color);
            } 
            else if (strcmp(input_buffer, "info") == 0) {
                vga_print_string("PingOS: Freestanding 32-bit Assembly & C Kernel.\n", current_text_color);
                vga_print_string("Loaded directly into Physical address space at 0x10000 via PingBoot.\n", current_text_color);
                vga_print_string("Fully customized execution, interrupt-safe control structures.\n", current_text_color);
            } 
            else if (strcmp(input_buffer, "ping") == 0) {
                vga_print_string("PONG! System core is active. Multi-sector kernel loaded flawlessly.\n", current_text_color);
            } 
            else if (strcmp(input_buffer, "clear") == 0) {
                vga_clear_screen();
            } 
            else if (strcmp(input_buffer, "time") == 0) {
                print_system_date_time();
            } 
            else if (strcmp(input_buffer, "cpu") == 0) {
                print_cpu_info();
            } 
            else if (strcmp(input_buffer, "registers") == 0) {
                print_architectural_registers();
            } 
            else if (strcmp(input_buffer, "history") == 0) {
                display_history();
            } 
            else if (strcmp(input_buffer, "note-view") == 0) {
                display_notebook();
            } 
            else if (strcmp(input_buffer, "note-clear") == 0) {
                clear_notebook();
                vga_print_string("Virtual notepad database cleared completely.\n", current_text_color);
            } 
            else if (strncmp(input_buffer, "note-add ", 9) == 0) {
                const char* note_content = input_buffer + 9;
                int free_slot = -1;
                for (int s = 0; s < NOTE_MAX_SLOTS; s++) {
                    if (note_active_slots[s] == 0) {
                        free_slot = s;
                        break;
                    }
                }
                if (free_slot != -1) {
                    write_note_slot(free_slot, note_content);
                    char slot_ch[4];
                    itoa(free_slot + 1, slot_ch, 1);
                    vga_print_string("Logged string safely into system slot ", current_text_color);
                    vga_print_string(slot_ch, current_prompt_color);
                    vga_print_string(".\n", current_text_color);
                } else {
                    vga_print_string("Notebook capacity full! Use 'note-clear' to release blocks.\n", COLOR_AMBER_ON_BLACK);
                }
            } 
            else if (strncmp(input_buffer, "calc ", 5) == 0) {
                execute_calculator_logic(input_buffer + 5);
            } 
            else if (strcmp(input_buffer, "reboot") == 0) {
                sys_reboot();
            } 
            else if (strcmp(input_buffer, "halt") == 0) {
                sys_halt();
            } 
            else if (strcmp(input_buffer, "theme") == 0) {
                // Cylindrical shift among system visual attributes
                if (current_prompt_color == COLOR_WHITE_ON_BLACK) {
                    current_prompt_color = COLOR_GREEN_ON_BLACK;
                    current_text_color   = COLOR_GREY_ON_BLACK;
                    vga_print_string("Color style updated to: Emerald Green terminal.\n", current_prompt_color);
                } else if (current_prompt_color == COLOR_GREEN_ON_BLACK) {
                    current_prompt_color = COLOR_BLUE_ON_BLACK;
                    current_text_color   = COLOR_BLUE_ON_BLACK;
                    vga_print_string("Color style updated to: Ocean Blue terminal.\n", current_prompt_color);
                } else if (current_prompt_color == COLOR_BLUE_ON_BLACK) {
                    current_prompt_color = COLOR_AMBER_ON_BLACK;
                    current_text_color   = COLOR_AMBER_ON_BLACK;
                    vga_print_string("Color style updated to: Retro Amber terminal.\n", current_prompt_color);
                } else {
                    current_prompt_color = COLOR_WHITE_ON_BLACK;
                    current_text_color   = COLOR_GREY_ON_BLACK;
                    vga_print_string("Color style updated to: Standard Monochrome.\n", current_prompt_color);
                }
            } 
            else if (strcmp(input_buffer, "devices") == 0) {
                vga_print_string("Probing system devices...\n", current_text_color);
                usb_init();
                intel_gpu_init();
            } 
            else if (input_index > 0) {
                vga_print_string("Error: Unknown command. Type 'help' for options.\n", current_prompt_color);
            }

            // Reset shell prompt
            input_index = 0;
            vga_print_string("pingos> ", current_prompt_color);
        } else if (key == '\b') {
            // Support Backspace deletion in input buffer
            if (input_index > 0) {
                input_index--;
                vga_put_char('\b', current_text_color);
            }
        } else {
            // Write normal key into shell buffer if it fits
            if (input_index < 63) {
                input_buffer[input_index++] = key;
                vga_put_char(key, current_text_color);
            }
        }
    }
}

/* ------------------------------------------------------------------------------
 * SECTION 10: Kernel Entry Initialization
 * ------------------------------------------------------------------------------
 */

/* Kernel Entry Point */
void main() {
    // 1. Clear VGA display memory
    vga_clear_screen();

    // 2. Initialize Hardware Keyboards
    keyboard_init();
    mouse_init();

    // 3. Draw Splash Screen (Monochrome layout exactly restored)
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