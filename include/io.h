#pragma once
#include <stdint.h>

inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

inline void io_wait() {
    // Port 0x80 jest tradycyjnie używany do krótkiego opóźnienia w komunikacji z PIC
    asm volatile("outb %%al, $0x80" : : "a"(0));
}