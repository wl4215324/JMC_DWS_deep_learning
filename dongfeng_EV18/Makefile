BUILD_ROOT = /home/tony/eclipse-workspace/t7_toolchain/hf/host/usr/bin/

OBJ_SRCS := 
ASM_SRCS := 
C_SRCS := 
O_SRCS := 
S_UPPER_SRCS := 
EXECUTABLES := 
OBJS :=
C_DEPS := 


CC = $(BUILD_ROOT)arm-linux-gnueabihf-gcc
CPP = $(BUILD_ROOT)arm-linux-gnueabihf-g++
AR  = $(BUILD_ROOT)arm-linux-gnueabihf-ar
STRIP = $(BUILD_ROOT)arm-linux-gnueabihf-strip

RM := rm -rf

SERIAL_AND_BT_APP_DIR = $(shell pwd)/SerialCommAndBootloader
SHM_COMM_DIR = $(shell pwd)/ShmCommon
VIDEO_ALGORITHM_APP_DIR = $(shell pwd)/VideoAndAlgorithm
BIN_TARGET_DIR = $(shell pwd)/bin

INC_DIR := -I$(SERIAL_AND_BT_APP_DIR)/Src/
INC_DIR += -I$(SHM_COMM_DIR)/

export CC CPP AR STRIP INC_DIR CUR_DIR LIB_DIR LIBS RM SERIAL_AND_BT_APP_DIR SHM_COMM_DIR \
VIDEO_ALGORITHM_APP_DIR BIN_TARGET_DIR

.PHONY ?= all


all: $(OBJS)
	cd $(SHM_COMM_DIR) && make
	cd $(SERIAL_AND_BT_APP_DIR) && make
	cd $(VIDEO_ALGORITHM_APP_DIR) && make
#	$(CC) -o start_app start_app.o
	
	
clean:
	cd $(SHM_COMM_DIR) && make clean
	cd $(SERIAL_AND_BT_APP_DIR) && make clean
	cd $(VIDEO_ALGORITHM_APP_DIR) && make clean
#	$(RM) *.o *.d start_app
	
%.o: %c
	$(CC) -c $@ $<

	
.PHONY += clean

