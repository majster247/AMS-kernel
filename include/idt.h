#include <stdint.h>

// Struktura rejestrów spychanych na stos podczas przerwiań (dla handlera)
struct registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

// Wpis w IDT (16 bajtów w trybie 64-bitowym)
struct idt_entry {
    uint16_t isr_low;   // Dolne 16 bitów adresu funkcji
    uint16_t kernel_cs; // Selektor segmentu kodu (0x08)
    uint8_t  ist;       // Interrupt Stack Table offset (3 bity)
    uint8_t  attributes;// Flagi (Present, DPL, Typ bramki)
    uint16_t isr_mid;   // Środkowe 16 bitów adresu funkcji
    uint32_t isr_high;  // Górne 32 bity adresu funkcji
    uint32_t reserved;
} __attribute__((packed));

struct idtr_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

extern "C" void idt_init();