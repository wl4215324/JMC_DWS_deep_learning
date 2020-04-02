################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
./dictionary.c \
./iniparser.c \
./usr_conf.c

CPP_SRCS += 

OBJS += \
./Debug/dictionary.o \
./Debug/iniparser.o \
./Debug/usr_conf.o


C_DEPS += \
./Debug/dictionary.d \
./Debug/iniparser.d \
./Debug/usr_conf.d

CPP_DEPS +=


#SDK_ROOT_DIR = /home/tony/t7_reference/t7linux-auto
          
#T7_SDK_LIB_PATH := ${SDK_ROOT_DIR}/buildroot-201611/target/user_rootfs_misc/sdk_lib

#VIDEO_STORE_INC_DIR := ${T7_SDK_LIB_PATH}/cedarx/include/libcedarc/include/
		
#CF = -std=c++11 -c -O3 -Wall -fpermissive -DCPU_ONLY 
#FAST = -ffast-math  -march=armv7-a -mfpu=vfpv4-d16 -mfloat-abi=hard -mcpu=cortex-a7 -fopenmp

#CXX_DEFINES := -DV4L2_NV21
#CXX_FLAGS := -std=c++11 -fpermissive
#
#C_DEFINES := -DV4L2_NV21
#C_FLAGS :=

#LIBS += -lMemAdapter -lsdk_g2d -lcdx_base -lvideoengine -lsdk_disp

# Each subdirectory must supply rules for building sources it contributes
./Debug/%.o: ./Src/iniparser/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	$(CC) $(C_DEFINES) $(C_FLAGS) -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
	
./Debug/%.o: ./Src/iniparser/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	echo "$(CPP)"
	$(CPP) $(CXX_DEFINES) $(CXX_FLAGS)  -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"  -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
		


