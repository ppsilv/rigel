# Makefile do Rigel
CROSS   := m68k-elf-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

SPLITTER := python3 ./tools/split68k.py
FILLER := python3 ./tools/fill.py
PROGRAMMER := minipro
CHECKSUMER := python3 ./tools/checksum_put.py

CHIP ?= AT28C256
ifeq ($(CHIP), AT29C512)
    CHIP_TYPE  := AT29C512
    FLASH_SIZE := 65536  # 64KB
    ROM_SIZE   := 65536  # 64KB
else ifeq ($(CHIP), AT28C256)
    CHIP_TYPE  := AT28C256
    FLASH_SIZE := 32768  # 32KB
    ROM_SIZE   := 32768  # 32KB
else
    $(error Chip desconhecido: $(CHIP). Escolha AT29C512 ou AT28C256)
endif


CPU     ?= -m68000
# nano.specs = newlib-nano (bem menor, ideal para ROM de 32K/64K)
CFLAGS  := $(CPU) -Wall -Os -ffreestanding -fomit-frame-pointer -ffunction-sections -fdata-sections -I kernel -I $(RIGELDIR)/include -I ./include
LDFLAGS := $(CPU) -T rigel.ld -nostartfiles -Wl,--gc-sections -Wl,-Map=rigel.map
# crt0 e' nosso; libc/libgcc entram automaticamente
LDLIBS  := -L$(RIGELDIR)/lib -Wl,--start-group -lc -lgcc -Wl,--end-group

SRCS := core/main.c core/syscalls.c boot/vector.c boot/ringbuffer.c core/boot_kernel.c \
../drivers/fatfs/ffunicode.c \
../drivers/fatfs/ffsystem.c \
../drivers/fatfs/ff.c \
../drivers/ide/ata.c \
../drivers/ide/diskio.c \



ASMS := boot/crt0.S boot/interrupts.S ../drivers/duart/duartasm.S

OBJS := $(ASMS:.S=.o) $(SRCS:.c=.o)

.PHONY: all clean size

TARGET := rigel
TARGET_BIN := $(TARGET).bin
TARGET_LOW := $(TARGET)_low.bin
TARGET_HIGH := $(TARGET)_high.bin

all: $(TARGET).bin size

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CPU) -c $< -o $@

$(TARGET).elf: $(OBJS) rigel.ld
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	m68k-elf-size -A $(TARGET).elf
	
size: $(TARGET).elf
	@$(SIZE) $<

clean:
	rm -f $(OBJS) rigel.elf rigel.bin rigel.map

split: rigel.elf
	@echo "Dividindo $(TARGET_BIN) em words 16-bit..."
	@$(SPLITTER) $(TARGET_BIN)
	@$(FILLER) $(TARGET_LOW) $(FLASH_SIZE)
	@$(FILLER) $(TARGET_HIGH) $(FLASH_SIZE)
	@echo "Arquivos gerados: $(TARGET_LOW) e $(TARGET_HIGH)"

programL:
	@echo "Gravando parte LOW na Flash..."
	$(PROGRAMMER) -p $(CHIP_TYPE) -w $(TARGET_LOW) -s -u
	@echo "Gravação concluída LOW! BOA SORTE"

programH: split
	@echo "Gravando parte HIGH na Flash..."
	@echo $(CHIP_TYPE)
	$(PROGRAMMER) -p $(CHIP_TYPE) -w $(TARGET_HIGH) -s -u
	@echo "Gravação concluída HIGH! GRAVE A MEMORIA LOW"