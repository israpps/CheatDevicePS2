#
# Cheat Device for PlayStation 2
# by root670
#

DTL_T10000 ?= 0
EXFAT ?= 0
HOMEBREW_IRX ?= 1 #wether to use or not homebrew IRX for pad, memcard and SIO2. if disabled. rom0: drivers will be used. wich is not a safe option. as it makes using the program on protokernel PS2 dangerous (at least for memcard I/O)
PRINTF = NONE
RELDIR = release
EE_BIN = CheatDevice$(HAS_EXFAT)$(HAS_HDD).ELF
# For minizip
EE_CFLAGS += -DUSE_FILE32API

# Helper libraries
OBJS += src/libraries/upng.o src/libraries/ini.o \
    src/libraries/minizip/ioapi.o src/libraries/minizip/zip.o \
    src/libraries/minizip/unzip.o src/libraries/lzari.o

# Main
OBJS += src/main.o src/objectpool.o src/hash.o src/pad.o \
    src/util.o src/startgame.o src/textcheats.o src/cheats.o \
    src/graphics.o src/saves.o src/menus.o src/settings.o

# Save Formats
OBJS += src/saveformats/util.o src/saveformats/cbs.o src/saveformats/psu.o src/saveformats/zip.o src/saveformats/max.o

# IRX Modules
IRX_OBJS += resources/usbd_irx.o
IRX_OBJS += resources/iomanX_irx.o
ifeq ($(HOMEBREW_IRX),1)
  IRX_OBJS += resources/sio2man_irx.o resources/mcman_irx.o resources/mcserv_irx.o resources/padman_irx.o
endif

ifeq ($(EXFAT),1)
  EE_CFLAGS += -DEXFAT
  HAS_EXFAT = -EXFAT
  IRX_OBJS += resources/bdm_irx.o resources/bdmfs_fatfs_irx.o resources/usbmass_bd_irx.o
else
  IRX_OBJS += resources/usbhdfsd_irx.o
endif

ifeq ($(HDD), 1)
  PRINTF = EE_SIO
  EE_CLFAGS += -O0 -g
  EE_LIBS += -lpoweroff
  IRX_OBJS += resources/ps2fs_irx.o resources/ps2hdd_irx.o resources/ps2atad_irx.o resources/poweroff_irx.o
  EE_CFLAGS += -DHDD
  FILEXIO_NEED = 1
  DEV9_NEED = 1
  HAS_HDD = -HDD
endif

ifeq ($(FILEXIO_NEED), 1)
  EE_CFLAGS += -DFILEXIO
  EE_LIBS += -lfileXio
  IRX_OBJS += resources/filexio_irx.o
endif

ifeq ($(DEV9_NEED), 1)
  EE_CFLAGS += -DDEV9
  IRX_OBJS += resources/ps2dev9_irx.o
endif


# Graphic resources
OBJS += resources/background_png.o \
    resources/check_png.o resources/hamburgerIcon_png.o resources/gamepad_png.o resources/cube_png.o \
    resources/savemanager_png.o resources/flashdrive_png.o resources/memorycard1_png.o resources/memorycard2_png.o \
    resources/buttonCross_png.o resources/buttonCircle_png.o resources/buttonTriangle_png.o \
    resources/buttonSquare_png.o resources/buttonStart_png.o resources/buttonSelect_png.o \
    resources/buttonL1_png.o resources/buttonL2_png.o resources/buttonL3_png.o \
    resources/buttonR1_png.o resources/buttonR2_png.o resources/buttonR3_png.o

# Engine
OBJS += engine/engine_erl.o

# Bootstrap ELF
OBJS += bootstrap/bootstrap_elf.o

ifeq ($(HOMEBREW_IRX),1)
	EE_LIBS += -lpadx
	EE_CFLAGS += -DHOMEBREW_IRX
else
	EE_LIBS += -lpad
endif
ifeq ($(DTL_T10000),1)
	EE_CFLAGS += -D_DTL_T10000 -g
endif

# Replace uses of "mass" with "host"
ifeq ($(NO_MASS),1)
	EE_CFLAGS += -D_NO_MASS
endif

GSKIT = $(PS2DEV)/gsKit
EE_LIBS += -lgskit_toolkit -lgskit -ldmakit -lc -lkernel -lmc -lpatches -lerl -lcdvd -lz -lmf
EE_LDFLAGS += -L$(PS2SDK)/ee/lib -L$(PS2SDK)/ports/lib -L$(GSKIT)/lib -s
EE_INCS += -I$(GSKIT)/include -I$(PS2SDK)/ports/include -Iinclude

EE_OBJS = $(IRX_OBJS) $(OBJS)

all: modules version main
	$(info Build finished)

modules:
	bin2o $(PS2SDK)/iop/irx/iomanX.irx resources/iomanX_irx.o _iomanX_irx
	bin2o $(PS2SDK)/iop/irx/usbd.irx resources/usbd_irx.o _usbd_irx
ifeq ($(EXFAT),1)
	bin2o iop/bdm.irx resources/bdm_irx.o _bdm_irx
	bin2o iop/bdmfs_fatfs.irx resources/bdmfs_fatfs_irx.o _bdmfs_fatfs_irx
	bin2o iop/usbmass_bd.irx resources/usbmass_bd_irx.o _usbmass_bd_irx
