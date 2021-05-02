TOOLCHAIN_PATH = /home/tux/Dokumente/hhu/Masterarbeit/dd-OS/gcc-arm-10.2-2020.11-x86_64-aarch64-none-elf/bin
ASM = aarch64-none-elf-gcc
CXX = aarch64-none-elf-gcc
LINKER = aarch64-none-elf-ld
OBJCPY = aarch64-none-elf-objcopy

CFILES = $(wildcard ./*.c)
OFILES = $(CFILES:.c=.o)
INCLUDES = -I./
GCCFLAGS = -Wall -O0 -ffreestanding -nostdinc -nostdlib -nostartfiles
LDFLAGS = -nostdlib -nostartfiles $(INCLUDES)
DELETE = rm

all: clean kernel8.img

startup.o: startup.S
	$(TOOLCHAIN_PATH)/$(ASM) $(GCCFLAGS) -c startup.S -o startup.o

%.o: %.c
	$(TOOLCHAIN_PATH)/$(CXX) $(GCCFLAGS) -c $< -o $@

kernel8.img: startup.o $(OFILES)
	$(TOOLCHAIN_PATH)/$(LINKER) $(LDFLAGS) startup.o $(OFILES) -T link.ld -o kernel8.elf
	$(TOOLCHAIN_PATH)/$(OBJCPY) -O binary kernel8.elf kernel8.img

clean:
	$(DELETE) -rf kernel8.img
	$(DELETE) -rf startup.o kernel8.elf $(OFILES)

