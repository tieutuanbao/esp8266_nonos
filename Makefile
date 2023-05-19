# brief:		Makefile cho esp8266
# author:		Tiêu Tuấn Bảo
# create at:	22/04/2022

APP_NAME	:= firmware_FireworkESP
Q			:= @

# Chọn kiểu gen (0=flash.bin+irom0text.bin, 1=app1.bin, 2=app2.bin)
APP				?= 1
# Chọn tốc độ SPI Flash (12m, 15m, 16m, 20m, 24m, 26m, 26m, 30m, 40m, 48m, 60m, 80m, keep)
SPI_SPEED		?= 80m
# Chọn chế độ SPI (qio, qout, dio, dout, keep)
SPI_MODE		?= qio
# Chọn kích thước SPI SPI Flash size in MegaBytes (512KB, 2MB-c1, 6=4MB-c1, detect, keep)
SPI_SIZE		:= 4

ifeq ($(APP), 0)
APP_BUILD_BOOT_ADDR	:= 0x00000
APP_ADDR			:= 0x10000
endif
ifeq ($(APP), 1)
APP_BUILD_ADDR		:= 0x02000
endif
ifeq ($(APP), 2)
APP_BUILD_ADDR		:= 0x82000
endif

# Cấu hình cho Compiler
SDK_BASE	?= ESP8266_NONOS_SDK/
PYTHON		?= tools/python2/App/Python/python.exe
ESPTOOL		?= tools/esptool/esptool.py
MONITORTOOL	?= tools/idf_monitor.py
BUILDFSTOOL	?= tools/tool-mkspiffs/mkspiffs.exe

XTENSA_DIR		?= D:/1.Software/Compiler/xtensa-lx106-elf/bin/
XTENSA_PREFIX	?= $(XTENSA_DIR)xtensa-lx106-elf-
CC			:= $(XTENSA_PREFIX)gcc
LD			:= $(XTENSA_PREFIX)gcc
AR			:= $(XTENSA_PREFIX)ar
OBJCOPY		:= $(XTENSA_PREFIX)objcopy
OBJDUMP		:= $(XTENSA_PREFIX)objdump
ADDR2LINE	:= $(XTENSA_PREFIX)addr2line

# Cấu hình File System
FS_SOURCE	= ./data
FS_END 		= 0x3FA000
FS_START	= 0x300000
FS_SZ		= $(shell echo $$(($(FS_END) - $(FS_START))))
FS_BLOCK_SZ	= 8192
FS_PAGE_SZ	= 256

# Cấu hình cổng com
FLASH_PORT		:= COM12

# Cấu hình tốc độ serial debug
SERIAL_BAUD		:= 76800

# Cấu hình tốc độ flash
FLASH_BAUD		:= 921600

# Cấu hình ESPTOOL
ESPTOOL_OPT		:= --chip esp8266 --port $(FLASH_PORT) --baud $(FLASH_BAUD) --before default_reset --after hard_reset
# Cấu hình flash
ESPTOOL_ARGS	:= --flash_freq $(SPI_SPEED) --flash_mode $(SPI_MODE) --flash_size 4MB

