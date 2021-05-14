TOOLCHAIN_PATH = /home/tux/bin/aarch32/gcc-arm-10.2-2020.11-x86_64-arm-none-eabi/bin
ASM = arm-none-eabi-gcc
CXX = arm-none-eabi-gcc
LINKER = arm-none-eabi-gcc
OBJCPY = arm-none-eabi-objcopy

# gcc -c -Q -march=native --help=target 

CFILES = $(wildcard ./*.c)
OFILES = $(CFILES:.c=.o)
INCLUDES = -I./
GCCFLAGS = -Wall -O0 \
	-march=armv6zk -mcpu=arm1176jzf-s \
	-mfpu=vfp -mfloat-abi=hard \
	-ffreestanding -nostdinc -nostdlib -nostartfiles
LDFLAGS = \
	-march=armv6zk -mcpu=arm1176jzf-s \
	-mfpu=vfp -mfloat-abi=hard \
	-nostdlib -nostartfiles $(INCLUDES)
DELETE = rm

all: clean kernel.img

startup.o: startup.S
	$(TOOLCHAIN_PATH)/$(ASM) $(GCCFLAGS) -c startup.S -o startup.o

%.o: %.c
	$(TOOLCHAIN_PATH)/$(CXX) $(GCCFLAGS) -c $< -o $@

kernel.img: startup.o $(OFILES)
	$(TOOLCHAIN_PATH)/$(LINKER) $(LDFLAGS) startup.o $(OFILES) -T link.ld -o kernel.elf
	$(TOOLCHAIN_PATH)/$(OBJCPY) -O binary kernel.elf sdcard/kernel.img

clean:
	$(DELETE) -rf sdcard/kernel.img
	$(DELETE) -rf startup.o kernel.elf $(OFILES)

