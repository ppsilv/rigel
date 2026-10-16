# =====================================================================
# Makefile Principal - libethernet.a (Sistema Rigel / M68k)
# =====================================================================

CROSS   ?= m68k-elf-
CC      := $(CROSS)gcc
AR      := $(CROSS)ar

RIGELDIR  ?= $(HOME)/project/rigel/rigel
LIBRIGEL  ?= $(RIGELDIR)/src/librigel
FSRC := src

# Flags idênticas ao padrão do Rigel
CFLAGS  := -m68000 -Os -Wall -ffreestanding -ffunction-sections -fdata-sections \
		   -Isrc/include/lwip \
           -I$(RIGELDIR)/include \
           -Iinclude \
           -Isrc/include \
           -I$(RIGELDIR)/src/kernel/include \
           -I$(LIBRIGEL)/include \
           -include lixo/fix_winnt.h

TARGET  := libethernet.a

# Lista base de fontes (será preenchida pelos subdiretórios)
SRCS :=

# Puxa as definições de fontes de cada subpasta
include src/api/sources.mk
include src/core/sources.mk
include src/core/ipv4/sources.mk
include src/core/ipv6/sources.mk
include src/netif/sources.mk

# Transforma a lista de .c em .o mantendo a estrutura de diretórios
OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Criando biblioteca estática $@..."
	@rm -f $@
	$(AR) rcs $@ $(OBJS)
	cp libethernet.a $(RIGELDIR)/lib/.

%.o: %.c
	@echo "Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Limpando objetos e bibliotecas..."
	@rm -f $(OBJS) $(TARGET)

.PHONY: all clean 
