#include "../include/gdt.h"
#include "../include/io.h"
#include "../include/klib.h"
#include "../include/serial.h"

// Definiujemy wielkość stosu dla wyjątków krytycznych (8 KB)
static constexpr uint64_t IST_STACK_SIZE = 8192;
static uint8_t ist1_stack[IST_STACK_SIZE] __attribute__((aligned(16)));

// Selektory segmentów
static constexpr uint16_t KERNEL_DS = 0x10;
static constexpr uint16_t TSS_SELECTOR = 0x38;

// Stałe flagi dla deskryptorów w architekturze x86_64 (płaski model pamięci Linuksa)
static constexpr uint64_t GDT_KERNEL_CODE64 = 0x00AF9B000000FFFFULL;
static constexpr uint64_t GDT_KERNEL_DATA64 = 0x00CF93000000FFFFULL;
static constexpr uint64_t GDT_USER_DATA64   = 0x00CFF3000000FFFFULL;
static constexpr uint64_t GDT_USER_CODE64   = 0x00AFFB000000FFFFULL;

// Tablica GDT (11 slotów wystarczy na null, kernel code/data, user code/data i 128-bitowy TSS)
uint64_t gdt_real[11] __attribute__((aligned(16))); 
tss_entry system_tss __attribute__((aligned(16)));

// Zewnętrzny symbol wskaźnika stosu Twojego kernela (zdefiniowany w linker.lds lub asm)
extern "C" uint8_t stack_top; 

extern "C" void gdt_init() {
    print_serial("Initializing GDT...\n");
    print_serial("Clearing GDT entries...\n");

    for (size_t i = 0; i < (sizeof(gdt_real) / sizeof(gdt_real[0])); i++) {
        gdt_real[i] = 0;
    }

    print_serial("Setting up GDT entries...\n");

    gdt_real[0] = 0;                  // Null descriptor
    print_serial("Null descriptor set.\n");
    gdt_real[1] = GDT_KERNEL_CODE64;  // Indeks 1 -> Selektor 0x08
    print_serial("Kernel Code Segment descriptor set.\n");
    gdt_real[2] = GDT_KERNEL_DATA64;  // Indeks 2 -> Selektor 0x10
    print_serial("Kernel Data Segment descriptor set.\n");
    gdt_real[3] = GDT_USER_DATA64;    // Indeks 3 -> Selektor 0x18
    print_serial("User Data Segment descriptor set.\n");
    gdt_real[4] = GDT_USER_CODE64;    // Indeks 4 -> Selektor 0x20
    print_serial("User Code Segment descriptor set.\n");

    print_serial("Setting up TSS...\n");

    // Konfiguracja TSS (zauważ zmianę indeksów na 5 i 6!)
    uint64_t tss_base = (uint64_t)&system_tss;
    uint32_t tss_limit = sizeof(tss_entry) - 1;

   for (size_t i = 0; i < sizeof(tss_entry) / sizeof(uint64_t); i++) {
        ((uint64_t*)&system_tss)[i] = 0;
    }

    print_serial("TSS structure cleared.\n");
    
    // Przypisanie stosu jądra (używamy przekazanego stack_top)
    system_tss.rsp0 = (uint64_t)&stack_top; 
    system_tss.ist[0] = (uint64_t)(ist1_stack + IST_STACK_SIZE);
    system_tss.iopb_offset = sizeof(tss_entry);

    print_serial("TSS configured with kernel stack and IST.\n");

    // Deskryptor TSS zajmuje slot 5 i 6 (128-bit)
    gdt_real[5] = ((uint64_t)tss_limit & 0xFFFFULL) |
                  ((tss_base & 0xFFFFFFULL) << 16) |
                  (0x89ULL << 40) |  // Type: 0x89 (Available 64-bit TSS)
                  ((((uint64_t)tss_limit >> 16) & 0xFULL) << 48) |
                  (((tss_base >> 24) & 0xFFULL) << 56);
    gdt_real[6] = (tss_base >> 32) & 0xFFFFFFFFULL;

    print_serial("GDT entries set. Loading GDTR...\n");

    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) gdtr;

    gdtr.limit = sizeof(gdt_real) - 1;
    gdtr.base = (uint64_t)gdt_real;

    print_serial("GDTR limit: ");
    print_hex(gdtr.limit);
    print_serial("\nGDTR base: ");
    print_hex(gdtr.base);
    print_serial("\nExecuting LGDT...\n");

    asm volatile("lgdt %0" : : "m"(gdtr));

    print_serial("GDT loaded. Updating segment registers...\n");

    // Przeładowanie CS na 0x08 i SS/DS/ES na 0x10
    asm volatile(
        "pushq $0x10\n"                      // Nowy SS (Kernel Data = 0x10)
        "movq %%rsp, %%rax\n"
        "pushq %%rax\n"
        "pushfq\n"
        "pushq $0x08\n"                      // Nowy CS (Kernel Code = 0x08)
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "iretq\n"                            // Bezpieczniejsza alternatywa dla lretq w Long Mode
        "1:\n"
        "movw $0x10, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        : : : "rax", "memory"
    );
    print_serial("Segment registers updated. GDT initialization complete!\n");

    // Na koniec ładujemy rejestr zadań (TR) wskazujący na nasz TSS
    // Indeks 5 * 8 = 40 = 0x28
    print_serial("Loading Task Register (TR) with TSS selector 0x28...\n");
    asm volatile("ltr %%ax" : : "a"((uint16_t)0x28));
    print_serial("TSS loaded. GDT and TSS initialization complete!\n");
}