# Add source và include vào project
SRCS		:= ./source/rf_init.c
SRCS		+= ./source/main.c
SRCS		+= ./mcu_xlibs/misc/bits_string.c
SRCS		+= ./mcu_xlibs/misc/bits_math.c
SRCS		+= ./mcu_xlibs/misc/color.c
SRCS		+= ./mcu_xlibs/arch/tensilica/l106/esp8266/driver/esp8266_gpio.c
SRCS		+= ./mcu_xlibs/arch/tensilica/l106/esp8266/driver/esp8266_iomux.c
SRCS		+= ./mcu_xlibs/arch/tensilica/l106/esp8266/driver/esp8266_i2s.c
SRCS		+= ./mcu_xlibs/arch/tensilica/l106/esp8266/driver/esp8266_slc.c
SRCS		+= ./mcu_xlibs/arch/tensilica/l106/esp8266/driver/esp8266_spi.c
SRCS		+= ./mcu_xlibs/arch/tensilica/l106/esp8266/libs/i2s_dma/i2s_dma.c
# SRCS		+= ./mcu_xlibs/arch/tensilica/l106/esp8266/libs/dns_server/dns_server.c
SRCS		+= ./source/libs/src/i2s.c
SRCS		+= ./source/http_parser/http_parser.c
SRCS		+= ./source/http_server/http_server.c
SRCS		+= ./source/websocket_parser/websocket_parser.c
SRCS		+= ./source/ucs1903/ucs1903.c
SRCS		+= ./source/lpd6803/lpd6803.c
SRCS		+= ./source/strip_led/meteor.c
SRCS		+= ./source/firework/firework.c
SRCS		+= ./source/jsmn/jsmn.c
SRCS		+= ./source/realtime/realtime.c
SRCS		+= ./mcu_xlibs/components/audio/audio_generator_wav.c
SRCS		+= ./mcu_xlibs/components/audio/audio_output_i2s_noDAC.c
SRCS		+= ./mcu_xlibs/components/sha1/sha1.c
SRCS		+= ./mcu_xlibs/components/base64/base64.c
SRCS		+= ./source/spiffs/fs.c
SRCS		+= ./source/spiffs/spiffs/src/spiffs_cache.c
SRCS		+= ./source/spiffs/spiffs/src/spiffs_check.c
SRCS		+= ./source/spiffs/spiffs/src/spiffs_gc.c
SRCS		+= ./source/spiffs/spiffs/src/spiffs_hydrogen.c
SRCS		+= ./source/spiffs/spiffs/src/spiffs_nucleus.c
SRCS		+= ./source/ezform/ezform.c
SRCS		+= ./source/web_binary/update_html.c
SRCS		+= ./source/web_binary/logo_png.c
SRCS		+= ./source/web_binary/firework_bg_js.c

INC			:= $(SDK_BASE)include
INC			+= $(SDK_BASE)driver_lib/include/driver
INC			+= $(SDK_BASE)third_party/include
INC			+= ./source
INC			+= ./mcu_xlibs/misc
INC			+= ./mcu_xlibs/arch/tensilica/l106/esp8266/include/driver
# INC			+= ./mcu_xlibs/arch/tensilica/l106/esp8266/libs/dns_server
INC			+= ./mcu_xlibs/arch/tensilica/l106/esp8266/libs/i2s_dma
INC			+= ./source/libs/inc
INC			+= ./mcu_xlibs/misc
INC			+= ./source/http_parser
INC			+= ./source/http_server
INC			+= ./source/websocket_parser
INC			+= ./source/ucs1903
INC			+= ./source/lpd6803
INC			+= ./source/strip_led
INC			+= ./source/firework
INC			+= ./source/jsmn
INC			+= ./mcu_xlibs/components/audio
INC			+= ./mcu_xlibs/components/sha1
INC			+= ./mcu_xlibs/components/base64
INC			+= ./source/spiffs
INC			+= ./source/spiffs/spiffs/src
INC			+= ./source/ezform
INC			+= ./source/realtime
INC			+= ./source/bboot/source
INC			+= ./source/web_binary

# Define
DEFINE		:= ICACHE_FLASH
DEFINE		+= __ets__
DEFINE		+= F_CPU=160000000
DEFINE		+= SPI_FLASH_SIZE_MAP=$(SPI_SIZE)
DEFINE		+= MEM_DEFAULT_USE_DRAM
DEFINE		+= LWIP_OPEN_SRC
DEFINE		+= MEM_LIBC_MALLOC=0
DEFINE		+= SPIFFS_LOG_PAGE_SIZE=$(FS_PAGE_SZ)
DEFINE		+= SPIFFS_LOG_BLOCK_SIZE=$(FS_BLOCK_SZ)
DEFINE		+= SPIFFS_BASE_ADDR=$(FS_START)
DEFINE		+= SPIFFS_SIZE=$(FS_SZ)
DEFINE		+= APP0_BOOT_ADDR=$(APP_BOOT_ADDR)
DEFINE		+= APP0_ADDR=$(APP_ADDR)
DEFINE		+= BUILD_TIME=\"$(DATETIME)\"

