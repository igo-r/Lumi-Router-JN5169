###############################################################################
#
# MODULE:       Makefile
#
# DESCRIPTION:  Makefile for the Lumi Router
#
###############################################################################
#
# This software is owned by NXP B.V. and/or its supplier and is protected
# under applicable copyright laws. All rights are reserved. We grant You,
# and any third parties, a license to use this software solely and
# exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5179].
# You, and any third parties must reproduce the copyright and warranty notice
# and any other legend of ownership on each copy or partial copy of the
# software.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Copyright NXP B.V. 2017. All rights reserved
#
###############################################################################
# Application target name

TARGET = LumiRouter

###############################################################################
# Application build date

BUILD_DATE = 20210320
CFLAGS    += -DBUILD_DATE_STRING=\"$(BUILD_DATE)\"

###############################################################################
# Network settings

# Channel (0 for default channels)
SINGLE_CHANNEL ?= 0
CFLAGS         += -DSINGLE_CHANNEL=$(SINGLE_CHANNEL)

# Enabling High Power Mode on the Modules
# to support the zigbee module installed in the Aqara ZHWG11LM device
ENABLING_HIGH_POWER_MODE ?= 1
ifeq ($(ENABLING_HIGH_POWER_MODE), 1)
CFLAGS += -DENABLING_HIGH_POWER_MODE
endif

###############################################################################
# Target chip is the JN5169

JENNIC_CHIP        = JN5169
JENNIC_CHIP_FAMILY = JN516x

###############################################################################
# Select the network stack (e.g. MAC, ZBPro, ZCL)

JENNIC_STACK = ZCL

###############################################################################
# Default SDK is the IEEE802.15.4 SDK

JENNIC_SDK = JN-SW-4170

###############################################################################
# Default MAC is the IEEE802.15.4 Mini MAC

JENNIC_MAC = MiniMacShim

###############################################################################
# ZBPro Stack specific options

ZBPRO_DEVICE_TYPE = ZCR
PDM_BUILD_TYPE    =_EEPROM

STACK_SIZE        = 5000
MINIMUM_HEAP_SIZE = 2000

ZNCLKCMD = AppBuildZBPro.ld
ENDIAN   = BIG_ENDIAN

###############################################################################
# Debug options

DEBUG ?= NONE

ifeq ($(DEBUG), UART1)
$(info Building with debug UART1 ...)
TRACE   = 1
CFLAGS += -DUART_DEBUGGING
CFLAGS += -DDBG_ENABLE
CLFAGS += -DDEBUG_BDB
CFLAGS += -DDEBUG_APP
CFLAGS += -DDEBUG_REPORT
CFLAGS += -DDEBUG_ZCL
CFLAGS += -DDEBUG_UART
CFLAGS += -DDEBUG_SERIAL
CFLAGS += -DDEBUG_DEVICE_TEMPERATURE
endif

###############################################################################
# BDB features – Enable as required

BDB_SUPPORT_NWK_STEERING ?= 1
BDB_SUPPORT_FIND_AND_BIND_TARGET ?= 1

###############################################################################
# Generate build file name

ifneq ($(SINGLE_CHANNEL), 0)
TARGET_FEATURES := $(TARGET_FEATURES)_CH$(SINGLE_CHANNEL)
endif

ifeq ($(DEBUG), UART1)
TARGET_FEATURES := $(TARGET_FEATURES)_DEBUG
endif

GENERATED_FILE_NAME = $(TARGET)$(TARGET_FEATURES)_$(BUILD_DATE)

###############################################################################
# Path definitions

# Use if application directory contains multiple targets
SDK_BASE_DIR = $(abspath ../../../sdk/$(JENNIC_SDK))
APP_BASE     = $(abspath ..)
APP_BLD_DIR  = $(APP_BASE)/Build
APP_SRC_DIR  = $(APP_BASE)/Source
UTIL_SRC_DIR = $(COMPONENTS_BASE_DIR)/ZigbeeCommon/Source
HW_SRC_DIR   = $(COMPONENTS_BASE_DIR)/HardwareAPI/Source

###############################################################################
# Application Source files

# Note: Path to source file is found using vpath below, so only .c filename is required
APPSRC  = irq_JN516x.S
APPSRC += portasm_JN516x.S
APPSRC += port_JN516x.c
APPSRC += pdum_gen.c
APPSRC += pdum_apdu.S
APPSRC += zps_gen.c
APPSRC += app_start.c
APPSRC += app_main.c
APPSRC += app_router_node.c
APPSRC += app_zcl_task.c
APPSRC += app_reporting.c
APPSRC += app_serial_commands.c
APPSRC += app_device_temperature.c
APPSRC += uart.c

APP_ZPSCFG = app.zpscfg

###############################################################################
# Standard Application header search paths

INCFLAGS += -I$(APP_SRC_DIR)
INCFLAGS += -I$(APP_SRC_DIR)/..

# Application specific include files
INCFLAGS += -I$(COMPONENTS_BASE_DIR)/ZCL/Include
INCFLAGS += -I$(COMPONENTS_BASE_DIR)/ZCIF/Include
INCFLAGS += -I$(COMPONENTS_BASE_DIR)/Xcv/Include/
INCFLAGS += -I$(COMPONENTS_BASE_DIR)/Recal/Include/
INCFLAGS += -I$(COMPONENTS_BASE_DIR)/MicroSpecific/Include
INCFLAGS += -I$(COMPONENTS_BASE_DIR)/ZigbeeCommon/Include
INCFLAGS += -I$(COMPONENTS_BASE_DIR)/HardwareAPI/Include

