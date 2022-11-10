EE_BIN = fobesmark.elf
EE_OBJS = main.o ui/ui.o fontengine/fontengine.o 
EE_OBJS += font_pallete_tex.o font_tex.o
EE_OBJS += pad.o padman.o sio2man.o
EE_OBJS += tests/emulation.o tests/vu/vu.o
EE_LIBS = -lstdc++ -lkernel -lpatches -lgraph -ldma -ldraw -lpad
EE_INCS += -Ifontengine/

EE_DVP = dvp-as

# Git version
GIT_VERSION := "$(shell git describe --abbrev=4 --always --tags)"

EE_CXXFLAGS = -I$(shell pwd) -Werror -DGIT_VERSION="\"$(GIT_VERSION)\"" -fno-exceptions

all: $(EE_BIN)

padman.s: $(PS2SDK)/iop/irx/padman.irx
	bin2s $< $@ padman_irx

sio2man.s: $(PS2SDK)/iop/irx/sio2man.irx
	bin2s $< $@ sio2man_irx

font_pallete_tex.s: font/font_pallete_tex.raw
	bin2s $< $@ font_pallete_tex

font_tex.s: font/font_tex.raw
	bin2s $< $@ font_tex

%.o: %.vsm
	$(EE_DVP) $< -o $@

clean:
	rm -f $(EE_OBJS)

run: $(EE_BIN)
	ps2client execee host:$(EE_BIN)

wsl: $(EE_BIN)
	$(PCSX2) --elf="$(shell wslpath -w $(shell pwd))/$(EE_BIN)"

emu: $(EE_BIN)
	$(PCSX2) --elf="$(shell pwd)/$(EE_BIN)"

reset:
	ps2client reset
	ps2client netdump

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
