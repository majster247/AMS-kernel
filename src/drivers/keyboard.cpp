#include "idt.h"
#include "io.h"
#include "serial.h"

// Podstawowa mapa Scancode Set 1 na ASCII
static const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,   '*',   0, ' '
};

extern "C" void keyboard_handler(registers* r) {
    (void)r; // Unikamy warningu o nieużywanym argumencie
    
    // Odczytujemy bajt z portu klawiatury
    uint8_t scancode = inb(0x60);

    // Jeśli bit 0x80 jest zapalony, klawisz został puszczony
    if (scancode & 0x80) {
        return;
    }

    // Jeśli mieści się w mapie, wypiszmy go na port szeregowy (widoczne w qemu.log / konsoli)
    if (scancode < sizeof(scancode_to_ascii)) {
        char c = scancode_to_ascii[scancode];
        if (c != 0) {
            // Na razie logujemy do seriala, żeby sprawdzić czy działa przerwanie
            write_serial(c);
            
            // TODO: Tutaj w kolejnym kroku wepniemy rysowanie na Twoim framebufferze!
        }
    }
}