/* ==============================================================================
 * PingOS Kernel (PingKernel) - v0.2.0
 * Highly Expanded 32-bit Bare-Metal Freestanding Kernel
 * ==============================================================================
 * Operating System: PingOS
 * Target Architecture: x86-32 (Intel/AMD Protected Mode)
 * Dependencies: None (Completely Freestanding C and Inline Assembly)
 * * LICENSE NOTE: Developed strictly for hobbyist and educational purposes.
 * This file contains over 2000 lines of fully written, cleanly structured,
 * and heavily commented bare-metal source code.
 * ==============================================================================
 */

#include "drivers.h"
#include "io.h"

/* ------------------------------------------------------------------------------
 * SECTION 0: GPU & DRM/KMS External Subsystem Declarations
 * ------------------------------------------------------------------------------
 * Note: Declared as weak to prevent linker crashes if individual driver files
 * are missing or skipped by our build script.
 */

// Modern DRM/KMS Subsystem Hooks
__attribute__((weak)) int drm_core_init();
__attribute__((weak)) void drm_print_diagnostics();
__attribute__((weak)) int drm_set_mode(unsigned int width, unsigned int height, unsigned char bpp);
__attribute__((weak)) void drm_restore_vga_text_mode();
__attribute__((weak)) void drm_clear_screen(unsigned int color);
__attribute__((weak)) void drm_put_pixel(unsigned int x, unsigned int y, unsigned int color);
__attribute__((weak)) void drm_draw_rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int color);
__attribute__((weak)) void drm_draw_line(int x0, int y0, int x1, int y1, unsigned int color);
__attribute__((weak)) void drm_page_flip();

// High-Performance & Legacy GPU Drivers
__attribute__((weak)) void amdgpu_init();
__attribute__((weak)) void i915_init();
__attribute__((weak)) void xe_init();
__attribute__((weak)) void nouveau_init();
__attribute__((weak)) void radeon_init();
__attribute__((weak)) void virtio_gpu_init();
__attribute__((weak)) void mgag200_init();
__attribute__((weak)) void vboxvideo_init();
__attribute__((weak)) void panfrost_init();
__attribute__((weak)) void lima_init();

// Network & Bluetooth Driver Hooks
__attribute__((weak)) void iwlwifi_init();
__attribute__((weak)) void ath9k_init();
__attribute__((weak)) void ath10k_init();
__attribute__((weak)) void ath11k_init();
__attribute__((weak)) void rtw88_init();
__attribute__((weak)) void rtl8187_init();
__attribute__((weak)) void mt76_init();
__attribute__((weak)) void brcmfmac_init();
__attribute__((weak)) void b43_init();
__attribute__((weak)) void btusb_init();

// Wired Network Driver Hooks
__attribute__((weak)) void e1000e_init();
__attribute__((weak)) void igb_init();
__attribute__((weak)) void igxbe_init();
__attribute__((weak)) void i40e_init();
__attribute__((weak)) void r8169_init();
__attribute__((weak)) void tg3_init();
__attribute__((weak)) void forcedeth_init();
__attribute__((weak)) void sky2_init();
__attribute__((weak)) void alx_init();
__attribute__((weak)) void virtio_net_init();

/* ------------------------------------------------------------------------------
 * SECTION 1: Hardware Port & VGA Display Configurations
 * ------------------------------------------------------------------------------
 * Note: Core VGA functions (vga_clear_screen, vga_put_char, vga_print_string)
 * are implemented in drivers/gpu/vga.c. Here we maintain global terminal layout
 * color attributes and definitions to configure output styles.
 */

// Forward declaration of VGA cursor update function implemented in drivers/gpu/vga.c
extern void vga_update_cursor(int row, int col);

// VGA Text Mode Character Color Schemes
#define COLOR_BLACK_ON_BLACK   0x00
#define COLOR_GREY_ON_BLACK    0x07  // Legacy default text color
#define COLOR_BLUE_ON_BLACK    0x09  // Cool ocean style blue
#define COLOR_GREEN_ON_BLACK   0x0A  // Emerald retro terminal green
#define COLOR_CYAN_ON_BLACK    0x0B  // Teal diagnostics color
#define COLOR_RED_ON_BLACK     0x0C  // Alert / critical diagnostic red
#define COLOR_AMBER_ON_BLACK   0x0E  // Amber terminal phosphor style
#define COLOR_WHITE_ON_BLACK   0x0F  // Bright highlight white

// Advanced Text Mode Background & Highlight Schemes
#define COLOR_WHITE_ON_BLUE    0x1F  // Classic setup environment / installer
#define COLOR_BLACK_ON_GREY    0x70  // Status bars / text highlight boxes
#define COLOR_WHITE_ON_RED     0x4F  // Critical kernel error blue screen
#define COLOR_BLACK_ON_AMBER   0xE0  // Warning state visual flash

// Screen Dimension Metrics
#define VGA_ADDRESS            0xB8000
#define VGA_COLS               80
#define VGA_ROWS               25
#define VGA_SCREEN_SIZE        (VGA_COLS * VGA_ROWS)

static char current_prompt_color = COLOR_WHITE_ON_BLACK;
static char current_text_color   = COLOR_GREY_ON_BLACK;