# Đường dẫn lưu các file sau build
OBJS_DIR	:= build/obj/
BIN_DIR		:= build/bin/
OUT_DIR		:= build/out/

OBJS		:= $(addprefix $(OBJS_DIR), $(patsubst  %.c, %.o, $(notdir $(SRCS))))
DATETIME	:= $(shell date "+%Y-%b-%d_%H:%M:%S_%Z")

CCFLAGS :=	-Os	\
			-g \
			-Wpointer-arith \
			-Wl,-EL \
			-fno-inline-functions \
			-nostdlib \
			-mlongcalls	\
			-mtext-section-literals \
			-ffunction-sections \
			-fdata-sections	\
			-fno-builtin-printf


# Linker
ifeq ($(APP), 0)
	LD_SCRIPT	= ./ld/default_32MB.ld
else
	LD_SCRIPT	= ./ld/default_32MB_app$(APP).ld
endif

LDFLAGS		= -Wl,--no-check-sections -Wl,--gc-sections -u call_user_start -Wl,-static

# Flag cho Compiler
SDK_LIBS 	:=	-L$(SDK_BASE)lib		\
				-nostdlib				\
				-T$(LD_SCRIPT)			\
				$(LDFLAGS)				\
				-Wl,--start-group		\
				-lc    				\
				-lgcc    			\
				-lhal				\
				-lphy				\
				-lpp    			\
				-lnet80211    		\
				-llwip	    		\
				-lmbedtls			\
				-lwpa    			\
				-lcrypto    		\
				-lmain    			\
				-ljson    			\
				-lssl    			\
				-lupgrade    		\
				-lsmartconfig 		\
				-lairkiss			\
				-ldriver


all: $(APP_NAME).bin

$(APP_NAME).bin: $(OBJS_DIR) $(OUT_DIR) $(BIN_DIR) $(APP_NAME).out
	$(Q)echo "------------------------------------"
	$(Q)rm -f -r $(BIN_DIR)eagle.S $(BIN_DIR)eagle.dump
	$(Q)$(OBJDUMP) -x -s $(OUT_DIR)$(APP_NAME).out > $(BIN_DIR)eagle.dump
	$(Q)$(OBJDUMP) -S $(OUT_DIR)$(APP_NAME).out > $(BIN_DIR)eagle.S
	$(Q)echo "Convert to BIN file"
	$(Q)echo "------------------------------------"
ifeq ($(APP), 0)
	$(Q)$(PYTHON) $(ESPTOOL) $(ESPTOOL_OPT) \
	elf2image $(ESPTOOL_ARGS) --version=1 \
	-o $(BIN_DIR)$(APP_NAME) $(OUT_DIR)$(APP_NAME).out
else
	$(Q)$(PYTHON) $(ESPTOOL) $(ESPTOOL_OPT) \
	elf2image $(ESPTOOL_ARGS) --version=2 \
	-o $(BIN_DIR)$(APP_NAME)_app$(APP).bin $(OUT_DIR)$(APP_NAME).out
	$(Q)echo $(BIN_DIR)$(APP_NAME)_app$(APP).bin "-------->" $(APP_BUILD_ADDR)
endif
	$(Q)echo "Build done!"
	
$(APP_NAME).out: libuser.a
	$(Q)echo "------------------------------------"
	$(Q)echo "Linking $@"
	$(Q)$(LD) $(SDK_LIBS) $(OUT_DIR)$< -Wl,--end-group -o $(OUT_DIR)$@

libuser.a: $(patsubst  %.c, %.o, $(SRCS))
	$(Q)echo "------------------------------------"
	$(Q)echo "Compress all obj to" $@
	$(Q)$(AR) ru $(OUT_DIR)$@ $(OBJS)

