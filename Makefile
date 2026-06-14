override OUTPUT := ams-kernel.elf

CC := x86_64-elf-g++
LD := x86_64-elf-ld

# Flagi kompilacji - dodaliśmy wyszukiwanie nagłówków z folderu include
CFLAGS := -g -O2 -pipe -Wall -Wextra -std=c++20 -ffreestanding -fno-stack-protector \
          -fno-exceptions -fno-rtti -mno-red-zone -mcmodel=kernel -I include -I src -fno-builtin -mgeneral-regs-only

LDFLAGS := -T src/linker.lds -nostdlib -z max-page-size=0x1000

# Automatyczne wyszukiwanie wszystkich plików źródłowych w strukturze src/
CPPSRC := $(shell find src -name "*.cpp")
ASMSRC := $(shell find src -name "*.S")

# Konwersja ścieżek źródłowych na pliki obiektów .o
OBJ := $(CPPSRC:.cpp=.o) $(ASMSRC:.S=.o)

all: bin/$(OUTPUT)

bin/$(OUTPUT): $(OBJ)
	mkdir -p bin
	$(LD) $(LDFLAGS) $(OBJ) -o $@

# Reguła kompilacji dla plików C++ (zachowuje strukturę podkatalogów)
src/%.o: src/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Reguła kompilacji dla plików assemblerowych
src/%.o: src/%.S
	$(CC) $(CFLAGS) -c $< -o $@

iso: bin/$(OUTPUT)
	rm -rf iso_root
	mkdir -p iso_root/boot/limine
	cp bin/$(OUTPUT) iso_root/boot/
	cp limine.conf iso_root/boot/limine/
	cp limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/boot/limine/
	mkdir -p iso_root/EFI/BOOT
	cp limine/BOOTX64.EFI limine/BOOTIA32.EFI iso_root/EFI/BOOT/
	
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
	        -no-emul-boot -boot-load-size 4 -boot-info-table \
	        --efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
	        --protective-msdos-label iso_root -o ams.iso
	
	./limine/limine bios-install ams.iso 

run: iso
	qemu-system-x86_64 -cdrom ams.iso -no-reboot -no-shutdown -serial stdio -d int,cpu_reset -D qemu.log
clean:
	rm -rf bin ams.iso iso_root
	find src -name "*.o" -type f -delete