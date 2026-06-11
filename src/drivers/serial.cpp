#include "serial.h"
#include "io.h"

#define COM1 0x3f8

void init_serial() {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80); 
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03); 
    outb(COM1 + 2, 0xC7); 
    outb(COM1 + 4, 0x0B); 
}

void write_serial(char a) {
    outb(COM1, a);
}

void print_serial(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        write_serial(str[i]);
    }
}

void print_num(uint64_t val) {
    char buf[21];
    int i = 19;
    buf[20] = '\0';
    if (val == 0) print_serial("0");
    while (val > 0) {
        buf[i--] = (val % 10) + '0';
        val /= 10;
    }
    print_serial(&buf[i + 1]);
}

void print_hex(uint64_t val) {
    char buf[17];
    int i = 15;
    buf[16] = '\0';
    if (val == 0) print_serial("0x0");
    while (val > 0) {
        uint8_t digit = val & 0xF;
        buf[i--] = (digit < 10) ? (digit + '0') : (digit - 10 + 'A');
        val >>= 4;
    }
    print_serial("0x");
    print_serial(&buf[i + 1]);
}