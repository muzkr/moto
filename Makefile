
PROJECT = moto

ENABLE_LOGGING ?= 0
VERSION_STRING ?= 1.0.0-preview1


TARGET = $(PROJECT)_$(VERSION_STRING)

######################################
# building variables
######################################
# debug build?
DEBUG = 0
# optimization
OPT = -Oz


#######################################
# paths
#######################################
# Build path
BUILD_DIR = build

######################################
# source
######################################
# C sources
C_SOURCES = \
src/main.c \
src/systick.c \
src/flashlight.c \
src/usbd_msc_impl.c \
src/dfu.c \
src/py32f071_it.c \
src/system_py32f071.c \
Middlewares/CherryUSB/core/usbd_core.c \
Middlewares/CherryUSB/port/usb_dc_py32.c \
Middlewares/CherryUSB/class/msc/usbd_msc.c \
Drivers/PY32F071_HAL_Driver/Src/py32f071_ll_dma.c \
Drivers/PY32F071_HAL_Driver/Src/py32f071_ll_gpio.c \
Drivers/PY32F071_HAL_Driver/Src/py32f071_ll_rcc.c \
Drivers/PY32F071_HAL_Driver/Src/py32f071_ll_pwr.c \
Drivers/PY32F071_HAL_Driver/Src/py32f071_ll_usart.c \
Drivers/PY32F071_HAL_Driver/Src/py32f071_ll_utils.c


# ASM sources
ASM_SOURCES = \
startup_py32f071xx.s


#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 
#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m0plus

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
AS_DEFS = 

# C defines
C_DEFS = \
-DPY32F071xB \
-DUSE_FULL_LL_DRIVER \
-DVERSION_STRING=\"$(VERSION_STRING)\"

# AS includes
AS_INCLUDES = 

# C includes
C_INCLUDES = \
-Isrc \
-IDrivers/PY32F071_HAL_Driver/Inc \
-IDrivers/CMSIS/Device/PY32F071/Include \
-IDrivers/CMSIS/Include \
-IMiddlewares/CherryUSB/port \
-IMiddlewares/CherryUSB/core \
-IMiddlewares/CherryUSB/common \
-IMiddlewares/CherryUSB/class/msc


ifeq ($(ENABLE_LOGGING),1)
C_SOURCES += \
	src/log.c \
	Utilities/printf/printf.c \
	Utilities/lwrb-3.2.0/lwrb/src/lwrb/lwrb.c
C_INCLUDES += \
	-IUtilities/printf \
	-IUtilities/lwrb-3.2.0/lwrb/src/include
C_DEFS += -DENABLE_LOGGING
endif


# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

# CFLAGS += $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS += -std=gnu11 $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -flto=auto

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif


# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"


#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = py32f071xb.ld

# libraries
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections \
	-Wl,--print-memory-usage

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin


#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	
$(BUILD_DIR):
	mkdir $@		

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)
  
#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***