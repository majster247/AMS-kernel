#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../limine/limine.h"
#include "fonts/font.hpp"
#include "../include/gdt.h"
#include "../include/idt.h"
#include "../include/io.h"
#include "../include/klib.h"
#include "../include/serial.h"

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

void hlt() {
    asm volatile ("hlt");
}

void draw_char(limine_framebuffer* fb, char c, int cx, int cy, uint32_t fg_color, uint32_t bg_color) {
    psf1_header* font = (psf1_header*)_binary_font_psf_start;
    uint8_t* font_buffer = (uint8_t*)_binary_font_psf_start + sizeof(psf1_header);
    uint8_t* glyph = font_buffer + (c * font->char_size);

    uint32_t fb_pitch_pixels = fb->pitch / 4;
    uint32_t* fb_address = (uint32_t*)fb->address;

    for (int y = 0; y < font->char_size; y++) {
        for (int x = 0; x < 8; x++) {
            if ((glyph[y] << x) & 0x80) {
                fb_address[(cy + y) * fb_pitch_pixels + (cx + x)] = fg_color;
            } else {
                fb_address[(cy + y) * fb_pitch_pixels + (cx + x)] = bg_color;
            }
        }
    }
}

void kmain(void) {
    init_serial();
    print_serial("Hello, AMS Kernel!\n");
    if (framebuffer_request.response == nullptr) {
        print_serial("Error: No framebuffer response!\n");
        for (;;) { asm ("hlt"); }
    }
    
    print_serial("Framebuffer found!\n");
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    print_serial("\nFramebuffer resolution: ");
    print_num(fb->width);
    print_serial("x");
    print_num(fb->height);
    print_serial("\nFramebuffer pitch: ");
    print_num(fb->pitch);
    print_serial("\nInitializing GDT and IDT...\n");

    gdt_init();
    print_serial("GDT initialized!\n");
    idt_init();
    print_serial("IDT initialized!\n");

    // Przepłukanie bufora klawiatury
    while (inb(0x64) & 1) {
        inb(0x60);
    }

    // Odmaskowujemy TYLKO klawiaturę (IRQ1). 
    // Bitowo: 0xFD to 11111101 (włączony tylko bit 1, bit 0 od zegara jest wyłączony!)
    outb(0x21, 0xFD); 
    outb(0xA1, 0xFF); // Slave PIC całkowicie wyłączony
    
    print_serial("Keyboard IRQ unmasked, PIT timer masked!\n");

    print_serial("Interrupts enabled!\n");

    // Rysowanie promptu na ekranie
    const char* msg = "majster@AMS-kernel:# ";
    int x = 10;
    for(int i = 0; msg[i] != '\0'; i++) {
        draw_char(fb, msg[i], x, 10, 0xFFFFFFFF, 0x00000000);
        x += 8;
    }

    print_serial("\n[Kernel] System gotowy. Czekam na przerwania...\n");

    // Prawidłowa pętla bez końca
    while (1) {
        asm volatile("hlt");
    }
}
}

//TODO: naprawić obsługę przerwań w idt.cpp celem dodania obsługi klawiatury (i innych IRQ) oraz poprawić panic handler, żeby wyświetlał więcej informacji o błędzie (numer przerwania, kod błędu jeśli jest, itp.)