###############################################################################
# Optional stack features to pull relevant libraries into the build.

OPTIONAL_STACK_FEATURES = $(shell $(ZPSCONFIG) -n $(TARGET) -f $(APP_SRC_DIR)/$(APP_ZPSCFG) -y )

###############################################################################
# Configure for the selected chip or chip family

include $(SDK_BASE_DIR)/Chip/Common/Build/config.mk
include $(SDK_BASE_DIR)/Stack/Common/Build/config.mk
include $(SDK_BASE_DIR)/Components/BDB/Build/config.mk

###############################################################################

TEMP = $(APPSRC:.c=.o)
APPOBJS_TMP = $(TEMP:.S=.o)
APPOBJS := $(addprefix $(APP_BLD_DIR)/,$(APPOBJS_TMP))

###############################################################################
# Application dynamic dependencies

APPDEPS_TMP = $(APPOBJS_TMP:.o=.d)
APPDEPS := $(addprefix $(APP_BLD_DIR)/,$(APPDEPS_TMP))

###############################################################################
# Linker

# Add application libraries before chip specific libraries to linker so
# symbols are resolved correctly (i.e. ordering is significant for GCC)

APPLDLIBS := $(foreach lib,$(APPLIBS),$(if $(wildcard $(addprefix $(COMPONENTS_BASE_DIR)/Library/lib,$(addsuffix _$(JENNIC_CHIP).a,$(lib)))),$(addsuffix _$(JENNIC_CHIP),$(lib)),$(addsuffix _$(JENNIC_CHIP_FAMILY),$(lib))))
LDLIBS := $(APPLDLIBS) $(LDLIBS)
LDLIBS += JPT_$(JENNIC_CHIP)

###############################################################################
# Dependency rules

.PHONY: all clean
# Path to directories containing application source 
vpath % $(APP_SRC_DIR):$(ZCL_SRC_DIRS):$(ZCL_SRC):$(BDB_SRC_DIR):$(UTIL_SRC_DIR):$(HW_SRC_DIR)

all: $(APP_BLD_DIR)/$(GENERATED_FILE_NAME).bin

-include $(APPDEPS)
$(APP_BLD_DIR)/%.d:
	rm -f $*.o

$(APP_SRC_DIR)/pdum_gen.c $(APP_SRC_DIR)/pdum_gen.h: $(APP_SRC_DIR)/$(APP_ZPSCFG) $(PDUMCONFIG)
	$(info Configuring the PDUM ...)
	$(PDUMCONFIG) -z $(TARGET) -f $< -o $(APP_SRC_DIR)

$(APP_SRC_DIR)/zps_gen.c $(APP_SRC_DIR)/zps_gen.h: $(APP_SRC_DIR)/$(APP_ZPSCFG) $(ZPSCONFIG)
	$(info Configuring the Zigbee Protocol Stack ...)
	$(ZPSCONFIG) -n $(TARGET) -t $(JENNIC_CHIP) -l $(ZPS_NWK_LIB) -a $(ZPS_APL_LIB) -c $(TOOL_COMMON_BASE_DIR)/$(TOOLCHAIN_PATH) -f $< -o $(APP_SRC_DIR)

$(APP_BLD_DIR)/%.o: %.S
	$(info Assembling $< ...)
	$(CC) -c -o $(subst Source,Build,$@) $(CFLAGS) $(INCFLAGS) $< -MD -MF $(APP_BLD_DIR)/$*.d -MP
	@echo

$(APP_BLD_DIR)/%.o: %.c 
	$(info Compiling $< ...)
	$(CC) -c -o $(subst Source,Build,$@) $(CFLAGS) $(INCFLAGS) $< -MD -MF $(APP_BLD_DIR)/$*.d -MP
	@echo

$(APP_BLD_DIR)/$(GENERATED_FILE_NAME).elf: $(APPOBJS) $(addsuffix.a,$(addprefix $(COMPONENTS_BASE_DIR)/Library/lib,$(APPLDLIBS))) 
	$(info Linking $@ ...)
	$(CC) -Wl,--gc-sections -Wl,-u_AppColdStart -Wl,-u_AppWarmStart $(LDFLAGS) -L $(SDK_BASE_DIR)/Stack/ZCL/Build/ -T$(ZNCLKCMD) -o $@ -Wl,--start-group $(APPOBJS) $(addprefix -l,$(LDLIBS)) -lm -Wl,--end-group -Wl,-Map,$(GENERATED_FILE_NAME).map 
	$(SIZE) $@

$(APP_BLD_DIR)/$(GENERATED_FILE_NAME).bin: $(APP_BLD_DIR)/$(GENERATED_FILE_NAME).elf
	$(info Generating binary ...)
	$(OBJCOPY) -j .version -j .bir -j .flashheader -j .vsr_table -j .vsr_handlers -j .rodata -j .text -j .data -j .bss -j .heap -j .stack -S -O binary $< $@

###############################################################################

clean:
	rm -f $(APPOBJS) $(APPDEPS) $(TARGET)*_$(BUILD_DATE).bin $(TARGET)*_$(BUILD_DATE).elf $(TARGET)*_$(BUILD_DATE).map
	rm -f $(APP_SRC_DIR)/pdum_gen.* $(APP_SRC_DIR)/zps_gen.* $(APP_SRC_DIR)/pdum_apdu.S

###############################################################################
