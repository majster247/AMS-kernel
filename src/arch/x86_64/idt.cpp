#include "../include/idt.h"
#include "../include/io.h"
#include "../include/klib.h"
#include "../include/serial.h"

// Zewnętrzne deklaracje stubów z assemblera (musisz mieć plik interrupts.S ze stubami isr0-31 oraz irq0-15)
extern "C" {
    void isr0();  void isr1();  void isr2();  void isr3();  void isr4();  void isr5();
    void isr6();  void isr7();  void isr8();  void isr9();  void isr10(); void isr11();
    void isr12(); void isr13(); void isr14(); void isr15(); void isr16(); void isr17();
    void isr18(); void isr19(); void isr20(); void isr21(); void isr22(); void isr23();
    void isr24(); void isr25(); void isr26(); void isr27(); void isr28(); void isr29();
    void isr30(); void isr31();
    
    void irq0();  void irq1();  void irq2();  void irq3();  void irq4();  void irq5();
    void irq6();  void irq7();  void irq8();  void irq9();  void irq10(); void irq11();
    void irq12(); void irq13(); void irq14(); void irq15();

}

idt_entry idt[256];

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags, uint8_t ist) {
    idt[num].isr_low = base & 0xFFFF;
    idt[num].kernel_cs = sel;
    idt[num].ist = ist & 0x7;
    idt[num].attributes = flags;
    idt[num].isr_mid = (base >> 16) & 0xFFFF;
    idt[num].isr_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].reserved = 0;
}

void pic_remap() {
    // Zapisanie masek
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);

    // Rozpoczęcie inicjalizacji (ICW1)
    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();

    // Mapowanie wektorów (ICW2)
    outb(0x21, 0x20); // Master PIC -> IDT 32 (0x20)
    io_wait();
    outb(0xA1, 0x28); // Slave PIC  -> IDT 40 (0x28)
    io_wait();

    // Konfiguracja połączenia Master-Slave (ICW3)
    outb(0x21, 0x04);
    io_wait();
    outb(0xA1, 0x02);
    io_wait();

    // Tryb 8086 (ICW4)
    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();

    // Przywrócenie masek (na razie wszystko zamaskowane, odmaskujemy w kmain)
    outb(0x21, a1);
    outb(0xA1, a2);
}

extern "C" void keyboard_handler(registers* r);

extern "C" void interrupt_handler(registers* r) {

    // KROK DEBUGOWANIA: Logujemy absolutnie KAŻDE wywołanie przerwania sprzętowego
    if (r->int_no >= 32 && r->int_no <= 47) {
        print_serial("[IRQ DETECTED] Odebrano przerwanie sprzętowe numer: ");
        print_num(r->int_no);
        print_serial("\n");
    }

    // 1. Jeśli to wyjątek procesora (0-31)
    if (r->int_no < 32) {
        print_serial("\n[PANIC] Wyjatek procesora: ");
        print_num(r->int_no);
        print_serial(" | Error Code: ");
        print_hex(r->err_code);
        print_serial("\nSystem zatrzymany.\n");
        for (;;) { asm volatile("hlt"); }
    }

    // 2. Jeśli to przerwanie sprzętowe (IRQs 0-15 mapowane na IDT 32-47)
    if (r->int_no >= 32 && r->int_no <= 47) {
        uint8_t irq_no = r->int_no - 32;

        if (irq_no == 1) {
            // Przerwanie klawiatury -> wywołujemy nasz sterownik
            keyboard_handler(r);
        } else {
            // Każde inne przerwanie sprzętowe (np. zegar IRQ0 = IDT 32) na razie ignorujemy,
            // ale MUSIMY wysłać EOI, żeby system żył!
        }

        // Wysłanie EOI (End of Interrupt) do kontrolerów PIC
        if (irq_no >= 8) {
            outb(0xA0, 0x20); // EOI dla Slave PIC
        }
        outb(0x20, 0x20);     // EOI dla Master PIC
    }
}

