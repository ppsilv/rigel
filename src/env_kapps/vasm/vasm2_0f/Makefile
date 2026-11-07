# Unix, using gcc

CC = m68k-elf-gcc
TARGET =
TARGETEXTENSION =

CCOUT = -o $(DUMMY)
CFLAGS = -c -m68000 -std=c99 -ffreestanding -pedantic -Wno-long-long -DUNIX -DNO_FLOAT $(OUTFMTS) \
-Wl,--strip-debug -fomit-frame-pointer -ffunction-sections -fdata-sections -include lixo/fix_winnt.h \
-I $(RIGELDIR)/include \
-I $(RIGELDIR)/src/kernel/include \
-I $(RIGELDIR)/src/librigel/include \
-I $(RIGELDIR)/src/kernel/include

LD = $(CC)
LDOUT = $(CCOUT)
LDFLAGS = $(CPU) -Wl,-q -T $(LINKER_FILE) -nostartfiles -Wl,--gc-sections -Wl,-Map=rigel.map  -Wl,--undefined=do_load \
-L$(RIGELDIR)/lib -L$(RIGELDIR)/lib/rigel -lrigel -lc -lgcc -Wl,--end-group

RM = rm -f

include make.rules