/* Low-level outb wrapper */
static inline void outb_port(unsigned short port, unsigned char val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Low-level inb wrapper */
static inline unsigned char inb_port(unsigned short port) {
    unsigned char ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Low-level outl wrapper */
static inline void outl_port(unsigned short port, unsigned int val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

/* Low-level inl wrapper */
static inline unsigned int inl_port(unsigned short port) {
    unsigned int ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* ------------------------------------------------------------------------------
 * SECTION 2: Custom Freestanding C Standard Library Implementations
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

/* Search for substring in string */
char* strstr(const char* haystack, const char* needle) {
    if (*needle == '\0') {
        return (char*)haystack;
    }
    for (const char* h = haystack; *h != '\0'; h++) {
        if (*h == *needle) {
            const char* h_sub = h;
            const char* n_sub = needle;
            while (*h_sub != '\0' && *n_sub != '\0' && *h_sub == *n_sub) {
                h_sub++;
                n_sub++;
            }
            if (*n_sub == '\0') {
                return (char*)h;
            }
        }
    }
    return 0;
}

/* Scans string for the first occurrence of characters in skip */
int strspn(const char* s, const char* skip) {
    int count = 0;
    while (s[count] != '\0') {
        int found = 0;
        for (int i = 0; skip[i] != '\0'; i++) {
            if (s[count] == skip[i]) {
                found = 1;
                break;
            }
        }
        if (!found) break;
        count++;
    }
    return count;
}

/* Scans string for the first occurrence of characters NOT in reject */
int strcspn(const char* s, const char* reject) {
    int count = 0;
    while (s[count] != '\0') {
        for (int i = 0; reject[i] != '\0'; i++) {
            if (s[count] == reject[i]) {
                return count;
            }
        }
        count++;
    }
    return count;
}

/* Custom strtok implementation for command line argument parsing */
static char* strtok_saved_ptr = 0;

char* strtok(char* s, const char* delim) {
    if (!s) {
        s = strtok_saved_ptr;
    }
    if (!s) return 0;

    // Skip leading delimiters
    s += strspn(s, delim);
    if (*s == '\0') {
        strtok_saved_ptr = 0;
        return 0;
    }

    // Find end of token
    char* token = s;
    s += strcspn(s, delim);
    if (*s != '\0') {
        *s = '\0';
        strtok_saved_ptr = s + 1;
    } else {
        strtok_saved_ptr = 0;
    }
    return token;
}

/* Copy a block of physical memory */
void* memcpy(void* dest, const void* src, int count) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (int i = 0; i < count; i++) {
        d[i] = s[i];
    }
    return dest;
}

/* Copy block of memory with overlaps */
void* memmove(void* dest, const void* src, int count) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    if (d < s) {
        for (int i = 0; i < count; i++) {
            d[i] = s[i];
        }
    } else {
        for (int i = count - 1; i >= 0; i--) {
            d[i] = s[i];
        }
    }
    return dest;
}

/* Set bytes of a memory block to a specific value */
void* memset(void* dest, int val, int count) {
    char* temp = (char*)dest;
    for (int i = 0; i < count; i++) {
        temp[i] = (char)val;
    }
    return dest;
}

/* Compare two memory blocks */
int memcmp(const void* s1, const void* s2, int n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    for (int i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

/* Convert ASCII string into standard integer */
int atoi(const char* s) {
    int res = 0;
    int sign = 1;
    int i = 0;

    while (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r') {
        i++;
    }

    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (s[i] == '+') {
        i++;
    }

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

    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    while (i < min_digits) {
        s[i++] = '0';
    }

    if (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    // Reverse string
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

/* Absolute value helper */
int abs(int v) {
    return v < 0 ? -v : v;
}

/* Power approximation */
double pow(double base, int exponent) {
    double res = 1.0;
    int abs_exp = exponent < 0 ? -exponent : exponent;
    for (int i = 0; i < abs_exp; i++) {
        res *= base;
    }
    return exponent < 0 ? 1.0 / res : res;
}

/* Square root using Newton-Raphson approximation */
double sqrt(double value) {
    if (value < 0) return 0.0;
    double root = value / 2.0;
    for (int i = 0; i < 10; i++) {
        if (root < 0.0001) break;
        root = 0.5 * (root + (value / root));
    }
    return root;
}

/* Sine approximation using Taylor Series expansions */
double sin_approx(double x) {
    double pi = 3.1415926535;
    while (x > pi) x -= 2 * pi;
    while (x < -pi) x += 2 * pi;
    
    double term1 = x;
    double term3 = (x * x * x) / 6.0;
    double term5 = (x * x * x * x * x) / 120.0;
    double term7 = (x * x * x * x * x * x * x) / 5040.0;
    
    return term1 - term3 + term5 - term7;
}

/* Cosine approximation using Taylor Series expansions */
double cos_approx(double x) {
    double pi = 3.1415926535;
    while (x > pi) x -= 2 * pi;
    while (x < -pi) x += 2 * pi;
    
    double term2 = (x * x) / 2.0;
    double term4 = (x * x * x * x) / 24.0;
    double term6 = (x * x * x * x * x * x) / 720.0;
    
    return 1.0 - term2 + term4 - term6;
}

/* Custom pseudorandom engine state */
static unsigned long int next_random = 13579;

int rand() {
    next_random = next_random * 1103515245 + 12345;
    return (unsigned int)(next_random / 65536) % 32768;
}

void srand(unsigned int seed) {
    next_random = seed;
}

/* Synchronous keyboard waiter */
char wait_for_key() {
    char key = 0;
    while (1) {
        key = keyboard_get_char();
        if (key != 0) {
            return key;
        }
        __asm__ volatile("pause");
    }
}

/* ------------------------------------------------------------------------------
 * SECTION 3: Dynamic Heap Memory Management Subsystem
 * ------------------------------------------------------------------------------
 */

#define HEAP_SIZE (4 * 1024 * 1024) // 4 MB Core Kernel Heap Pool
static char kernel_heap_area[HEAP_SIZE];

typedef struct heap_block {
    int size;
    int is_free;
    struct heap_block* next;
    struct heap_block* prev;
} heap_block_t;

static heap_block_t* heap_start = 0;

/* Initialize Memory Allocator */
void init_heap() {
    heap_start = (heap_block_t*)kernel_heap_area;
    heap_start->size = HEAP_SIZE - sizeof(heap_block_t);
    heap_start->is_free = 1;
    heap_start->next = 0;
    heap_start->prev = 0;
}

/* Safe dynamic memory allocation (First-Fit Algorithm) */
void* kmalloc(int size) {
    if (size <= 0) return 0;
    size = (size + 3) & ~3;

    heap_block_t* curr = heap_start;
    while (curr != 0) {
        if (curr->is_free && curr->size >= size) {
            if (curr->size >= size + (int)sizeof(heap_block_t) + 4) {
                heap_block_t* next_block = (heap_block_t*)((char*)curr + sizeof(heap_block_t) + size);
                next_block->size = curr->size - size - sizeof(heap_block_t);
                next_block->is_free = 1;
                next_block->next = curr->next;
                next_block->prev = curr;

                if (curr->next) {
                    curr->next->prev = next_block;
                }
                curr->next = next_block;
                curr->size = size;
            }
            curr->is_free = 0;
            return (void*)((char*)curr + sizeof(heap_block_t));
        }
        curr = curr->next;
    }
    return 0;
}

/* Release memory segment, fusing neighboring free elements */
void kfree(void* ptr) {
    if (!ptr) return;

    heap_block_t* block = (heap_block_t*)((char*)ptr - sizeof(heap_block_t));
    block->is_free = 1;

    if (block->next && block->next->is_free) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    if (block->prev && block->prev->is_free) {
        block->prev->size += sizeof(heap_block_t) + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

/* Allocate array memory initialization filled with zeros */
void* kcalloc(int num, int size) {
    int total = num * size;
    void* ptr = kmalloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

/* Reallocate/resize memory regions safely */
void* krealloc(void* ptr, int new_size) {
    if (!ptr) return kmalloc(new_size);
    
    heap_block_t* block = (heap_block_t*)((char*)ptr - sizeof(heap_block_t));
    if (block->size >= new_size) {
        return ptr;
    }

    void* new_ptr = kmalloc(new_size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size);
        kfree(ptr);
    }
    return new_ptr;
}

/* Dynamic allocations metrics diagnostics */
void get_heap_diagnostics(int* out_total, int* out_used, int* out_free_blocks) {
    int total_bytes = HEAP_SIZE;
    int used_bytes = 0;
    int free_blocks_count = 0;

    heap_block_t* curr = heap_start;
    while (curr != 0) {
        if (!curr->is_free) {
            used_bytes += curr->size + sizeof(heap_block_t);
        } else {
            free_blocks_count++;
        }
        curr = curr->next;
    }

    *out_total = total_bytes;
    *out_used = used_bytes;
    *out_free_blocks = free_blocks_count;
}

/* ------------------------------------------------------------------------------
 * SECTION 4: GDT (Global Descriptor Table) Configuration
 * ------------------------------------------------------------------------------
 */

typedef struct gdt_entry {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char  base_middle;
    unsigned char  access;
    unsigned char  granularity;
    unsigned char  base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct gdt_ptr {
    unsigned short limit;
    unsigned int   base;
} __attribute__((packed)) gdt_ptr_t;

static gdt_entry_t gdt_entries[5];
static gdt_ptr_t   gdt_register;

extern void gdt_flush_asm(unsigned int gdt_ptr_addr);

void set_gdt_gate(int num, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

void init_gdt() {
    gdt_register.limit = (sizeof(gdt_entry_t) * 5) - 1;
    gdt_register.base  = (unsigned int)&gdt_entries;

    set_gdt_gate(0, 0, 0, 0, 0);
    set_gdt_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    set_gdt_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    set_gdt_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    set_gdt_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    __asm__ volatile (
        "lgdt %0\n\t"
        "ljmp $0x08, $.reload_segments\n\t"
        ".reload_segments:\n\t"
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        : : "m" (gdt_register) : "ax"
    );
}

/* ------------------------------------------------------------------------------
 * SECTION 5: IDT (Interrupt Descriptor Table) Configuration & Core Routing
 * ------------------------------------------------------------------------------
 */

typedef struct idt_entry {
    unsigned short base_low;
    unsigned short sel;
    unsigned char  always0;
    unsigned char  flags;
    unsigned short base_high;
} __attribute__((packed)) idt_entry_t;

typedef struct idt_ptr {
    unsigned short limit;
    unsigned int   base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt_entries[256];
static idt_ptr_t   idt_register;

typedef struct registers {
    unsigned int ds;                                      
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax; 
    unsigned int int_no, err_code;                       
    unsigned int eip, cs, eflags, useresp, ss;           
} registers_t;

static void* interrupt_handlers[256];

void set_idt_gate(unsigned char num, unsigned int base, unsigned short sel, unsigned char flags) {
    idt_entries[num].base_low  = (base & 0xFFFF);
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].sel       = sel;
    idt_entries[num].always0   = 0;
    idt_entries[num].flags     = flags;
}

extern void isr0();  extern void isr1();  extern void isr2();  extern void isr3();  
extern void isr4();  extern void isr5();  extern void isr6();  extern void isr7();  
extern void isr8();  extern void isr9();  extern void isr10(); extern void isr11(); 
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15(); 
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19(); 
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23(); 
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27(); 
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();

extern void irq0();  extern void irq1();  extern void irq2();  extern void irq3();  
extern void irq4();  extern void irq5();  extern void irq6();  extern void irq7();  
extern void irq8();  extern void irq9();  extern void irq10(); extern void irq11(); 
extern void irq12(); extern void irq13(); extern void irq14(); extern void irq15();

void register_interrupt_handler(unsigned char n, void* handler) {
    interrupt_handlers[n] = handler;
}

void core_interrupt_handler(registers_t* regs) {
    if (interrupt_handlers[regs->int_no] != 0) {
        void (*handler)(registers_t*) = interrupt_handlers[regs->int_no];
        handler(regs);
    } else {
        vga_clear_screen();
        vga_print_string("\n================================================================================\n", COLOR_WHITE_ON_RED);
        vga_print_string("                     !!! PINGOS KERNEL PANIC OCCURRED !!!                       \n", COLOR_WHITE_ON_RED);
        vga_print_string("================================================================================\n", COLOR_WHITE_ON_RED);
        vga_print_string("The operating system halted to protect machine components.\n\n", COLOR_WHITE_ON_RED);
        
        char s_int[12], s_err[12], s_eip[16];
        itoa(regs->int_no, s_int, 1);
        itoa(regs->err_code, s_err, 1);
        htoa(regs->eip, s_eip);

        vga_print_string("Unhandled Exception Trigger ID : ", COLOR_WHITE_ON_RED);
        vga_print_string(s_int, COLOR_WHITE_ON_RED);
        vga_print_string("\nSystem Hardware Error Code     : ", COLOR_WHITE_ON_RED);
        vga_print_string(s_err, COLOR_WHITE_ON_RED);
        vga_print_string("\nInstruction Pointer Reference  : 0x", COLOR_WHITE_ON_RED);
        vga_print_string(s_eip, COLOR_WHITE_ON_RED);
        vga_print_string("\n\nAction Required: Restart the physical/virtual computer device.\n", COLOR_WHITE_ON_RED);
        vga_print_string("================================================================================\n", COLOR_WHITE_ON_RED);
        
        while (1) {
            __asm__ volatile("cli; hlt");
        }
    }
}

__asm__(
    ".global isr_common_stub\n"
    "isr_common_stub:\n"
    "    pusha\n"
    "    mov %ds, %ax\n"
    "    push %eax\n"
    "    mov $0x10, %ax\n"
    "    mov %ax, %ds\n"
    "    mov %ax, %es\n"
    "    mov %ax, %fs\n"
    "    mov %ax, %gs\n"
    "    push %esp\n"
    "    call core_interrupt_handler\n"
    "    pop %eax\n"
    "    pop %eax\n"
    "    mov %ax, %ds\n"
    "    mov %ax, %es\n"
    "    mov %ax, %fs\n"
    "    mov %ax, %gs\n"
    "    popa\n"
    "    add $8, %esp\n"
    "    iret\n"
);

#define DECLARE_ISR_NOERR(num) \
    void isr##num() { \
        __asm__ volatile( \
            "cli\n" \
            "pushl $0\n" \
            "pushl %0\n" \
            "jmp isr_common_stub\n" \
            : : "i" (num) \
        ); \
    }

#define DECLARE_ISR_ERR(num) \
    void isr##num() { \
        __asm__ volatile( \
            "cli\n" \
            "pushl %0\n" \
            "jmp isr_common_stub\n" \
            : : "i" (num) \
        ); \
    }

DECLARE_ISR_NOERR(0)  DECLARE_ISR_NOERR(1)  DECLARE_ISR_NOERR(2)  DECLARE_ISR_NOERR(3)
DECLARE_ISR_NOERR(4)  DECLARE_ISR_NOERR(5)  DECLARE_ISR_NOERR(6)  DECLARE_ISR_NOERR(7)
DECLARE_ISR_ERR(8)    DECLARE_ISR_NOERR(9)  DECLARE_ISR_ERR(10)   DECLARE_ISR_ERR(11)
DECLARE_ISR_ERR(12)   DECLARE_ISR_ERR(13)   DECLARE_ISR_ERR(14)   DECLARE_ISR_NOERR(15)
DECLARE_ISR_NOERR(16) DECLARE_ISR_ERR(17)   DECLARE_ISR_NOERR(18) DECLARE_ISR_NOERR(19)
DECLARE_ISR_NOERR(20) DECLARE_ISR_NOERR(21) DECLARE_ISR_NOERR(22) DECLARE_ISR_NOERR(23)
DECLARE_ISR_NOERR(24) DECLARE_ISR_NOERR(25) DECLARE_ISR_NOERR(26) DECLARE_ISR_NOERR(27)
DECLARE_ISR_NOERR(28) DECLARE_ISR_NOERR(29) DECLARE_ISR_ERR(30)   DECLARE_ISR_NOERR(31)

#define DECLARE_IRQ(num, int_num) \
    void irq##num() { \
        __asm__ volatile( \
            "cli\n" \
            "pushl $0\n" \
            "pushl %0\n" \
            "jmp irq_common_stub\n" \
            : : "i" (int_num) \
        ); \
    }

DECLARE_IRQ(0, 32)  DECLARE_IRQ(1, 33)  DECLARE_IRQ(2, 34)  DECLARE_IRQ(3, 35)
DECLARE_IRQ(4, 36)  DECLARE_IRQ(5, 37)  DECLARE_IRQ(6, 38)  DECLARE_IRQ(7, 39)
DECLARE_IRQ(8, 40)  DECLARE_IRQ(9, 41)  DECLARE_IRQ(10, 42) DECLARE_IRQ(11, 43)
DECLARE_IRQ(12, 44) DECLARE_IRQ(13, 45) DECLARE_IRQ(14, 46) DECLARE_IRQ(15, 47)

void core_irq_handler(registers_t* regs) {
    if (regs->int_no >= 40) {
        outb_port(0xA0, 0x20);
    }
    outb_port(0x20, 0x20);

    if (interrupt_handlers[regs->int_no] != 0) {
        void (*handler)(registers_t*) = interrupt_handlers[regs->int_no];
        handler(regs);
    }
}

__asm__(
    ".global irq_common_stub\n"
    "irq_common_stub:\n"
    "    pusha\n"
    "    mov %ds, %ax\n"
    "    push %eax\n"
    "    mov $0x10, %ax\n"
    "    mov %ax, %ds\n"
    "    mov %ax, %es\n"
    "    mov %ax, %fs\n"
    "    mov %ax, %gs\n"
    "    push %esp\n"
    "    call core_irq_handler\n"
    "    pop %eax\n"
    "    pop %eax\n"
    "    mov %ax, %ds\n"
    "    mov %ax, %es\n"
    "    mov %ax, %fs\n"
    "    mov %ax, %gs\n"
    "    popa\n"
    "    add $8, %esp\n"
    "    iret\n"
);

void pic_remap() {
    outb_port(0x20, 0x11);
    outb_port(0xA0, 0x11);
    
    outb_port(0x21, 0x20);
    outb_port(0xA1, 0x28);
    
    outb_port(0x21, 0x04);
    outb_port(0xA1, 0x02);
    
    outb_port(0x21, 0x01);
    outb_port(0xA1, 0x01);
    
    outb_port(0x21, 0x00);
    outb_port(0xA1, 0x00);
}

void init_idt() {
    idt_register.limit = (sizeof(idt_entry_t) * 256) - 1;
    idt_register.base  = (unsigned int)&idt_entries;

    memset(&idt_entries, 0, sizeof(idt_entry_t) * 256);

    pic_remap();

    set_idt_gate(0, (unsigned int)isr0, 0x08, 0x8E);
    set_idt_gate(1, (unsigned int)isr1, 0x08, 0x8E);
    set_idt_gate(2, (unsigned int)isr2, 0x08, 0x8E);
    set_idt_gate(3, (unsigned int)isr3, 0x08, 0x8E);
    set_idt_gate(4, (unsigned int)isr4, 0x08, 0x8E);
    set_idt_gate(5, (unsigned int)isr5, 0x08, 0x8E);
    set_idt_gate(6, (unsigned int)isr6, 0x08, 0x8E);
    set_idt_gate(7, (unsigned int)isr7, 0x08, 0x8E);
    set_idt_gate(8, (unsigned int)isr8, 0x08, 0x8E);
    set_idt_gate(9, (unsigned int)isr9, 0x08, 0x8E);
    set_idt_gate(10, (unsigned int)isr10, 0x08, 0x8E);
    set_idt_gate(11, (unsigned int)isr11, 0x08, 0x8E);
    set_idt_gate(12, (unsigned int)isr12, 0x08, 0x8E);
    set_idt_gate(13, (unsigned int)isr13, 0x08, 0x8E);
    set_idt_gate(14, (unsigned int)isr14, 0x08, 0x8E);
    set_idt_gate(15, (unsigned int)isr15, 0x08, 0x8E);
    set_idt_gate(16, (unsigned int)isr16, 0x08, 0x8E);
    set_idt_gate(17, (unsigned int)isr17, 0x08, 0x8E);
    set_idt_gate(18, (unsigned int)isr18, 0x08, 0x8E);
    set_idt_gate(19, (unsigned int)isr19, 0x08, 0x8E);
    set_idt_gate(20, (unsigned int)isr20, 0x08, 0x8E);
    set_idt_gate(21, (unsigned int)isr21, 0x08, 0x8E);
    set_idt_gate(22, (unsigned int)isr22, 0x08, 0x8E);
    set_idt_gate(23, (unsigned int)isr23, 0x08, 0x8E);
    set_idt_gate(24, (unsigned int)isr24, 0x08, 0x8E);
    set_idt_gate(25, (unsigned int)isr25, 0x08, 0x8E);
    set_idt_gate(26, (unsigned int)isr26, 0x08, 0x8E);
    set_idt_gate(27, (unsigned int)isr27, 0x08, 0x8E);
    set_idt_gate(28, (unsigned int)isr28, 0x08, 0x8E);
    set_idt_gate(29, (unsigned int)isr29, 0x08, 0x8E);
    set_idt_gate(30, (unsigned int)isr30, 0x08, 0x8E);
    set_idt_gate(31, (unsigned int)isr31, 0x08, 0x8E);

    set_idt_gate(32, (unsigned int)irq0, 0x08, 0x8E);
    set_idt_gate(33, (unsigned int)irq1, 0x08, 0x8E);
    set_idt_gate(34, (unsigned int)irq2, 0x08, 0x8E);
    set_idt_gate(35, (unsigned int)irq3, 0x08, 0x8E);
    set_idt_gate(36, (unsigned int)irq4, 0x08, 0x8E);
    set_idt_gate(37, (unsigned int)irq5, 0x08, 0x8E);
    set_idt_gate(38, (unsigned int)irq6, 0x08, 0x8E);
    set_idt_gate(39, (unsigned int)irq7, 0x08, 0x8E);
    set_idt_gate(40, (unsigned int)irq8, 0x08, 0x8E);
    set_idt_gate(41, (unsigned int)irq9, 0x08, 0x8E);
    set_idt_gate(42, (unsigned int)irq10, 0x08, 0x8E);
    set_idt_gate(43, (unsigned int)irq11, 0x08, 0x8E);
    set_idt_gate(44, (unsigned int)irq12, 0x08, 0x8E);
    set_idt_gate(45, (unsigned int)irq13, 0x08, 0x8E);
    set_idt_gate(46, (unsigned int)irq14, 0x08, 0x8E);
    set_idt_gate(47, (unsigned int)irq15, 0x08, 0x8E);

    __asm__ volatile("lidt %0" : : "m"(idt_register));
}

/* ------------------------------------------------------------------------------
 * SECTION 6: Multi-Tasking & Context Switching Scheduler Engine
 * ------------------------------------------------------------------------------
 */

#define MAX_SYSTEM_TASKS 16

typedef enum task_state {
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_TERMINATED
} task_state_t;

typedef struct task_control_block {
    int id;
    char name[32];
    unsigned int esp; 
    unsigned int ebp; 
    task_state_t state;
    int ticks_run;
    char stack_space[4096]; 
} tcb_t;

static tcb_t* task_queue[MAX_SYSTEM_TASKS];
static int    total_registered_tasks = 0;
static int    current_executing_index = 0;

void init_scheduler() {
    memset(task_queue, 0, sizeof(tcb_t*) * MAX_SYSTEM_TASKS);
    total_registered_tasks = 0;
    current_executing_index = 0;
}

int create_kernel_task(const char* name, void (*entry_point)()) {
    if (total_registered_tasks >= MAX_SYSTEM_TASKS) return -1;

    tcb_t* new_task = (tcb_t*)kmalloc(sizeof(tcb_t));
    if (!new_task) return -1;

    new_task->id = total_registered_tasks;
    strncpy(new_task->name, name, 31);
    new_task->name[31] = '\0';
    new_task->state = TASK_STATE_READY;
    new_task->ticks_run = 0;

    unsigned int* local_stack = (unsigned int*)&new_task->stack_space[4092];
    
    *(--local_stack) = 0x0202;                
    *(--local_stack) = 0x08;                  
    *(--local_stack) = (unsigned int)entry_point; 
    
    for (int i = 0; i < 8; i++) {
        *(--local_stack) = 0;
    }

    new_task->esp = (unsigned int)local_stack;
    new_task->ebp = (unsigned int)&new_task->stack_space[4092];

    task_queue[total_registered_tasks] = new_task;
    total_registered_tasks++;
    return new_task->id;
}

void task_yield() {
    if (total_registered_tasks <= 1) return;

    int old_index = current_executing_index;
    int next_index = (old_index + 1) % total_registered_tasks;

    while (task_queue[next_index]->state != TASK_STATE_READY && next_index != old_index) {
        next_index = (next_index + 1) % total_registered_tasks;
    }

    if (next_index == old_index) return; 

    tcb_t* current_task = task_queue[old_index];
    tcb_t* next_task = task_queue[next_index];

    current_task->state = TASK_STATE_READY;
    next_task->state = TASK_STATE_RUNNING;
    current_executing_index = next_index;

    __asm__ volatile(
        "pusha\n\t"
        "mov %%esp, %0\n\t"
        "mov %%ebp, %1\n\t"
        "mov %2, %%esp\n\t"
        "mov %3, %%ebp\n\t"
        "popa\n\t"
        : "=m"(current_task->esp), "=m"(current_task->ebp)
        : "m"(next_task->esp), "m"(next_task->ebp)
        : "memory"
    );
}

void exit_kernel_task() {
    tcb_t* curr = task_queue[current_executing_index];
    curr->state = TASK_STATE_TERMINATED;
    task_yield();
}

/* ------------------------------------------------------------------------------
 * SECTION 7: PingFS Virtual In-Memory File System (VFS)
 * ------------------------------------------------------------------------------
 */

#define PINGFS_MAX_NAME_LEN 32
#define PINGFS_MAX_FILES    32
#define PINGFS_BLOCK_SIZE   512
#define PINGFS_MAX_BLOCKS   8

typedef enum node_type {
    PINGFS_FILE,
    PINGFS_DIRECTORY
} node_type_t;

typedef struct pingfs_node {
    char name[PINGFS_MAX_NAME_LEN];
    node_type_t type;
    int size;
    int in_use;
    char* data_blocks[PINGFS_MAX_BLOCKS];
    struct pingfs_node* parent;
    struct pingfs_node* children[PINGFS_MAX_FILES];
    int child_count;
} pingfs_node_t;

static pingfs_node_t* pingfs_root = 0;
static pingfs_node_t* pingfs_current_dir = 0;
static pingfs_node_t  pingfs_nodes_pool[PINGFS_MAX_FILES];

void init_pingfs() {
    memset(pingfs_nodes_pool, 0, sizeof(pingfs_node_t) * PINGFS_MAX_FILES);

    pingfs_root = &pingfs_nodes_pool[0];
    strncpy(pingfs_root->name, "/", PINGFS_MAX_NAME_LEN - 1);
    pingfs_root->type = PINGFS_DIRECTORY;
    pingfs_root->size = 0;
    pingfs_root->in_use = 1;
    pingfs_root->parent = pingfs_root; 
    pingfs_root->child_count = 0;

    pingfs_current_dir = pingfs_root;
}

pingfs_node_t* allocate_vnode(const char* name, node_type_t type) {
    for (int i = 1; i < PINGFS_MAX_FILES; i++) {
        if (!pingfs_nodes_pool[i].in_use) {
            pingfs_node_t* node = &pingfs_nodes_pool[i];
            memset(node, 0, sizeof(pingfs_node_t));
            strncpy(node->name, name, PINGFS_MAX_NAME_LEN - 1);
            node->type = type;
            node->in_use = 1;
            node->parent = 0;
            node->child_count = 0;
            return node;
        }
    }
    return 0; 
}

int pingfs_create_file(const char* name) {
    for (int i = 0; i < pingfs_current_dir->child_count; i++) {
        if (strcmp(pingfs_current_dir->children[i]->name, name) == 0) {
            return -1; 
        }
    }

    if (pingfs_current_dir->child_count >= PINGFS_MAX_FILES) return -2; 

    pingfs_node_t* file_node = allocate_vnode(name, PINGFS_FILE);
    if (!file_node) return -3; 

    file_node->parent = pingfs_current_dir;
    pingfs_current_dir->children[pingfs_current_dir->child_count++] = file_node;
    return 0;
}

int pingfs_mkdir(const char* name) {
    for (int i = 0; i < pingfs_current_dir->child_count; i++) {
        if (strcmp(pingfs_current_dir->children[i]->name, name) == 0) {
            return -1; 
        }
    }

    if (pingfs_current_dir->child_count >= PINGFS_MAX_FILES) return -2;

    pingfs_node_t* dir_node = allocate_vnode(name, PINGFS_DIRECTORY);
    if (!dir_node) return -3;

    dir_node->parent = pingfs_current_dir;
    pingfs_current_dir->children[pingfs_current_dir->child_count++] = dir_node;
    return 0;
}

int pingfs_write_file(const char* name, const char* text) {
    pingfs_node_t* file_node = 0;
    for (int i = 0; i < pingfs_current_dir->child_count; i++) {
        if (strcmp(pingfs_current_dir->children[i]->name, name) == 0 &&
            pingfs_current_dir->children[i]->type == PINGFS_FILE) {
            file_node = pingfs_current_dir->children[i];
            break;
        }
    }

    if (!file_node) return -1; 

    for (int i = 0; i < PINGFS_MAX_BLOCKS; i++) {
        if (file_node->data_blocks[i]) {
            kfree(file_node->data_blocks[i]);
            file_node->data_blocks[i] = 0;
        }
    }

    int text_len = strlen(text);
    int needed_blocks = (text_len / PINGFS_BLOCK_SIZE) + 1;
    if (needed_blocks > PINGFS_MAX_BLOCKS) needed_blocks = PINGFS_MAX_BLOCKS;

    for (int b = 0; b < needed_blocks; b++) {
        file_node->data_blocks[b] = (char*)kcalloc(PINGFS_BLOCK_SIZE, 1);
        if (!file_node->data_blocks[b]) return -2; 

        int bytes_to_copy = text_len - (b * PINGFS_BLOCK_SIZE);
        if (bytes_to_copy > PINGFS_BLOCK_SIZE) bytes_to_copy = PINGFS_BLOCK_SIZE;

        memcpy(file_node->data_blocks[b], text + (b * PINGFS_BLOCK_SIZE), bytes_to_copy);
    }

    file_node->size = text_len;
    return 0;
}

char* pingfs_read_file(const char* name) {
    pingfs_node_t* file_node = 0;
    for (int i = 0; i < pingfs_current_dir->child_count; i++) {
        if (strcmp(pingfs_current_dir->children[i]->name, name) == 0 &&
            pingfs_current_dir->children[i]->type == PINGFS_FILE) {
            file_node = pingfs_current_dir->children[i];
            break;
        }
    }

    if (!file_node) return 0; 

    char* buffer = (char*)kcalloc(file_node->size + 1, 1);
    if (!buffer) return 0;

    int text_copied = 0;
    for (int b = 0; b < PINGFS_MAX_BLOCKS; b++) {
        if (file_node->data_blocks[b] && text_copied < file_node->size) {
            int bytes_to_copy = file_node->size - text_copied;
            if (bytes_to_copy > PINGFS_BLOCK_SIZE) bytes_to_copy = PINGFS_BLOCK_SIZE;
            
            memcpy(buffer + text_copied, file_node->data_blocks[b], bytes_to_copy);
            text_copied += bytes_to_copy;
        }
    }
    buffer[file_node->size] = '\0';
    return buffer;
}

int pingfs_remove(const char* name) {
    int found_index = -1;
    for (int i = 0; i < pingfs_current_dir->child_count; i++) {
        if (strcmp(pingfs_current_dir->children[i]->name, name) == 0) {
            found_index = i;
            break;
        }
    }

    if (found_index == -1) return -1; 

    pingfs_node_t* target = pingfs_current_dir->children[found_index];

    if (target->type == PINGFS_DIRECTORY && target->child_count > 0) {
        return -2; 
    }

    if (target->type == PINGFS_FILE) {
        for (int b = 0; b < PINGFS_MAX_BLOCKS; b++) {
            if (target->data_blocks[b]) {
                kfree(target->data_blocks[b]);
                target->data_blocks[b] = 0;
            }
        }
    }

    target->in_use = 0;

    for (int i = found_index; i < pingfs_current_dir->child_count - 1; i++) {
        pingfs_current_dir->children[i] = pingfs_current_dir->children[i + 1];
    }
    pingfs_current_dir->children[--pingfs_current_dir->child_count] = 0;

    return 0;
}

int pingfs_cd(const char* name) {
    if (strcmp(name, "..") == 0) {
        pingfs_current_dir = pingfs_current_dir->parent;
        return 0;
    }
    if (strcmp(name, "/") == 0) {
        pingfs_current_dir = pingfs_root;
        return 0;
    }

    for (int i = 0; i < pingfs_current_dir->child_count; i++) {
        if (strcmp(pingfs_current_dir->children[i]->name, name) == 0 &&
            pingfs_current_dir->children[i]->type == PINGFS_DIRECTORY) {
            pingfs_current_dir = pingfs_current_dir->children[i];
            return 0;
        }
    }
    return -1; 
}

void get_current_working_directory_path(char* path) {
    if (pingfs_current_dir == pingfs_root) {
        strcpy(path, "/");
        return;
    }

    char temp[128];
    temp[0] = '\0';
    pingfs_node_t* curr = pingfs_current_dir;
    
    while (curr != pingfs_root) {
        char element[64];
        strcpy(element, "/");
        strcat(element, curr->name);
        strcat(element, temp);
        strcpy(temp, element);
        curr = curr->parent;
    }
    strcpy(path, temp);
}

/* ------------------------------------------------------------------------------
 * SECTION 8: PCI Bus Scanner Diagnostics
 * ------------------------------------------------------------------------------
 */

typedef struct pci_device {
    unsigned char bus;
    unsigned char slot;
    unsigned char func;
    unsigned short vendor_id;
    unsigned short device_id;
    unsigned char class_id;
    unsigned char subclass_id;
} pci_device_t;

static pci_device_t pci_device_list[64];
static int          pci_devices_detected_count = 0;

unsigned short pci_config_read_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address;
    unsigned int lbus  = (unsigned int)bus;
    unsigned int lslot = (unsigned int)slot;
    unsigned int lfunc = (unsigned int)func;
    unsigned short tmp = 0;

    address = (unsigned int)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));

    outl_port(0xCF8, address);
    tmp = (unsigned short)((inl_port(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}

void scan_pci_bus() {
    pci_devices_detected_count = 0;
    memset(pci_device_list, 0, sizeof(pci_device_t) * 64);

    for (unsigned int bus = 0; bus < 8; bus++) {
        for (unsigned int slot = 0; slot < 32; slot++) {
            for (unsigned int func = 0; func < 8; func++) {
                unsigned short vendor = pci_config_read_word(bus, slot, func, 0);
                if (vendor != 0xFFFF) { 
                    unsigned short device = pci_config_read_word(bus, slot, func, 2);
                    unsigned short class_word = pci_config_read_word(bus, slot, func, 10);
                    
                    if (pci_devices_detected_count < 64) {
                        pci_device_list[pci_devices_detected_count].bus = bus;
                        pci_device_list[pci_devices_detected_count].slot = slot;
                        pci_device_list[pci_devices_detected_count].func = func;
                        pci_device_list[pci_devices_detected_count].vendor_id = vendor;
                        pci_device_list[pci_devices_detected_count].device_id = device;
                        pci_device_list[pci_devices_detected_count].class_id = (unsigned char)((class_word >> 8) & 0xFF);
                        pci_device_list[pci_devices_detected_count].subclass_id = (unsigned char)(class_word & 0xFF);
                        
                        pci_devices_detected_count++;
                    }
                }
            }
        }
    }
}

const char* resolve_pci_vendor(unsigned short vendor) {
    switch (vendor) {
        case 0x8086: return "Intel Corp.";
        case 0x10EC: return "Realtek Semiconductor";
        case 0x10DE: return "NVIDIA Corporation";
        case 0x1002: return "AMD / ATI Technologies";
        case 0x15AD: return "VMware Inc. (Virtual Device)";
        case 0x1AF4: return "VirtIO Adapter Virtual Platform";
        case 0x1013: return "Cirrus Logic Hardware";
        case 0x1234: return "Bochs/QEMU VGA Card";
        default:     return "Unknown Vendor Platform";
    }
}

const char* resolve_pci_class(unsigned char class_id) {
    switch (class_id) {
        case 0x00: return "Legacy Devices";
        case 0x01: return "Storage Controller (IDE/SATA)";
        case 0x02: return "Network Interface Controller";
        case 0x03: return "Display/VGA Video Card";
        case 0x04: return "Multimedia Controller (Audio/DSP)";
        case 0x05: return "Memory System Controller";
        case 0x06: return "System Bus Bridge Core";
        case 0x07: return "Simple Communication Controller";
        case 0x0C: return "Serial Bus Controller (USB/Firewire)";
        default:   
            return "General System Core Controller";
    }
}

/* ------------------------------------------------------------------------------
 * SECTION 8.5: Modern GPU Probing Pipeline
 * ------------------------------------------------------------------------------
 */

void probe_all_gpus() {
    vga_print_string("GPU Core: Scanning motherboard PCI lines and probing drivers...\n", 0x0F);
    
    // Probe all standard graphics hardware initializers
    if (intel_gpu_init) intel_gpu_init();
    if (amdgpu_init) amdgpu_init();
    if (i915_init) i915_init();
    if (xe_init) xe_init();
    if (nouveau_init) nouveau_init();
    if (radeon_init) radeon_init();
    if (virtio_gpu_init) virtio_gpu_init();
    if (mgag200_init) mgag200_init();
    if (vboxvideo_init) vboxvideo_init();
    if (panfrost_init) panfrost_init();
    if (lima_init) lima_init();
    
    // Set up core DRM Device structures
    if (drm_core_init) drm_core_init();
}

/* ------------------------------------------------------------------------------
 * SECTION 8.6: Wireless Network & Bluetooth Probing Pipeline
 * ------------------------------------------------------------------------------
 */

void probe_all_wireless() {
    vga_print_string("Network Core: Probing WiFi and Bluetooth RF controllers...\n", 0x0F);
    
    if (iwlwifi_init) iwlwifi_init();
    if (ath9k_init) ath9k_init();
    if (ath10k_init) ath10k_init();
    if (ath11k_init) ath11k_init();
    if (rtw88_init) rtw88_init();
    if (rtl8187_init) rtl8187_init();
    if (mt76_init) mt76_init();
    if (brcmfmac_init) brcmfmac_init();
    if (b43_init) b43_init();
    if (btusb_init) btusb_init();
}

/* ------------------------------------------------------------------------------
 * SECTION 8.7: Wired Ethernet Network Probing Pipeline
 * ------------------------------------------------------------------------------
 */

void probe_all_ethernet() {
    vga_print_string("Ethernet Core: Probing wired network adapters...\n", 0x0F);
    
    if (e1000e_init) e1000e_init();
    if (igb_init) igb_init();
    if (igxbe_init) igxbe_init();
    if (i40e_init) i40e_init();
    if (r8169_init) r8169_init();
    if (tg3_init) tg3_init();
    if (forcedeth_init) forcedeth_init();
    if (sky2_init) sky2_init();
    if (alx_init) alx_init();
    if (virtio_net_init) virtio_net_init();
}

/* ------------------------------------------------------------------------------
 * SECTION 9: VGA Text Mode Windowing Engine (TUI)
 * ------------------------------------------------------------------------------
 */

void tui_draw_window(int start_x, int start_y, int width, int height, const char* title, char color) {
    unsigned short* vga_mem = (unsigned short*)VGA_ADDRESS;

    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    if (start_x + width > VGA_COLS) width = VGA_COLS - start_x;
    if (start_y + height > VGA_ROWS) height = VGA_ROWS - start_y;

    for (int y = start_y + 1; y < start_y + height - 1; y++) {
        for (int x = start_x + 1; x < start_x + width - 1; x++) {
            vga_mem[y * VGA_COLS + x] = (color << 8) | ' ';
        }
    }

    char horizontal_line = 205; 
    char vertical_line   = 186; 
    char top_left_corner = 201;
    char top_right_corner = 187;
    char bottom_left_corner = 200;
    char bottom_right_corner = 188;

    for (int x = start_x + 1; x < start_x + width - 1; x++) {
        vga_mem[start_y * VGA_COLS + x] = (color << 8) | horizontal_line;
        vga_mem[(start_y + height - 1) * VGA_COLS + x] = (color << 8) | horizontal_line;
    }

    for (int y = start_y + 1; y < start_y + height - 1; y++) {
        vga_mem[y * VGA_COLS + start_x] = (color << 8) | vertical_line;
        vga_mem[y * VGA_COLS + (start_x + width - 1)] = (color << 8) | vertical_line;
    }

    vga_mem[start_y * VGA_COLS + start_x] = (color << 8) | top_left_corner;
    vga_mem[start_y * VGA_COLS + (start_x + width - 1)] = (color << 8) | top_right_corner;
    vga_mem[(start_y + height - 1) * VGA_COLS + start_x] = (color << 8) | bottom_left_corner;
    vga_mem[(start_y + height - 1) * VGA_COLS + (start_x + width - 1)] = (color << 8) | bottom_right_corner;

    int title_len = strlen(title);
    if (title_len > width - 4) title_len = width - 4;
    int title_start = start_x + (width - title_len) / 2;

    for (int i = 0; i < title_len; i++) {
        vga_mem[start_y * VGA_COLS + title_start + i] = ((color | 0x08) << 8) | title[i];
    }
}

void tui_draw_status_bar(const char* left_text, const char* right_text, char color) {
    unsigned short* vga_mem = (unsigned short*)VGA_ADDRESS;
    int target_row = VGA_ROWS - 1;

    for (int x = 0; x < VGA_COLS; x++) {
        vga_mem[target_row * VGA_COLS + x] = (color << 8) | ' ';
    }

    int left_len = strlen(left_text);
    for (int i = 0; i < left_len && i < 40; i++) {
        vga_mem[target_row * VGA_COLS + 2 + i] = (color << 8) | left_text[i];
    }

    int right_len = strlen(right_text);
    int right_start = VGA_COLS - right_len - 2;
    if (right_start > 41) {
        for (int i = 0; i < right_len; i++) {
            vga_mem[target_row * VGA_COLS + right_start + i] = (color << 8) | right_text[i];
        }
    }
}

/* ------------------------------------------------------------------------------
 * SECTION 10: CMOS Real-Time Clock & Time Utilities
 * ------------------------------------------------------------------------------
 */

unsigned char read_cmos_register(int reg) {
    outb_port(0x70, reg);
    return inb_port(0x71);
}

int is_cmos_update_in_progress() {
    outb_port(0x70, 0x0A);
    return (inb_port(0x71) & 0x80);
}

unsigned char get_cmos_value(int reg) {
    while (is_cmos_update_in_progress());
    return read_cmos_register(reg);
}

void print_system_date_time() {
    unsigned char second = get_cmos_value(0x00);
    unsigned char minute = get_cmos_value(0x02);
    unsigned char hour   = get_cmos_value(0x04);
    unsigned char day    = get_cmos_value(0x07);
    unsigned char month  = get_cmos_value(0x08);
    unsigned char year   = get_cmos_value(0x09);
    unsigned char century = 0x20; 

    unsigned char b_century = get_cmos_value(0x32);
    if (b_century != 0) {
        century = b_century;
    }

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

    if (!(registerB & 0x02) && (hour & 0x80)) {
        hour = ((hour & 0x7F) + 12) % 24;
    }

    char s_sec[3], s_min[3], s_hr[3], s_day[3], s_mth[3], s_yr[5];
    itoa(second, s_sec, 2);
    itoa(minute, s_min, 2);
    itoa(hour, s_hr, 2);
    itoa(day, s_day, 2);
    itoa(month, s_mth, 2);
    
    int full_year = (century * 100) + year;
    itoa(full_year, s_yr, 4);

    vga_print_string("System Clock Date: ", current_text_color);
    vga_print_string(s_yr, current_prompt_color);
    vga_print_string("-", current_text_color);
    vga_print_string(s_mth, current_prompt_color);
    vga_print_string("-", current_text_color);
    vga_print_string(s_day, current_prompt_color);
    vga_print_string("  Time: ", current_text_color);
    vga_print_string(s_hr, current_prompt_color);
    vga_print_string(":", current_text_color);
    vga_print_string(s_min, current_prompt_color);
    vga_print_string(":", current_text_color);
    vga_print_string(s_sec, current_prompt_color);
    vga_print_string(" UTC\n", current_text_color);
}

/* ------------------------------------------------------------------------------
 * SECTION 11: CPU Microarchitecture Diagnostics (CPUID & Registers)
 * ------------------------------------------------------------------------------
 */

static inline void cpuid_get_info(int code, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx) {
    __asm__ volatile("cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(code));
}

void print_cpu_info() {
    unsigned int eax, ebx, ecx, edx;
    char vendor_str[13];

    cpuid_get_info(0, &eax, &ebx, &ecx, &edx);
    *((unsigned int*)&vendor_str[0]) = ebx;
    *((unsigned int*)&vendor_str[4]) = edx;
    *((unsigned int*)&vendor_str[8]) = ecx;
    vendor_str[12] = '\0';

    vga_print_string("CPU Model Brand Vendor: ", current_text_color);
    vga_print_string(vendor_str, current_prompt_color);
    vga_print_string("\n", current_text_color);

    cpuid_get_info(1, &eax, &ebx, &ecx, &edx);

    vga_print_string("Processor Diagnostics: ", current_text_color);
    if (edx & (1 << 0))  vga_print_string("[FPU] ", current_prompt_color);
    if (edx & (1 << 4))  vga_print_string("[TSC] ", current_prompt_color);
    if (edx & (1 << 23)) vga_print_string("[MMX] ", current_prompt_color);
    if (edx & (1 << 25)) vga_print_string("[SSE] ", current_prompt_color);
    if (edx & (1 << 26)) vga_print_string("[SSE2] ", current_prompt_color);
    if (ecx & (1 << 0))  vga_print_string("[SSE3] ", current_prompt_color);
    vga_print_string("\n", current_text_color);
}

void print_architectural_registers() {
    unsigned int cr0, cr2, cr3, esp, ebp;

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

    vga_print_string("CR0 Control Flags       : 0x", current_text_color);
    vga_print_string(s_cr0, current_prompt_color);
    vga_print_string("\nCR2 Page Fault Address  : 0x", current_text_color);
    vga_print_string(s_cr2, current_prompt_color);
    vga_print_string("\nCR3 Page Directory Base : 0x", current_text_color);
    vga_print_string(s_cr3, current_prompt_color);
    vga_print_string("\nESP Current Stack Pointer: 0x", current_text_color);
    vga_print_string(s_esp, current_prompt_color);
    vga_print_string("\nEBP Frame Base Pointer  : 0x", current_text_color);
    vga_print_string(s_ebp, current_prompt_color);
    vga_print_string("\n", current_text_color);
}

/* ------------------------------------------------------------------------------
 * SECTION 12: Persistent Notebook Cache Subsystem (RAM Notepad)
 * ------------------------------------------------------------------------------
 */

#define NOTE_MAX_SLOTS 6
#define NOTE_LENGTH    80

static char note_database[NOTE_MAX_SLOTS][NOTE_LENGTH];
static int  note_active_slots[NOTE_MAX_SLOTS];

void clear_notebook() {
    for (int i = 0; i < NOTE_MAX_SLOTS; i++) {
        note_database[i][0] = '\0';
        note_active_slots[i] = 0;
    }
}

void write_note_slot(int slot, const char* content) {
    if (slot >= 0 && slot < NOTE_MAX_SLOTS) {
        strncpy(note_database[slot], content, NOTE_LENGTH - 1);
        note_database[slot][NOTE_LENGTH - 1] = '\0';
        note_active_slots[slot] = 1;
    }
}

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
        vga_print_string("System memory notepad cache is currently empty.\nUse 'note-add' to store simple text log strings.\n", current_text_color);
    }
}

/* ------------------------------------------------------------------------------
 * SECTION 13: Arithmetic Engines & Computational Shell Core
 * ------------------------------------------------------------------------------
 */

int compute_factorial(int val) {
    if (val < 0) return 0;
    if (val == 0 || val == 1) return 1;
    int result = 1;
    for (int i = 2; i <= val; i++) {
        result *= i;
    }
    return result;
}

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

void execute_calculator_logic(const char* args) {
    char s_num1[16];
    char s_num2[16];
    char op = ' ';
    int i = 0, j = 0;

    while (args[i] == ' ' || args[i] == '\t') i++;
    while (args[i] >= '0' && args[i] <= '9') {
        if (j < 15) s_num1[j++] = args[i];
        i++;
    }
    s_num1[j] = '\0';

    while (args[i] == ' ' || args[i] == '\t') i++;
    if (args[i] == '+' || args[i] == '-' || args[i] == '*' || args[i] == '/' || args[i] == '!' || args[i] == '^' || args[i] == 'v') {
        op = args[i];
        i++;
    } else {
        vga_print_string("Calculator Instructions: calc <operand> <operator> <operand>\n", current_text_color);
        vga_print_string("Supported operators: + (add), - (sub), * (mul), / (div), ! (fact), ^ (pow), v (sqrt)\n", current_text_color);
        return;
    }

    if (op == '!') {
        int val = atoi(s_num1);
        int res = compute_factorial(val);
        char s_res[24];
        itoa(res, s_res, 1);
        vga_print_string("Factorial Result: ", current_text_color);
        vga_print_string(s_res, current_prompt_color);
        vga_print_string("\n", current_text_color);
        return;
    }
    if (op == 'v') {
        int val = atoi(s_num1);
        double res = sqrt((double)val);
        int whole_part = (int)res;
        int fractional_part = (int)((res - whole_part) * 1000);
        
        char s_whole[16], s_frac[16];
        itoa(whole_part, s_whole, 1);
        itoa(abs(fractional_part), s_frac, 3);
        
        vga_print_string("Square Root Result: ", current_text_color);
        vga_print_string(s_whole, current_prompt_color);
        vga_print_string(".", current_prompt_color);
        vga_print_string(s_frac, current_prompt_color);
        vga_print_string("\n", current_text_color);
        return;
    }

    j = 0;
    while (args[i] == ' ' || args[i] == '\t') i++;
    while (args[i] >= '0' && args[i] <= '9') {
        if (j < 15) s_num2[j++] = args[i];
        i++;
    }
    s_num2[j] = '\0';

    if (strlen(s_num1) == 0 || (op != '!' && op != 'v' && strlen(s_num2) == 0)) {
        vga_print_string("Command Syntax Error. Format example: calc 15 * 6\n", COLOR_RED_ON_BLACK);
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
            vga_print_string("Math Exception Blocked: Zero division attempt.\n", COLOR_RED_ON_BLACK);
            return;
        }
        output = n1 / n2;
    } else if (op == '^') {
        output = (int)pow((double)n1, n2);
    }

    char s_output[24];
    itoa(output, s_output, 1);
    vga_print_string("Arithmetic Result: ", current_text_color);
    vga_print_string(s_output, current_prompt_color);
    vga_print_string("\n", current_text_color);
}

/* ------------------------------------------------------------------------------
 * SECTION 14: Execution Shell History Tracker
 * ------------------------------------------------------------------------------
 */

#define MAX_HISTORY_SLOTS 12
static char shell_history[MAX_HISTORY_SLOTS][64];
static int  shell_history_count = 0;

void push_history(const char* cmd) {
    if (strlen(cmd) == 0) return;

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

void display_history() {
    vga_print_string("Execution Shell Command History:\n", current_prompt_color);
    for (int i = 0; i < shell_history_count; i++) {
        char idx_str[4];
        itoa(i + 1, idx_str, 1);
        vga_print_string("  [", current_text_color);
        vga_print_string(idx_str, current_text_color);
        vga_print_string("]: ", current_text_color);
        vga_print_string(shell_history[i], current_text_color);
        vga_print_string("\n", current_text_color);
    }
}

/* ------------------------------------------------------------------------------
 * SECTION 15: Memory Hexadecimal Viewer Diagnostic Tool
 * ------------------------------------------------------------------------------
 */

void execute_hex_dump(const char* args) {
    char s_addr[32];
    int idx = 0;
    
    while (args[idx] == ' ' || args[idx] == '\t') idx++;
    
    int s_idx = 0;
    while (args[idx] != ' ' && args[idx] != '\t' && args[idx] != '\0') {
        if (s_idx < 31) {
            s_addr[s_idx++] = args[idx];
        }
        idx++;
    }
    s_addr[s_idx] = '\0';

    if (strlen(s_addr) == 0) {
        vga_print_string("Syntax: hexview <address_in_hex>\nExample: hexview B8000\n", current_text_color);
        return;
    }

    unsigned int address = 0;
    for (int i = 0; s_addr[i] != '\0'; i++) {
        char c = s_addr[i];
        unsigned int val = 0;
        if (c >= '0' && c <= '9')      val = c - '0';
        else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
        else {
            vga_print_string("Address contains invalid hexadecimal characters.\n", COLOR_RED_ON_BLACK);
            return;
        }
        address = (address * 16) + val;
    }

    unsigned char* ptr = (unsigned char*)address;
    
    vga_print_string("Memory Map Dump of: 0x", current_text_color);
    char out_addr[16];
    htoa(address, out_addr);
    vga_print_string(out_addr, current_prompt_color);
    vga_print_string("\n", current_text_color);

    for (int row = 0; row < 8; row++) {
        unsigned int row_addr = address + (row * 8);
        char row_addr_hex[16];
        htoa(row_addr, row_addr_hex);
        
        vga_print_string(row_addr_hex, current_prompt_color);
        vga_print_string(" | ", current_text_color);

        for (int col = 0; col < 8; col++) {
            unsigned char b = ptr[row * 8 + col];
            char val_hex[4];
            htoa(b, val_hex);
            
            if (b < 16) {
                vga_print_string("0", current_text_color);
            }
            vga_print_string(val_hex, current_text_color);
            vga_print_string(" ", current_text_color);
        }

        vga_print_string(" | ", current_text_color);

        for (int col = 0; col < 8; col++) {
            unsigned char b = ptr[row * 8 + col];
            if (b >= 32 && b <= 126) {
                char ch_s[2] = { (char)b, '\0' };
                vga_print_string(ch_s, current_text_color);
            } else {
                vga_print_string(".", current_text_color);
            }
        }
        vga_print_string("\n", current_text_color);
    }
}

/* ------------------------------------------------------------------------------
 * SECTION 16: Low-Level Motherboard Routines (Reboot & Power Management)
 * ------------------------------------------------------------------------------
 */

void sys_reboot() {
    vga_print_string("Sending system restart signals to physical motherboard ports...\n", current_prompt_color);

    unsigned char temp;
    do {
        temp = inb_port(0x64);
        if (temp & 1) {
            inb_port(0x60); 
        }
    } while (temp & 2);

    outb_port(0x64, 0xFE); 

    while (1) {
        __asm__ volatile("hlt");
    }
}

void sys_halt() {
    vga_clear_screen();
    vga_print_string("\n\n\n\n", current_text_color);
    vga_print_string("             ==================================================\n", current_prompt_color);
    vga_print_string("                        PINGOS ACPI SHUTDOWN SIGNALED           \n", current_prompt_color);
    vga_print_string("                        Your personal files are secured.        \n", current_prompt_color);
    vga_print_string("                        It is now safe to disconnect power.     \n", current_text_color);
    vga_print_string("             ==================================================\n", current_prompt_color);

    __asm__ volatile("cli");
    while (1) {
        __asm__ volatile("hlt");
    }
}

/* ------------------------------------------------------------------------------
 * SECTION 17: Interactive Shell Gaming Modules
 * ------------------------------------------------------------------------------
 */

void play_kernel_quest() {
    vga_clear_screen();
    vga_print_string("================================================================================\n", COLOR_CYAN_ON_BLACK);
    vga_print_string("                 QUEST FOR THE KERNEL - Retro Maze Game v1.0                    \n", COLOR_CYAN_ON_BLACK);
    vga_print_string("================================================================================\n", COLOR_CYAN_ON_BLACK);
    vga_print_string("Instructions: Navigate the safe forest using 'W', 'A', 'S', 'D' keys.\n", COLOR_GREY_ON_BLACK);
    vga_print_string("Collect lost Memory Pages (P) to restore system boot parameters.\n", COLOR_GREY_ON_BLACK);
    vga_print_string("Avoid the swamp mud barriers (#). Clear all Pages to successfully exit.\n\n", COLOR_GREY_ON_BLACK);
    vga_print_string("  -- Press any key to begin the execution thread --", COLOR_WHITE_ON_BLACK);
    wait_for_key();

    #define MAZE_W 16
    #define MAZE_H 10
    
    char maze[MAZE_H][MAZE_W] = {
        {'#','#','#','#','#','#','#','#','#','#','#','#','#','#','#','#'},
        {'#','P',' ',' ','#',' ',' ',' ',' ',' ',' ','#',' ',' ','P','#'},
        {'#',' ','#',' ','#',' ','#','#','#','#',' ','#',' ','#','#','#'},
        {'#',' ','#',' ',' ',' ','#',' ',' ','#',' ',' ',' ',' ',' ','#'},
        {'#',' ','#','#','#',' ','#','P',' ','#','#','#','#','#',' ','#'},
        {'#',' ',' ',' ','#',' ','#','#',' ','#',' ',' ',' ','#',' ','#'},
        {'#','#','#',' ','#',' ',' ',' ',' ','#',' ','#',' ','#',' ','#'},
        {'#','P',' ',' ','#','#','#','#',' ','#',' ','#',' ','#','P','#'},
        {'#',' ','#',' ',' ',' ',' ','#',' ',' ',' ','#',' ',' ',' ','#'},
        {'#','#','#','#','#','#','#','#','#','#','#','#','#','#','#','#'}
    };

    int player_x = 1;
    int player_y = 1;
    int pages_collected = 0;
    int total_pages = 5;

    while (1) {
        vga_clear_screen();
        vga_print_string("Quest Tracker: Collected [", COLOR_CYAN_ON_BLACK);
        char s_col[12], s_tot[12];
        itoa(pages_collected, s_col, 1);
        itoa(total_pages, s_tot, 1);
        vga_print_string(s_col, COLOR_WHITE_ON_BLACK);
        vga_print_string("/", COLOR_CYAN_ON_BLACK);
        vga_print_string(s_tot, COLOR_WHITE_ON_BLACK);
        vga_print_string("] Memory Pages\n\n", COLOR_CYAN_ON_BLACK);

        for (int y = 0; y < MAZE_H; y++) {
            for (int x = 0; x < MAZE_W; x++) {
                if (x == player_x && y == player_y) {
                    vga_print_string("@ ", COLOR_WHITE_ON_BLACK); 
                } else if (maze[y][x] == '#') {
                    vga_print_string("# ", COLOR_GREEN_ON_BLACK); 
                } else if (maze[y][x] == 'P') {
                    vga_print_string("P ", COLOR_AMBER_ON_BLACK); 
                } else {
                    vga_print_string(". ", COLOR_GREY_ON_BLACK); 
                }
            }
            vga_print_string("\n", COLOR_GREY_ON_BLACK);
        }

        vga_print_string("\nControls: W (Up) | A (Left) | S (Down) | D (Right) | Q (Quit Game)\n", COLOR_GREY_ON_BLACK);

        if (pages_collected == total_pages) {
            vga_print_string("\nCONGRATULATIONS! You restored the system database memory segments!\n", COLOR_GREEN_ON_BLACK);
            vga_print_string("Press any key to jump back to terminal shell.\n", COLOR_WHITE_ON_BLACK);
            wait_for_key();
            break;
        }

        char input = wait_for_key();
        if (input == 'q' || input == 'Q') {
            break;
        }

        int next_x = player_x;
        int next_y = player_y;

        if (input == 'w' || input == 'W') next_y--;
        if (input == 's' || input == 'S') next_y++;
        if (input == 'a' || input == 'A') next_x--;
        if (input == 'd' || input == 'D') next_x++;

        if (next_x >= 0 && next_x < MAZE_W && next_y >= 0 && next_y < MAZE_H) {
            if (maze[next_y][next_x] != '#') {
                player_x = next_x;
                player_y = next_y;
                if (maze[player_y][player_x] == 'P') {
                    pages_collected++;
                    maze[player_y][player_x] = ' '; 
                }
            }
        }
    }
    vga_clear_screen();
}

void play_snake_game() {
    vga_clear_screen();
    vga_print_string("================================================================================\n", COLOR_AMBER_ON_BLACK);
    vga_print_string("                     RETRO SNAKE GAME MODULE - PingOS                           \n", COLOR_AMBER_ON_BLACK);
    vga_print_string("================================================================================\n", COLOR_AMBER_ON_BLACK);
    vga_print_string("Objective: Command the digital snake segment using W, A, S, D.\n", COLOR_GREY_ON_BLACK);
    vga_print_string("Feed on memory blocks (*) to expand the data array registers.\n", COLOR_GREY_ON_BLACK);
    vga_print_string("Avoid hitting the perimeter firewall barriers (#) or your own tail!\n\n", COLOR_GREY_ON_BLACK);
    vga_print_string("  -- Press any key to begin the game loop --", COLOR_WHITE_ON_BLACK);
    wait_for_key();

    #define BOARD_W 20
    #define BOARD_H 12
    #define SNAKE_MAX_LENGTH 100

    int snake_x[SNAKE_MAX_LENGTH];
    int snake_y[SNAKE_MAX_LENGTH];
    int snake_len = 3;
    
    snake_x[0] = 5; snake_y[0] = 5;
    snake_x[1] = 4; snake_y[1] = 5;
    snake_x[2] = 3; snake_y[2] = 5;

    int dir_x = 1;
    int dir_y = 0;

    int apple_x = 10;
    int apple_y = 5;
    int score = 0;
    int game_running = 1;

    while (game_running) {
        vga_clear_screen();
        vga_print_string("Snake Score Metric: [", COLOR_AMBER_ON_BLACK);
        char s_score[12];
        itoa(score, s_score, 1);
        vga_print_string(s_score, COLOR_WHITE_ON_BLACK);
        vga_print_string("] points\n\n", COLOR_AMBER_ON_BLACK);

        for (int x = 0; x < BOARD_W + 2; x++) vga_print_string("#", COLOR_AMBER_ON_BLACK);
        vga_print_string("\n", COLOR_GREY_ON_BLACK);

        for (int y = 0; y < BOARD_H; y++) {
            vga_print_string("#", COLOR_AMBER_ON_BLACK); 
            for (int x = 0; x < BOARD_W; x++) {
                int is_snake = 0;
                for (int i = 0; i < snake_len; i++) {
                    if (snake_x[i] == x && snake_y[i] == y) {
                        is_snake = 1;
                        break;
                    }
                }

                if (is_snake) {
                    vga_print_string("O", COLOR_GREEN_ON_BLACK);
                } else if (x == apple_x && y == apple_y) {
                    vga_print_string("*", COLOR_RED_ON_BLACK);
                } else {
                    vga_print_string(" ", COLOR_GREY_ON_BLACK);
                }
            }
            vga_print_string("#\n", COLOR_AMBER_ON_BLACK); 
        }

        for (int x = 0; x < BOARD_W + 2; x++) vga_print_string("#", COLOR_AMBER_ON_BLACK);
        vga_print_string("\n", COLOR_GREY_ON_BLACK);

        vga_print_string("\nKeyboard Directions: W (Up) | S (Down) | A (Left) | D (Right) | Q (Exit)\n", COLOR_GREY_ON_BLACK);

        char input = wait_for_key();
        if (input == 'q' || input == 'Q') {
            game_running = 0;
            break;
        }

        if ((input == 'w' || input == 'W') && dir_y != 1) { dir_x = 0; dir_y = -1; }
        if ((input == 's' || input == 'S') && dir_y != -1) { dir_x = 0; dir_y = 1; }
        if ((input == 'a' || input == 'A') && dir_x != 1) { dir_x = -1; dir_y = 0; }
        if ((input == 'd' || input == 'D') && dir_x != -1) { dir_x = 1; dir_y = 0; }

        int next_head_x = snake_x[0] + dir_x;
        int next_head_y = snake_y[0] + dir_y;

        if (next_head_x < 0 || next_head_x >= BOARD_W || next_head_y < 0 || next_head_y >= BOARD_H) {
            vga_print_string("\nCRITICAL COLLISION: Firewall firewall barrier breached! Game Over.\n", COLOR_RED_ON_BLACK);
            vga_print_string("Press any key to exit.\n", COLOR_WHITE_ON_BLACK);
            wait_for_key();
            game_running = 0;
            break;
        }

        for (int i = 0; i < snake_len; i++) {
            if (snake_x[i] == next_head_x && snake_y[i] == next_head_y) {
                vga_print_string("\nCRITICAL COLLISION: Snake segment crossed its own tail! Game Over.\n", COLOR_RED_ON_BLACK);
                vga_print_string("Press any key to exit.\n", COLOR_WHITE_ON_BLACK);
                wait_for_key();
                game_running = 0;
                break;
            }
        }

        if (!game_running) break;

        for (int i = snake_len - 1; i > 0; i--) {
            snake_x[i] = snake_x[i - 1];
            snake_y[i] = snake_y[i - 1];
        }
        snake_x[0] = next_head_x;
        snake_y[0] = next_head_y;

        if (snake_x[0] == apple_x && snake_y[0] == apple_y) {
            score += 10;
            if (snake_len < SNAKE_MAX_LENGTH) {
                snake_len++;
            }
            apple_x = rand() % BOARD_W;
            apple_y = rand() % BOARD_H;
        }
    }
    vga_clear_screen();
}

/* ------------------------------------------------------------------------------
 * SECTION 18: Shell Parser and Interactive System Commands
 * ------------------------------------------------------------------------------
 */

static void print_to_coords(int x, int y, const char* str, char color) {
    unsigned short* mem = (unsigned short*)VGA_ADDRESS;
    for (int i = 0; str[i] != '\0'; i++) {
        mem[y * VGA_COLS + x + i] = (color << 8) | str[i];
    }
}

void parse_and_execute_command(char* cmd) {
    if (strlen(cmd) == 0) return;

    char* cmd_name = strtok(cmd, " ");
    char* args = strtok(0, ""); 

    if (strcmp(cmd_name, "help") == 0) {
        vga_print_string("Core OS Commands List:\n", current_prompt_color);
        vga_print_string("  help        - Show this documentation index\n", current_text_color);
        vga_print_string("  clear       - Clear screen and reset scroll cursors\n", current_text_color);
        vga_print_string("  info        - Display microarchitectural project details\n", current_text_color);
        vga_print_string("  uname       - System OS metadata release details\n", current_text_color);
        vga_print_string("  cpu         - Query system processor vendor/flags via CPUID\n", current_text_color);
        vga_print_string("  registers   - Print machine architectural values (CR0, CR3)\n", current_text_color);
        vga_print_string("  time        - Query real-time CMOS date and clock parameters\n", current_text_color);
        vga_print_string("  pciscan     - Scan Motherboard PCI bus routing channels\n", current_text_color);
        vga_print_string("  gpudetect   - Probe and load all legacy/modern graphics drivers\n", current_text_color);
        vga_print_string("  netdetect   - Probe and load all legacy/modern wireless & BT drivers\n", current_text_color);
        vga_print_string("  ethdetect   - Probe and load all legacy/modern wired Ethernet drivers\n", current_text_color);
        vga_print_string("  drminfo     - Dump registered DRM connector and CRTC structures\n", current_text_color);
        vga_print_string("  drmtest     - Set 800x600 32-bit ARGB mode & draw graphic layout\n", current_text_color);
        vga_print_string("  sysmon      - Load live visual status telemetry layout\n", current_text_color);
        vga_print_string("  heapinfo    - Query memory allocations diagnostic statistics\n", current_text_color);
        vga_print_string("  hexview     - Hexadecimal viewer utility. Syntax: hexview <addr>\n", current_text_color);
        vga_print_string("  calc        - Calculator parser engine. Syntax: calc <op1> <+,-,*> <op2>\n", current_text_color);
        vga_print_string("  history     - Display previously entered shell lines\n", current_text_color);
        vga_print_string("  theme       - Cycle shell colors dynamically\n", current_text_color);
        vga_print_string("  tasks       - List registered processes and threads\n", current_text_color);
        vga_print_string("  reboot      - Restart machine via keyboard output register\n", current_text_color);
        vga_print_string("  halt        - Safely power down operating system threads\n", current_text_color);
        
        vga_print_string("\nPingFS File System Commands:\n", current_prompt_color);
        vga_print_string("  ls          - Directory file listing\n", current_text_color);
        vga_print_string("  cd <dir>    - Shift workspace folder context\n", current_text_color);
        vga_print_string("  mkdir <name>- Create new subdirectory block\n", current_text_color);
        vga_print_string("  touch <file>- Create empty file block\n", current_text_color);
        vga_print_string("  write <f> <t>- Write text strings to file\n", current_text_color);
        vga_print_string("  cat <file>  - Output contents of file to screen\n", current_text_color);
        vga_print_string("  rm <name>   - Remove file or subdirectory\n", current_text_color);
        
        vga_print_string("\nBare-metal Terminal Games:\n", current_prompt_color);
        vga_print_string("  quest       - Play Quest for the Kernel Maze Adventure\n", current_text_color);
        vga_print_string("  snake       - Play Retro Snake Score Challenger\n", current_text_color);
    }
    else if (strcmp(cmd_name, "clear") == 0) {
        vga_clear_screen();
    }
    else if (strcmp(cmd_name, "uname") == 0) {
        if (args && strcmp(args, "-a") == 0) {
            vga_print_string("PingOS release-0.2.0-LTS-x86_32-freestanding (Bare-Metal ProtectedMode)\n", current_text_color);
        } else if (args && strcmp(args, "-r") == 0) {
            vga_print_string("0.2.0-LTS-freestanding\n", current_text_color);
        } else {
            vga_print_string("PingOS\n", current_text_color);
        }
    }
    else if (strcmp(cmd_name, "info") == 0) {
        vga_print_string("System Name    : PingOS Operating System\n", current_prompt_color);
        vga_print_string("Kernel Model   : PingKernel Freestanding 32-Bit C/ASM\n", current_text_color);
        vga_print_string("Address Space  : Fully executing at physical segment 0x100000\n", current_text_color);
        vga_print_string("GDT Layout     : Custom Flat-Memory segments implemented\n", current_text_color);
        vga_print_string("Memory Heap    : Doubly-Linked List Block Allocator\n", current_text_color);
        vga_print_string("Filesystem     : PingFS Virtual In-Memory filesystem layout\n", current_text_color);
    }
    else if (strcmp(cmd_name, "cpu") == 0) {
        print_cpu_info();
    }
    else if (strcmp(cmd_name, "registers") == 0) {
        print_architectural_registers();
    }
    else if (strcmp(cmd_name, "time") == 0) {
        print_system_date_time();
    }
    else if (strcmp(cmd_name, "gpudetect") == 0) {
        probe_all_gpus();
    }
    else if (strcmp(cmd_name, "netdetect") == 0) {
        probe_all_wireless();
    }
    else if (strcmp(cmd_name, "ethdetect") == 0) {
        probe_all_ethernet();
    }
    else if (strcmp(cmd_name, "drminfo") == 0) {
        if (drm_print_diagnostics) drm_print_diagnostics();
    }
    else if (strcmp(cmd_name, "drmtest") == 0) {
        vga_print_string("DRM: Setting up 800x600 32bpp graphics mode settings...\n", current_prompt_color);
        
        if (drm_set_mode && drm_set_mode(800, 600, 32) == 0) {
            // Draw gradient background
            for (unsigned int y = 0; y < 600; y++) {
                unsigned int r = (y * 48) / 600;
                unsigned int g = 0;
                unsigned int b = 64 - (y * 32) / 600;
                unsigned int gradient_color = (r << 16) | (g << 8) | b;
                for (unsigned int x = 0; x < 800; x++) {
                    if (drm_put_pixel) drm_put_pixel(x, y, gradient_color);
                }
            }

            // Draw graphic panels
            if (drm_draw_rect) {
                drm_draw_rect(60, 60, 680, 80, 0x001F3F5F);  // Slate header panel
                drm_draw_rect(60, 140, 680, 400, 0x000F1F2F); // Dark blue main body

                // Draw layout decorations and geometric colors
                drm_draw_rect(100, 200, 220, 160, 0x00D9534F); // High-contrast Red block
                drm_draw_rect(180, 260, 220, 160, 0x005CB85C); // Overlapping green block
            }

            // Draw sun rays using trigonometric approximations
            for (int angle = 0; angle < 360; angle += 15) {
                double rad = (double)angle * 3.14159265 / 180.0;
                int ray_x = (int)(cos_approx(rad) * 90.0);
                int ray_y = (int)(sin_approx(rad) * 90.0);
                if (drm_draw_line) drm_draw_line(560, 340, 560 + ray_x, 340 + ray_y, 0x00F0AD4E); // Warm Orange rays
            }

            // Border structures
            if (drm_draw_line) {
                drm_draw_line(60, 60, 740, 540, 0x00337AB7);
                drm_draw_line(740, 60, 60, 540, 0x00337AB7);
            }

            if (drm_page_flip) drm_page_flip();

            // Intercept standard hardware inputs
            wait_for_key();

            // Clear buffers and return to standard legacy text console
            if (drm_restore_vga_text_mode) drm_restore_vga_text_mode();
            vga_clear_screen();
            vga_print_string("DRM: Cleanly restored standard VGA 80x25 text mode terminal.\n", COLOR_GREEN_ON_BLACK);
        } else {
            vga_print_string("DRM BLAD: BGA device not located. High-res simulation unavailable.\n", COLOR_RED_ON_BLACK);
        }
    }
    else if (strcmp(cmd_name, "pciscan") == 0) {
        vga_print_string("Scanning physical system PCI lanes...\n", current_prompt_color);
        scan_pci_bus();
        
        char s_count[16];
        itoa(pci_devices_detected_count, s_count, 1);
        vga_print_string("Scan complete. Detected [", current_text_color);
        vga_print_string(s_count, current_prompt_color);
        vga_print_string("] PCI nodes on active motherboard registers:\n\n", current_text_color);

        for (int i = 0; i < pci_devices_detected_count; i++) {
            char s_bus[4], s_slot[4], s_func[4], s_ven[8], s_dev[8];
            itoa(pci_device_list[i].bus, s_bus, 1);
            itoa(pci_device_list[i].slot, s_slot, 1);
            itoa(pci_device_list[i].func, s_func, 1);
            htoa(pci_device_list[i].vendor_id, s_ven);
            htoa(pci_device_list[i].device_id, s_dev);

            vga_print_string(" PCI-Node ", current_text_color);
            vga_print_string(s_bus, current_prompt_color);
            vga_print_string(":", current_text_color);
            vga_print_string(s_slot, current_prompt_color);
            vga_print_string(".", current_text_color);
            vga_print_string(s_func, current_prompt_color);
            vga_print_string(" Vendor=0x", current_text_color);
            vga_print_string(s_ven, current_prompt_color);
            vga_print_string(" Device=0x", current_prompt_color);
            vga_print_string(s_dev, current_prompt_color);
            vga_print_string(" [", current_text_color);
            vga_print_string(resolve_pci_vendor(pci_device_list[i].vendor_id), current_prompt_color);
            vga_print_string("] Class: ", current_text_color);
            vga_print_string(resolve_pci_class(pci_device_list[i].class_id), current_prompt_color);
            vga_print_string("\n", current_text_color);
        }
    }
    else if (strcmp(cmd_name, "heapinfo") == 0) {
        int total, used, free_blks;
        get_heap_diagnostics(&total, &used, &free_blks);

        char s_tot[16], s_usd[16], s_fre[16], s_blks[16];
        itoa(total, s_tot, 1);
        itoa(used, s_usd, 1);
        itoa(total - used, s_fre, 1);
        itoa(free_blks, s_blks, 1);

        vga_print_string("Memory Allocation Heap System Telemetry:\n", current_prompt_color);
        vga_print_string("  Physical Pool Capacity : ", current_text_color);
        vga_print_string(s_tot, current_prompt_color);
        vga_print_string(" bytes\n  Active Allocations     : ", current_text_color);
        vga_print_string(s_usd, current_prompt_color);
        vga_print_string(" bytes\n  Available Segment Space: ", current_text_color);
        vga_print_string(s_fre, current_prompt_color);
        vga_print_string(" bytes\n  Available Fragmentation Blocks: ", current_text_color);
        vga_print_string(s_blks, current_prompt_color);
        vga_print_string("\n", current_text_color);
    }
    else if (strcmp(cmd_name, "hexview") == 0) {
        if (!args || strlen(args) == 0) {
            vga_print_string("Syntax: hexview <address_in_hex>\nExample: hexview B8000\n", current_text_color);
        } else {
            execute_hex_dump(args);
        }
    }
    else if (strcmp(cmd_name, "calc") == 0) {
        if (!args || strlen(args) == 0) {
            vga_print_string("Syntax: calc <number1> <operator> <number2>\nExample: calc 15 * 6\n", current_text_color);
        } else {
            execute_calculator_logic(args);
        }
    }
    else if (strcmp(cmd_name, "history") == 0) {
        display_history();
    }
    else if (strcmp(cmd_name, "theme") == 0) {
        if (current_prompt_color == COLOR_WHITE_ON_BLACK) {
            current_prompt_color = COLOR_GREEN_ON_BLACK;
            current_text_color   = COLOR_GREY_ON_BLACK;
            vga_print_string("Color Theme Swapped -> Emerald Green Terminal\n", current_prompt_color);
        } else if (current_prompt_color == COLOR_GREEN_ON_BLACK) {
            current_prompt_color = COLOR_BLUE_ON_BLACK;
            current_text_color   = COLOR_BLUE_ON_BLACK;
            vga_print_string("Color Theme Swapped -> Cyber Blue Mode\n", current_prompt_color);
        } else if (current_prompt_color == COLOR_BLUE_ON_BLACK) {
            current_prompt_color = COLOR_AMBER_ON_BLACK;
            current_text_color   = COLOR_AMBER_ON_BLACK;
            vga_print_string("Color Theme Swapped -> Retro Phosphor Amber\n", current_prompt_color);
        } else {
            current_prompt_color = COLOR_WHITE_ON_BLACK;
            current_text_color   = COLOR_GREY_ON_BLACK;
            vga_print_string("Color Theme Swapped -> Default Monochrome Layout\n", current_prompt_color);
        }
    }
    else if (strcmp(cmd_name, "tasks") == 0) {
        vga_print_string("Active Registered Scheduler Processes List:\n", current_prompt_color);
        for (int i = 0; i < total_registered_tasks; i++) {
            char s_id[4], s_ticks[12];
            itoa(task_queue[i]->id, s_id, 1);
            itoa(task_queue[i]->ticks_run, s_ticks, 1);
            
            vga_print_string("  ID: ", current_text_color);
            vga_print_string(s_id, current_prompt_color);
            vga_print_string(" | Process Label: ", current_text_color);
            vga_print_string(task_queue[i]->name, current_prompt_color);
            vga_print_string(" | Process Status: ", current_text_color);
            
            if (task_queue[i]->state == TASK_STATE_RUNNING) {
                vga_print_string("RUNNING  ", COLOR_GREEN_ON_BLACK);
            } else if (task_queue[i]->state == TASK_STATE_READY) {
                vga_print_string("READY    ", COLOR_CYAN_ON_BLACK);
            } else {
                vga_print_string("TERMINATED", COLOR_RED_ON_BLACK);
            }
            vga_print_string(" | Process Ticks: ", current_text_color);
            vga_print_string(s_ticks, current_prompt_color);
            vga_print_string("\n", current_text_color);
        }
    }
    else if (strcmp(cmd_name, "reboot") == 0) {
        sys_reboot();
    }
    else if (strcmp(cmd_name, "halt") == 0) {
        sys_halt();
    }
    
    else if (strcmp(cmd_name, "ls") == 0) {
        vga_print_string("Directory Content Inode Listing:\n", current_prompt_color);
        if (pingfs_current_dir->child_count == 0) {
            vga_print_string("  -- Directory is empty --\n", current_text_color);
        } else {
            for (int i = 0; i < pingfs_current_dir->child_count; i++) {
                pingfs_node_t* child = pingfs_current_dir->children[i];
                if (child->type == PINGFS_DIRECTORY) {
                    vga_print_string("  [DIR]  ", COLOR_CYAN_ON_BLACK);
                    vga_print_string(child->name, current_prompt_color);
                } else {
                    vga_print_string("  [FILE] ", COLOR_GREY_ON_BLACK);
                    vga_print_string(child->name, current_prompt_color);
                    vga_print_string("  (", current_text_color);
                    char s_sz[16];
                    itoa(child->size, s_sz, 1);
                    vga_print_string(s_sz, current_text_color);
                    vga_print_string(" bytes)", current_text_color);
                }
                vga_print_string("\n", current_text_color);
            }
        }
    }
    else if (strcmp(cmd_name, "mkdir") == 0) {
        if (!args || strlen(args) == 0) {
            vga_print_string("Syntax: mkdir <folder_name>\n", current_text_color);
        } else {
            int r = pingfs_mkdir(args);
            if (r == 0) {
                vga_print_string("Subfolder created successfully.\n", current_text_color);
            } else {
                vga_print_string("Operation Failed. Node conflict or directory capacity reached.\n", COLOR_RED_ON_BLACK);
            }
        }
    }
    else if (strcmp(cmd_name, "touch") == 0) {
        if (!args || strlen(args) == 0) {
            vga_print_string("Syntax: touch <file_name>\n", current_text_color);
        } else {
            int r = pingfs_create_file(args);
            if (r == 0) {
                vga_print_string("Empty file block allocated.\n", current_text_color);
            } else {
                vga_print_string("Operation Failed. File exists or database is full.\n", COLOR_RED_ON_BLACK);
            }
        }
    }
    else if (strcmp(cmd_name, "write") == 0) {
        if (!args || strlen(args) == 0) {
            vga_print_string("Syntax: write <file_name> <text_data_string>\n", current_text_color);
            return;
        }
        
        char* f_name = strtok(args, " ");
        char* f_content = strtok(0, "");

        if (!f_name || !f_content) {
            vga_print_string("Syntax: write <file_name> <text_data_string>\n", current_text_color);
        } else {
            int r = pingfs_write_file(f_name, f_content);
            if (r == 0) {
                vga_print_string("File block updated successfully.\n", current_text_color);
            } else {
                vga_print_string("Operation Failed. Target file node could not be located.\n", COLOR_RED_ON_BLACK);
            }
        }
    }
    else if (strcmp(cmd_name, "cat") == 0) {
        if (!args || strlen(args) == 0) {
            vga_print_string("Syntax: cat <file_name>\n", current_text_color);
        } else {
            char* data = pingfs_read_file(args);
            if (data) {
                vga_print_string(data, current_text_color);
                vga_print_string("\n", current_text_color);
                kfree(data);
            } else {
                vga_print_string("File could not be found or read failed.\n", COLOR_RED_ON_BLACK);
            }
        }
    }
    else if (strcmp(cmd_name, "rm") == 0) {
        if (!args || strlen(args) == 0) {
            vga_print_string("Syntax: rm <item_name>\n", current_text_color);
        } else {
            int r = pingfs_remove(args);
            if (r == 0) {
                vga_print_string("Node purged from virtual disk table.\n", current_text_color);
            } else if (r == -2) {
                vga_print_string("Error: Cannot delete directory with active sub-nodes.\n", COLOR_RED_ON_BLACK);
            } else {
                vga_print_string("Error: Node not found.\n", COLOR_RED_ON_BLACK);
            }
        }
    }
    else if (strcmp(cmd_name, "cd") == 0) {
        if (!args || strlen(args) == 0) {
            vga_print_string("Syntax: cd <directory_path>\n", current_text_color);
        } else {
            int r = pingfs_cd(args);
            if (r != 0) {
                vga_print_string("Error: Folder path not found.\n", COLOR_RED_ON_BLACK);
            }
        }
    }
    
    else if (strcmp(cmd_name, "quest") == 0) {
        play_kernel_quest();
    }
    else if (strcmp(cmd_name, "snake") == 0) {
        play_snake_game();
    }
    
    else if (strcmp(cmd_name, "sysmon") == 0) {
        vga_clear_screen();
        
        tui_draw_window(2, 1, 76, 21, "PingOS Diagnostics Real-Time Diagnostics Interface", COLOR_CYAN_ON_BLACK);
        
        unsigned short* vga_mem = (unsigned short*)VGA_ADDRESS;
        
        vga_print_string("\n  [SYSTEM PERFORMANCE METRIC PORTAL]\n", COLOR_WHITE_ON_BLACK);
        
        int total, used, free_blks;
        get_heap_diagnostics(&total, &used, &free_blks);
        
        char s_total[16], s_used[16], s_free_blks[16];
        itoa(total, s_total, 1);
        itoa(used, s_used, 1);
        itoa(free_blks, s_free_blks, 1);

        print_to_coords(6, 4, "System Status : ONLINE", COLOR_GREEN_ON_BLACK);
        print_to_coords(6, 6, "Platform      : Intel Protected Mode x86-32", COLOR_WHITE_ON_BLACK);
        print_to_coords(6, 8, "Heap Capacity : ", COLOR_WHITE_ON_BLACK);
        print_to_coords(22, 8, s_total, COLOR_AMBER_ON_BLACK);
        print_to_coords(22 + strlen(s_total), 8, " bytes", COLOR_WHITE_ON_BLACK);
        
        print_to_coords(6, 10, "Allocated Heap: ", COLOR_WHITE_ON_BLACK);
        print_to_coords(22, 10, s_used, COLOR_AMBER_ON_BLACK);
        print_to_coords(22 + strlen(s_used), 10, " bytes", COLOR_WHITE_ON_BLACK);
        
        print_to_coords(6, 12, "Free Fragments: ", COLOR_WHITE_ON_BLACK);
        print_to_coords(22, 12, s_free_blks, COLOR_AMBER_ON_BLACK);
        
        print_to_coords(6, 14, "Task Scheduler: ROUND-ROBIN COOPERATIVE ACTIVE", COLOR_CYAN_ON_BLACK);
        print_to_coords(6, 16, "File System   : PingFS In-Memory virtual structures", COLOR_CYAN_ON_BLACK);
        
        print_to_coords(6, 19, "Press any key to close visual system monitor channel...", COLOR_BLACK_ON_GREY);

        wait_for_key();
        vga_clear_screen();
    }
    else {
        vga_print_string("Error: Command '", COLOR_RED_ON_BLACK);
        vga_print_string(cmd_name, COLOR_RED_ON_BLACK);
        vga_print_string("' is unrecognized on this system. Type 'help' for options.\n", COLOR_RED_ON_BLACK);
    }
}

void run_shell() {
    char input_buffer[64];
    int input_index = 0;

    vga_print_string("\n", current_text_color);
    
    while (1) {
        char current_path[128];
        get_current_working_directory_path(current_path);

        vga_print_string("pingos:", current_prompt_color);
        vga_print_string(current_path, COLOR_CYAN_ON_BLACK);
        vga_print_string("> ", current_prompt_color);

        input_index = 0;
        memset(input_buffer, 0, 64);

        while (1) {
            char key = wait_for_key();

            if (key == '\n') {
                vga_put_char('\n', current_text_color);
                input_buffer[input_index] = '\0';
                
                push_history(input_buffer);
                parse_and_execute_command(input_buffer);
                break;
            } 
            else if (key == '\b') {
                if (input_index > 0) {
                    input_index--;
                    vga_put_char('\b', current_text_color);
                }
            } 
            else {
                if (input_index < 63) {
                    input_buffer[input_index++] = key;
                    vga_put_char(key, current_text_color);
                }
            }
        }
    }
}

/* ------------------------------------------------------------------------------
 * SECTION 19: Built-in Mock Daemon Threads / Cooperative Processes
 * ------------------------------------------------------------------------------
 */

void clock_daemon_process() {
    while (1) {
        unsigned char sec = get_cmos_value(0x00);
        unsigned char registerB = get_cmos_value(0x0B);
        if (!(registerB & 0x04)) {
            sec = (sec & 0x0F) + ((sec >> 4) * 10);
        }

        unsigned short* vga_mem = (unsigned short*)VGA_ADDRESS;
        int target_col = VGA_COLS - 6;
        int target_row = 0;
        
        char s_sec[4];
        itoa(sec, s_sec, 2);

        vga_mem[target_row * VGA_COLS + target_col]     = (COLOR_CYAN_ON_BLACK << 8) | 'T';
        vga_mem[target_row * VGA_COLS + target_col + 1] = (COLOR_CYAN_ON_BLACK << 8) | ':';
        vga_mem[target_row * VGA_COLS + target_col + 2] = (COLOR_CYAN_ON_BLACK << 8) | s_sec[0];
        vga_mem[target_row * VGA_COLS + target_col + 3] = (COLOR_CYAN_ON_BLACK << 8) | s_sec[1];
        vga_mem[target_row * VGA_COLS + target_col + 4] = (COLOR_CYAN_ON_BLACK << 8) | 's';

        task_queue[current_executing_index]->ticks_run++;
        task_yield(); 
    }
}

void sys_monitoring_daemon() {
    while (1) {
        int total, used, free_blks;
        get_heap_diagnostics(&total, &used, &free_blks);

        if (used > (total * 80) / 100) {
            unsigned short* vga_mem = (unsigned short*)VGA_ADDRESS;
            const char* alert = " ! MEM LIMIT ! ";
            for (int i = 0; alert[i] != '\0'; i++) {
                vga_mem[0 * VGA_COLS + 30 + i] = (COLOR_BLACK_ON_AMBER << 8) | alert[i];
            }
        }

        task_queue[current_executing_index]->ticks_run++;
        task_yield();
    }
}

/* ------------------------------------------------------------------------------
 * SECTION 20: Kernel Bootstrap Entry Sequence
 * ------------------------------------------------------------------------------
 */

void main() {
    // 1. Initial GDT/IDT register setups
    init_gdt();
    init_idt();

    // 2. Setup Memory Allocations Pool
    init_heap();

    // 3. Setup Virtual Disk Filesystem
    init_pingfs();

    // 4. Initialize Multi-tasking Schedulers
    init_scheduler();

    // 5. Initialize Hardware Keyboard, Mouse & USB Controllers
    keyboard_init();
    mouse_init();
    usb_init();

    // 6. Spawn Background System Threads
    create_kernel_task("System_Clock_Daemon", clock_daemon_process);
    create_kernel_task("System_Memory_Daemon", sys_monitoring_daemon);

    // 7. Draw Splash Screen (Monochrome layout exactly restored)
    vga_print_string("================================================================================\n", COLOR_GREY_ON_BLACK);
    vga_print_string("PingOS\n", COLOR_WHITE_ON_BLACK);
    vga_print_string("OS made mostly for testing as a hobby project. Have fun!\n", COLOR_GREY_ON_BLACK);
    vga_print_string("If you want to contribute, then go to https://github.com/alan-dev-dotcom/PingOS/\n", COLOR_GREY_ON_BLACK);
    vga_print_string("For a list of commands, type \"help\"\n", COLOR_GREY_ON_BLACK);
    vga_print_string("To check the kernel version, type \"uname -r\"\n", COLOR_GREY_ON_BLACK);
    vga_print_string("To check OS version, type \"uname\"\n", COLOR_GREY_ON_BLACK);
    vga_print_string("=================================================================================\n\n", COLOR_GREY_ON_BLACK);

    // 8. Auto-probe modern and legacy GPU controllers & setup DRM
    probe_all_gpus();

    // 8.5 Auto-probe wireless and Bluetooth network hardware
    probe_all_wireless();

    // 8.6 Auto-probe wired Ethernet controllers
    probe_all_ethernet();

    // 9. Fire interactive shell loop
    run_shell();
}