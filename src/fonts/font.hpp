#pragma once
#include <stdint.h>

struct psf1_header {
    uint8_t magic[2];     // Zawsze 0x36, 0x04
    uint8_t mode;         // Tryb czcionki (np. czy ma 256 czy 512 znaków)
    uint8_t char_size;    // Wysokość znaku w pikselach (zazwyczaj 16)
};

// Te symbole wyciągamy z pliku assemblerowego font.S
extern "C" char _binary_font_psf_start[];
extern "C" char _binary_font_psf_end[];