# Tạo file module file.o từ file.c
%.o: $(or %.c,%.s)
	$(Q)echo "Compiling: $@"
	$(Q)$(CC) $(CCFLAGS) $(addprefix -D,$(DEFINE)) $(addprefix -I,$(INC)) -o $(OBJS_DIR)$(notdir $@) -c $<

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)

$(OUT_DIR):
	@mkdir -p $(OUT_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Clean project
clean:
	$(Q)echo "------------------------------------"
	@echo "Clean Project"
	@rm -rf build
	@echo "Done!"
	$(Q)echo ""

# Nạp chương trình
flash:
	$(Q)echo "------------------------------------"
	$(Q)echo "Flashing ESP8266 ... "
ifeq ($(APP), 0)
	$(Q)$(PYTHON) $(ESPTOOL) --chip esp8266 --port $(FLASH_PORT) --baud $(FLASH_BAUD) --before default_reset --after hard_reset \
	write_flash $(ESPTOOL_ARGS) --compress \
	0x3FB000 $(SDK_BASE)bin/blank.bin \
	0x3FC000 $(SDK_BASE)bin/esp_init_data_default_v08.bin \
	0x3FE000 $(SDK_BASE)bin/blank.bin \
	$(APP_BOOT_ADDR) $(BIN_DIR)$(APP_NAME)0x00000.bin \
	$(APP_ADDR) $(BIN_DIR)$(APP_NAME)$(APP_ADDR).bin
else
	$(Q)$(PYTHON) $(ESPTOOL) --chip esp8266 --port $(FLASH_PORT) --baud $(FLASH_BAUD) --before default_reset --after hard_reset \
	write_flash $(ESPTOOL_ARGS) --compress \
	0x3FB000 $(SDK_BASE)bin/blank.bin \
	0x3FC000 $(SDK_BASE)bin/esp_init_data_default_v08.bin \
	0x3FE000 $(SDK_BASE)bin/blank.bin \
	0x00000 source/bboot/build/bin/bboot0x00000.bin \
	$(APP_BUILD_ADDR) $(BIN_DIR)$(APP_NAME)_app$(APP).bin
endif
	$(Q)echo ""

read_flash:
	$(Q)echo "------------------------------------"
	$(Q)echo "Reading ESP8266 ... "
	$(Q)$(PYTHON) $(ESPTOOL) --chip esp8266 --port $(FLASH_PORT) --baud $(FLASH_BAUD) --before default_reset --after hard_reset \
	read_flash 0x0000 0x400000 flash_data.bin
	$(Q)echo "--- Read file Done!!! ---"
	$(Q)echo "Flash data save in file: flash_data.bin"
	
# Erase
erase:
	$(Q)echo "------------------------------------"
	$(Q)echo "Erase flash ESP8266 ... "
	$(Q)$(PYTHON) $(ESPTOOL) --chip esp8266 --port $(FLASH_PORT) \
	--baud $(FLASH_BAUD) \
	erase_flash
	$(Q)echo ""

uploadfs:
	$(Q)echo "------------------------------------"
	$(Q)echo "Uploading File System ... "
	$(Q)$(PYTHON) $(ESPTOOL) --chip esp8266 --port $(FLASH_PORT) \
	--baud $(FLASH_BAUD) \
	write_flash $(ESPTOOL_ARGS) \
	$(FS_START) $(BIN_DIR)data.bin
	$(Q)echo ""

buildfs:
	$(Q)echo "------------------------------------"
	$(Q)echo "Building File System ... "
	$(Q)$(BUILDFSTOOL) --create $(FS_SOURCE) --page $(FS_PAGE_SZ) --block $(FS_BLOCK_SZ) --size $(FS_SZ) -- $(BIN_DIR)data.bin
	$(Q)echo ""
	
# monitor
monitor:
	$(Q)$(PYTHON) $(MONITORTOOL) --port $(FLASH_PORT) --toolchain-prefix $(XTENSA_PREFIX) --baud $(SERIAL_BAUD) --eol CRLF $(OUT_DIR)$(APP_NAME).out

# Không cần quan tâm
.PHONY: all clean flash unbrick monitor