extern "C" void idt_init() {
    for (size_t i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0, 0); // Inicjalizacja wszystkich wpisów IDT na "pusty" stan
    }

    pic_remap();

    // Rejestracja wyjątków procesora (0-31), flagi 0x8E = Present, Ring 0, Interrupt Gate
    // Dla błędów krytycznych podpinamy IST=1 (nasz osobny, bezpieczny stos)
    idt_set_gate(0,  (uint64_t)isr0,  0x08, 0x8E, 0);
    idt_set_gate(1,  (uint64_t)isr1,  0x08, 0x8E, 0);
    idt_set_gate(2,  (uint64_t)isr2,  0x08, 0x8E, 0);
    idt_set_gate(3,  (uint64_t)isr3,  0x08, 0x8E, 0);
    idt_set_gate(4,  (uint64_t)isr4,  0x08, 0x8E, 0);
    idt_set_gate(5,  (uint64_t)isr5,  0x08, 0x8E, 0);
    idt_set_gate(6,  (uint64_t)isr6,  0x08, 0x8E, 0);
    idt_set_gate(7,  (uint64_t)isr7,  0x08, 0x8E, 0);
    idt_set_gate(8,  (uint64_t)isr8,  0x08, 0x8E, 0); // Double Fault na bezpiecznym stosie IST1
    idt_set_gate(9,  (uint64_t)isr9,  0x08, 0x8E, 0);
    idt_set_gate(10, (uint64_t)isr10, 0x08, 0x8E, 0);
    idt_set_gate(11, (uint64_t)isr11, 0x08, 0x8E, 0);
    idt_set_gate(12, (uint64_t)isr12, 0x08, 0x8E, 0);
    idt_set_gate(13, (uint64_t)isr13, 0x08, 0x8E, 0); // General Protection Fault na IST1
    idt_set_gate(14, (uint64_t)isr14, 0x08, 0x8E, 0); // Page Fault na IST1
    idt_set_gate(15, (uint64_t)isr15, 0x08, 0x8E, 0);
    idt_set_gate(16, (uint64_t)isr16, 0x08, 0x8E, 0);
    idt_set_gate(17, (uint64_t)isr17, 0x08, 0x8E, 0);
    idt_set_gate(18, (uint64_t)isr18, 0x08, 0x8E, 0);
    idt_set_gate(19, (uint64_t)isr19, 0x08, 0x8E, 0);
    idt_set_gate(20, (uint64_t)isr20, 0x08, 0x8E, 0);
    idt_set_gate(21, (uint64_t)isr21, 0x08, 0x8E, 0);
    idt_set_gate(22, (uint64_t)isr22, 0x08, 0x8E, 0);
    idt_set_gate(23, (uint64_t)isr23, 0x08, 0x8E, 0);
    idt_set_gate(24, (uint64_t)isr24, 0x08, 0x8E, 0);
    idt_set_gate(25, (uint64_t)isr25, 0x08, 0x8E, 0);
    idt_set_gate(26, (uint64_t)isr26, 0x08, 0x8E, 0);
    idt_set_gate(27, (uint64_t)isr27, 0x08, 0x8E, 0);
    idt_set_gate(28, (uint64_t)isr28, 0x08, 0x8E, 0);
    idt_set_gate(29, (uint64_t)isr29, 0x08, 0x8E, 0);
    idt_set_gate(30, (uint64_t)isr30, 0x08, 0x8E, 0);
    idt_set_gate(31, (uint64_t)isr31, 0x08, 0x8E, 0);

    // Rejestracja przerwiań sprzętowych IRQ (32-47)
    idt_set_gate(32, (uint64_t)irq0,  0x08, 0x8E, 0); // Timer
    idt_set_gate(33, (uint64_t)irq1,  0x08, 0x8E, 1); // Klawiatura
    idt_set_gate(34, (uint64_t)irq2,  0x08, 0x8E, 0);
    idt_set_gate(35, (uint64_t)irq3,  0x08, 0x8E, 0);
    idt_set_gate(36, (uint64_t)irq4,  0x08, 0x8E, 0);
    idt_set_gate(37, (uint64_t)irq5,  0x08, 0x8E, 0);
    idt_set_gate(38, (uint64_t)irq6,  0x08, 0x8E, 0);
    idt_set_gate(39, (uint64_t)irq7,  0x08, 0x8E, 0);
    idt_set_gate(40, (uint64_t)irq8,  0x08, 0x8E, 0);
    idt_set_gate(41, (uint64_t)irq9,  0x08, 0x8E, 0);
    idt_set_gate(42, (uint64_t)irq10, 0x08, 0x8E, 0);
    idt_set_gate(43, (uint64_t)irq11, 0x08, 0x8E, 0);
    idt_set_gate(44, (uint64_t)irq12, 0x08, 0x8E, 0); // Myszka
    idt_set_gate(45, (uint64_t)irq13, 0x08, 0x8E, 0);
    idt_set_gate(46, (uint64_t)irq14, 0x08, 0x8E, 0);
    idt_set_gate(47, (uint64_t)irq15, 0x08, 0x8E, 0);

    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) idtr;

    idtr.limit = (sizeof(idt_entry) * 256) - 1; // Upewnij się czy masz 256 wpisów
    idtr.base = (uint64_t)idt; // Twoja tablica IDT

    asm volatile("lidt %0" : : "m"(idtr));
    print_serial("IDT loaded into CPU!\n");
}