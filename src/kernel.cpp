#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../limine/limine.h"




#ifdef __cplusplus
#define restrict __restrict
#endif

#define COM1 0x3f8
char cmd_buffer[256];
size_t buffer_index = 0;

extern "C" {

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0,
    .response = nullptr
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *restrict pdest = (uint8_t *restrict)dest;
    const uint8_t *restrict psrc = (const uint8_t *restrict)src;
    for (size_t i = 0; i < n; i++) pdest[i] = psrc[i];
    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;
    for (size_t i = 0; i < n; i++) p[i] = (uint8_t)c;
    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;
    if (psrc > pdest) {
        for (size_t i = 0; i < n; i++) pdest[i] = psrc[i];
    } else if (psrc < pdest) {
        for (size_t i = n; i > 0; i--) pdest[i-1] = psrc[i-1];
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) return p1[i] < p2[i] ? -1 : 1;
    }
    return 0;
}


void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}


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


void print_hex(uint64_t val) {
    const char hex[] = "0123456789ABCDEF";
    char buf[17];
    int i = 15;
    buf[16] = '\0';
    if (val == 0) print_serial("0");
    while (val > 0) {
        buf[i--] = hex[val % 16];
        val /= 16;
    }
    print_serial(&buf[i + 1]);
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



int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}


void process_command(char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        print_serial("Dostepne komendy: help, clear, status\n");
    } else if (strcmp(cmd, "status") == 0) {
        print_serial("Kernel dziala w trybie 64-bit (long mode)\n");
    } else {
        print_serial("Nieznana komenda: ");
        print_serial(cmd);
        print_serial("\n");
    }
}


void add_to_buffer(char c) {
    if (c == '\r' || c == '\n') {
        cmd_buffer[buffer_index] = '\0';
        process_command(cmd_buffer);
        buffer_index = 0;
        print_serial("AMS> "); 
    } else if (c == 0x08) { 
        if (buffer_index > 0) buffer_index--;
    } else {
        cmd_buffer[buffer_index++] = c;
    }
}


//-------------------------------------------------------------------
// Main kernel function
//------------------------------------------------------------------


void kmain(void) {
    init_serial();
    print_serial("Hello, AMS Kernel!\n");
    if (framebuffer_request.response == nullptr) {
        print_serial("Error: No framebuffer response!\n");
        for (;;) { asm ("hlt"); }
    }
    
    print_serial("Framebuffer found!\n");

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];


    print_serial("Framebuffer details:\n");
    print_serial("Width: ");
    print_num(fb->width);
    print_serial("\nHeight: ");
    print_num(fb->height);
    print_serial("\nPitch: ");
    print_num(fb->pitch);
    print_serial("\nBPP: ");
    print_num(fb->bpp);

    while (1) {
        if (inb(COM1 + 5) & 1) {
            char c = inb(COM1);
            write_serial(c); 
            add_to_buffer(c);
        }
    }
}
} 