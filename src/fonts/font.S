.section .rodata
.global _binary_font_psf_start
.global _binary_font_psf_end

_binary_font_psf_start:
    .incbin "src/fonts/iso01a-8x14.psf"  # Ścieżka do pliku czcionki
_binary_font_psf_end: