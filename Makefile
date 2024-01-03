LVGL_DIR_NAME ?= lvgl
LVGL_DIR ?= $(PWD)
-include $(LVGL_DIR)/$(LVGL_DIR_NAME)/lvgl.mk

EE_OBJS = lv_port_disp.o lv_port_indev.o $(CSRCS:.c=.o)
EE_INCS = -I$(PS2DEV)/gsKit/include -I$(LVGL_DIR)/$(LVGL_DIR_NAME)/src
EE_LDFLAGS = -L$(PS2DEV)/gsKit/lib
EE_LIBS = -ldmakit -lgskit -lpad

EE_NEWLIB_NANO = 0
EE_COMPACT_EXECUTABLE = 1

# Benchmark
EE_OBJS += test01.o
EE_BIN_NAME = test01

# Widgets
#EE_OBJS += test02.o
#EE_BIN_NAME = test02

# Simple self made
#EE_OBJS += test03.o
#EE_BIN_NAME = test03

EE_BIN = $(EE_BIN_NAME)_unpacked.elf
EE_BIN_PACKED = $(EE_BIN_NAME).elf

$(EE_BIN_PACKED): $(EE_BIN)
	ps2-packer $< $@ > /dev/null

lvgl:
	git clone --depth 1 -b release/v8.3 https://github.com/lvgl/lvgl

all: lvgl $(EE_BIN_PACKED)

run: all
	ps2client -h 192.168.1.10 execee host:$(EE_BIN_PACKED) -bsd=udpbd -dvd=mass:$(GAME)

sim: all
	flatpak --filesystem=host run net.pcsx2.PCSX2 $(PWD)/$(EE_BIN_PACKED)

clean:
	rm -rf $(EE_OBJS) *_irx.o *_elf.o
	rm -rf $(EE_BIN) $(EE_BIN_PACKED)
	rm -rf modules

include Defs.make
include Rules.make
