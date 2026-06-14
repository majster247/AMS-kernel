.macro ISR_NOERRCODE num
.global isr\num
isr\num:
    pushq $0                 # Sztuczny kod błędu (0)
    pushq $\num               # Numer przerwania
    jmp interrupt_common
.endm

.macro ISR_ERRCODE num
.global isr\num
isr\num:
    # Procesor sam wypycha kod błędu na stos
    pushq $\num               # Numer przerwania
    jmp interrupt_common
.endm

.macro IRQ num int_no
.global irq\num
irq\num:
    pushq $0                 # Sztuczny kod błędu (0)
    pushq $\int_no            # Numer mapowanego przerwania w IDT (32 + num)
    jmp interrupt_common
.endm

.section .text
.align 8

# 1. Wyjątki procesora (ISRs 0-31)
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8   # Double Fault posiada kod błędu
ISR_NOERRCODE 9
ISR_ERRCODE   10  # Invalid TSS posiada kod błędu
ISR_ERRCODE   11  # Segment Not Present posiada kod błędu
ISR_ERRCODE   12  # Stack Segment Fault posiada kod błędu
ISR_ERRCODE   13  # General Protection Fault posiada kod błędu
ISR_ERRCODE   14  # Page Fault posiada kod błędu
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17  # Alignment Check posiada kod błędu
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_ERRCODE   21  # Control Protection Exception posiada kod błędu
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30  # VMM Communication Exception posiada kod błędu
ISR_NOERRCODE 31

# 2. Przerwania sprzętowe (IRQs 0-15 mapped to IDT 32-47)
IRQ 0  32
IRQ 1  33
IRQ 2  34
IRQ 3  35
IRQ 4  36
IRQ 5  37
IRQ 6  38
IRQ 7  39
IRQ 8  40
IRQ 9  41
IRQ 10 42
IRQ 11 43
IRQ 12 44
IRQ 13 45
IRQ 14 46
IRQ 15 47

# 3. System Call Stub (INT 0x80)
.global isr128
isr128:
    pushq $0
    pushq $128
    jmp interrupt_common

# 4. Wspólny kod dla wszystkich przerwaniowych bram
interrupt_common:
    # Zapisujemy stan rejestrów (struktura 'registers' w idt.h)
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # Wskaźnik na stos (RSP) jest teraz wskaźnikiem na strukturę rejestrów.
    # Przekazujemy go do C++ jako pierwszy argument (rejestr RDI wg x86_64 ABI)
    movq %rsp, %rdi

    cld

    # Wywołujemy główny dyspozytor przerwiań z pliku idt.cpp
    .extern interrupt_handler
    call interrupt_handler

    # Odtwarzamy stan rejestrów
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax

    # Czyścimy ze stosu wypchnięte wcześniej: int_no oraz err_code (2 * 8 bajtów)
    addq $16, %rsp
    
    # Powrót z przerwania w trybie 64-bitowym
    iretq