#pragma once
#include <stdint.h>

// Struktura pojedynczego wpisu TSS (Task State Segment) w x86_64
struct tss_entry {
    uint32_t reserved0;
    uint64_t rsp0;      // Stos dla Ring 0 (gdy przerwanie nastąpi z Ring 3)
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];    // Tablica Interrupt Stack Table (IST1 do IST7)
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} __attribute__((packed));

// Struktura wskaźnika GDTR przekazywanego do instrukcji lgdt
struct gdtr_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// Inicjalizacja GDT i załadowanie TSS
extern "C" void gdt_init();