else
	bin2o $(PS2SDK)/iop/irx/usbhdfsd.irx resources/usbhdfsd_irx.o _usbhdfsd_irx
endif
ifeq ($(HOMEBREW_IRX),1)
	bin2o $(PS2SDK)/iop/irx/freesio2.irx resources/sio2man_irx.o _sio2man_irx
	bin2o $(PS2SDK)/iop/irx/mcman.irx resources/mcman_irx.o _mcman_irx
	bin2o $(PS2SDK)/iop/irx/mcserv.irx resources/mcserv_irx.o _mcserv_irx
	bin2o $(PS2SDK)/iop/irx/freepad.irx resources/padman_irx.o _padman_irx
endif
ifeq ($(FILEXIO_NEED), 1)
	bin2o $(PS2SDK)/iop/irx/fileXio.irx resources/filexio_irx.o _filexio_irx
endif
ifeq ($(DEV9_NEED), 1)
	bin2o $(PS2SDK)/iop/irx/ps2dev9.irx resources/ps2dev9_irx.o _ps2dev9_irx
endif
ifeq ($(HDD), 1)
	bin2o $(PS2SDK)/iop/irx/ps2fs.irx resources/ps2fs_irx.o _ps2fs_irx
	bin2o $(PS2SDK)/iop/irx/ps2hdd-osd.irx resources/ps2hdd_irx.o _ps2hdd_irx
	bin2o $(PS2SDK)/iop/irx/ps2atad.irx resources/ps2atad_irx.o _ps2atad_irx
	bin2o $(PS2SDK)/iop/irx/poweroff.irx resources/poweroff_irx.o _poweroff_irx
endif

	@# Graphics
	@bin2o resources/background.png resources/background_png.o _background_png
	@bin2o resources/check.png resources/check_png.o _check_png
	@bin2o resources/hamburgerIcon.png resources/hamburgerIcon_png.o _hamburgerIcon_png

	@bin2o resources/gamepad.png resources/gamepad_png.o _gamepad_png
	@bin2o resources/cube.png resources/cube_png.o _cube_png
	@bin2o resources/savemanager.png resources/savemanager_png.o _savemanager_png
	@bin2o resources/flashdrive.png resources/flashdrive_png.o _flashdrive_png
	@bin2o resources/memorycard1.png resources/memorycard1_png.o _memorycard1_png
	@bin2o resources/memorycard2.png resources/memorycard2_png.o _memorycard2_png
	@bin2o resources/buttonCross.png resources/buttonCross_png.o _buttonCross_png
	@bin2o resources/buttonCircle.png resources/buttonCircle_png.o _buttonCircle_png
	@bin2o resources/buttonTriangle.png resources/buttonTriangle_png.o _buttonTriangle_png
	@bin2o resources/buttonSquare.png resources/buttonSquare_png.o _buttonSquare_png
	@bin2o resources/buttonStart.png resources/buttonStart_png.o _buttonStart_png
	@bin2o resources/buttonSelect.png resources/buttonSelect_png.o _buttonSelect_png
	@bin2o resources/buttonL1.png resources/buttonL1_png.o _buttonL1_png
	@bin2o resources/buttonL2.png resources/buttonL2_png.o _buttonL2_png
	@bin2o resources/buttonL3.png resources/buttonL3_png.o _buttonL3_png
	@bin2o resources/buttonR1.png resources/buttonR1_png.o _buttonR1_png
	@bin2o resources/buttonR2.png resources/buttonR2_png.o _buttonR2_png
	@bin2o resources/buttonR3.png resources/buttonR3_png.o _buttonR3_png

	@# Engine
	@cd engine && $(MAKE)
	@bin2o engine/engine.erl engine/engine_erl.o _engine_erl

	@# Bootstrap
	@cd bootstrap && $(MAKE)
	@bin2o bootstrap/bootstrap.elf bootstrap/bootstrap_elf.o _bootstrap_elf

version:
	@echo -n '#define GIT_VERSION "'> src/version.h
	@git rev-parse --short HEAD | tr -d '\n'>> src/version.h
	@echo '"'>> src/version.h

main: $(EE_BIN)

$(RELDIR): all
	rm -rf $(RELDIR)
	mkdir $(RELDIR)
	mkdir $(RELDIR)/extra_cheats/
	cp extra_cheats/*.zip $(RELDIR)/extra_cheats/
	ps2-packer $(EE_BIN) $(RELDIR)/$(EE_BIN)
	zip -q -9 $(RELDIR)/CheatDatabase.zip CheatDatabase.txt
	cp CheatDevicePS2.ini LICENSE README.md $(RELDIR)
	sed -i 's/CheatDatabase.txt/CheatDatabase.zip/g' $(RELDIR)/CheatDevicePS2.ini
	cd $(RELDIR) && zip -q -9 CheatDevicePS2$(HAS_EXFAT)$(HAS_HDD).zip * extra_cheats/*.zip

clean:
	rm -rf src/*.o src/libraries/*.o src/libraries/minizip/*.o src/saveformats/*.o $(EE_BIN) $(RELDIR)/$(EE_BIN)
	rm -f resources/*.o
	cd engine && make clean
	cd bootstrap && make clean

rebuild: clean all

ifeq ($(PRINTF),EE_SIO)
  EE_CFLAGS += -DEE_SIO
endif
ifeq ($(PRINTF),COMMON)
  EE_CFLAGS += -DCOMMON_PRINTF
endif

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
