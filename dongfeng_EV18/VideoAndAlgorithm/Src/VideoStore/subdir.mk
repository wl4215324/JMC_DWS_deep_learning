################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
./file_operation.c \
./files_manager.c \
./rtc_operations.c \
./timer.c \
./user_timer.c \
./basic_queue.c \
./queue_for_image.c \
./video_encode.c \
./warn_video_store.c

CPP_SRCS +=

OBJS += \
./Debug/file_operation.o \
./Debug/files_manager.o \
./Debug/rtc_operations.o \
./Debug/timer.o \
./Debug/user_timer.o \
./Debug/basic_queue.o \
./Debug/queue_for_image.o \
./Debug/video_encode.o \
./Debug/warn_video_store.o

C_DEPS += \
./Debug/file_operation.d \
./Debug/files_manager.d \
./Debug/rtc_operations.d \
./Debug/timer.d \
./Debug/user_timer.d \
./Debug/basic_queue.d \
./Debug/queue_for_image.d \
./Debug/video_encode.d \
./Debug/warn_video_store.d

CPP_DEPS +=

SDK_ROOT_DIR = /home/tony/t7_reference/t7linux-auto
          
T7_SDK_LIB_PATH := ${SDK_ROOT_DIR}/buildroot-201611/target/user_rootfs_misc/sdk_lib

VIDEO_STORE_INC_DIR := ${T7_SDK_LIB_PATH}/cedarx/include/libcedarc/include/
		
#CF = -std=c++11 -c -O3 -Wall -fpermissive -DCPU_ONLY 
#FAST = -ffast-math  -march=armv7-a -mfpu=vfpv4-d16 -mfloat-abi=hard -mcpu=cortex-a7 -fopenmp

#CXX_DEFINES := -DV4L2_NV21
#CXX_FLAGS := -std=c++11 -fpermissive
#
#C_DEFINES := -DV4L2_NV21
#C_FLAGS :=

#LIBS += -lMemAdapter -lsdk_g2d -lcdx_base -lvideoengine -lsdk_disp

# Each subdirectory must supply rules for building sources it contributes
./Debug/%.o: ./Src/VideoStore/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	$(CC) $(C_DEFINES) -I$(VIDEO_STORE_INC_DIR) $(C_FLAGS) -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
	
./Debug/%.o: ./Src/VideoStore/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	echo "$(CPP)"
	$(CPP) $(CXX_DEFINES) -I$(VIDEO_STORE_INC_DIR) $(CXX_FLAGS)  -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"  -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
		


