# brief:		Makefile cho esp8266
# author:		Tiêu Tuấn Bảo
# create at:	22/04/2022

APP_NAME	:= bboot

Q			:= @
# Chọn tốc độ SPI Flash (12m, 15m, 16m, 20m, 24m, 26m, 26m, 30m, 40m, 48m, 60m, 80m, keep)
SPI_SPEED		?= 80m
# Chọn chế độ SPI (qio, qout, dio, dout, keep)
SPI_MODE		?= qio
# Chọn kích thước SPI SPI Flash size in MegaBytes (512KB, 2MB-c1, 6=4MB-c1, detect, keep)
SPI_SIZE		:= 4

# Cấu hình cho Compiler
SDK_BASE	?= ../../ESP8266_NONOS_SDK/
ESPTOOL		?= ../../tools/esptool/esptool.py
ESPTOOL2	?= ../../tools/esptool2/esptool2
MONITORTOOL	?= ../../tools/idf_monitor.py
GENAPPTOOL	?= ../../tools/gen_appbin.py
BUILDFSTOOL	?= ../../tools/tool-mkspiffs/mkspiffs.exe

XTENSA_DIR	?= C:/msys32/opt/xtensa-lx106-elf/bin/
CC			:= $(XTENSA_DIR)xtensa-lx106-elf-gcc
LD			:= $(XTENSA_DIR)xtensa-lx106-elf-gcc
AR			:= $(XTENSA_DIR)xtensa-lx106-elf-ar
OBJCOPY		:= $(XTENSA_DIR)xtensa-lx106-elf-objcopy
OBJDUMP		:= $(XTENSA_DIR)xtensa-lx106-elf-objdump
ADDR2LINE	:= $(XTENSA_DIR)xtensa-lx106-elf-addr2line

# Cấu hình cổng com
FLASH_PORT	:= COM4
# Cấu hình tốc độ serial debug
SERIAL_BAUD	:= 76800
# Cấu hình tốc độ flash
FLASH_BAUD	:= 921600

# Cấu hình flash
ESPTOOL_ARGS	:= --flash_freq $(SPI_SPEED) --flash_mode $(SPI_MODE) --flash_size 4MB

# Add source và include vào project
SRCS		:= ./source/bboot.c

INC			:= ./source
INC			+= ./build

# Define
DEFINE		:= __ets__
DEFINE		+= ICACHE_FLASH

# Đường dẫn lưu các file sau build
OBJS_DIR	:= build/obj/
BIN_DIR		:= build/bin/
OUT_DIR		:= build/out/

OBJS		:= $(addprefix $(OBJS_DIR), $(patsubst  %.c, %.o, $(notdir $(SRCS))))

CCFLAGS :=	-Os	\
			-Wpointer-arith \
			-Wundef \
			-Werror \
			-Wl,-EL \
			-fno-inline-functions \
			-mlongcalls	\
			-mtext-section-literals \
			-fno-builtin-printf


# Linker
LD_BBOOT_SCRIPT		= ld/eagle.app.v6.ld
LD_LOADER_SCRIPT	= ld/loader.ld

LDFLAGS		= -Wl,--no-check-sections -Wl,--gc-sections -u call_user_start -Wl,-static

# Flag cho Compiler
SDK_LIBS 	:=	-L$(SDK_BASE)lib		\
				-T$(LD_BBOOT_SCRIPT)	\
				$(LDFLAGS)				\
				-Wl,--start-group		\
				-lc    					\
				-lgcc    				\
				
all: $(OBJS_DIR) $(OUT_DIR) $(BIN_DIR) build/loader.h $(BIN_DIR)bboot.bin

$(OBJS_DIR):
	$(Q)mkdir -p $(OBJS_DIR)
$(OUT_DIR):
	$(Q)mkdir -p $(OUT_DIR)
$(BIN_DIR):
	$(Q)mkdir -p $(BIN_DIR)

# BUILD Loader
build/loader.h: $(OUT_DIR)loader.elf
	$(Q)echo "Create loader.h"
	$(Q)$(ESPTOOL2) -quiet -header $^ $@ .text
	$(Q)rm $(OBJS_DIR)loader.*
	$(Q)rm $(OUT_DIR)loader.*
$(OUT_DIR)loader.elf: $(OBJS_DIR)loader.o
	$(Q)$(LD) -L$(SDK_BASE)lib -T$(LD_LOADER_SCRIPT) -nostdlib -u call_user_start -Wl,-static -Wl,--start-group -lc -lgcc $< -Wl,--end-group -o $@
$(OBJS_DIR)loader.o: source/loader.c
	$(Q)$(CC) -Os -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DICACHE_FLASH -Isource -c ./source/loader.c -o $(OBJS_DIR)$(notdir $@)

# BUILD bboot
$(BIN_DIR)bboot.bin: $(OUT_DIR)bboot.out
	$(Q)echo ""
	$(Q)mkdir -p $(BIN_DIR)
	$(Q)rm -f -r $(BIN_DIR)eagle.S $(BIN_DIR)eagle.dump
	$(Q)$(OBJDUMP) -x -s $< > $(BIN_DIR)eagle.dump
	$(Q)$(OBJDUMP) -S $< > $(BIN_DIR)eagle.S
	$(Q)echo ""
	$(Q)python $(ESPTOOL) $(ESPTOOL_OPT) \
	elf2image $(ESPTOOL_ARGS) --version=1 \
	-o $(BIN_DIR)bboot $^
	
$(OUT_DIR)bboot.out: $(OBJS_DIR)bboot.a
	$(Q)$(LD) $(SDK_LIBS) $< -Wl,--end-group -o $@

# Tạo main.a từ tất cả các file .o trong $(OBJS_DIR)
$(OBJS_DIR)bboot.a: $(OBJS_DIR)bboot.o
	$(Q)mkdir -p $(OUT_DIR)
	$(Q)$(AR) ru $@ $<

# Tạo file module file.o từ file.c
$(OBJS_DIR)bboot.o: $(SRCS)
	$(Q)mkdir -p $(OBJS_DIR)
	$(Q)$(CC) $(CCFLAGS) $(addprefix -D,$(DEFINE)) $(addprefix -I,$(INC)) -o $@ -c $<

# Clean project
clean:
	$(Q)rm -rf build
	$(Q)echo "Clean build"

# Nạp chương trình
flash:
	$(Q)python $(ESPTOOL) --chip esp8266 --port $(FLASH_PORT) --baud $(FLASH_BAUD) --before default_reset --after hard_reset \
	write_flash $(FLASHDEF) \
	0x00000 $(BIN_DIR)$(APP_NAME)0x00000.bin
	
# Erase
erase:
	$(Q)python $(ESPTOOL) --chip esp8266 --port $(FLASH_PORT) \
	--baud $(FLASH_BAUD) \
	erase_flash

# monitor
monitor:
	$(Q)python $(MONITORTOOL) --port $(FLASH_PORT) --baud $(SERIAL_BAUD) --eol CRLF $(OUT_DIR)$(APP_NAME).out

# Không cần quan tâm
.PHONY: all clean flash unbrick monitor