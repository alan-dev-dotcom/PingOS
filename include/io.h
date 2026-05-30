/* include/io.h - Port I/O Helpers for PingOS */
#ifndef IO_H
#define IO_H

/* Write a byte to an I/O port */
static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Read a byte from an I/O port */
static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Write a 16-bit word to an I/O port */
static inline void outw(unsigned short port, unsigned short val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

/* Read a 16-bit word from an I/O port */
static inline unsigned short inw(unsigned short port) {
    unsigned short ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Write a 32-bit double word to an I/O port */
static inline void outd(unsigned short port, unsigned int val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

/* Read a 32-bit double word from an I/O port */
static inline unsigned int ind(unsigned short port) {
    unsigned int ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#endif