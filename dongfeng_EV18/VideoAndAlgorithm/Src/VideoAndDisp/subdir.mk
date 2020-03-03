################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS = \
./DmaIon.c \
./sdklog.c \
./sunxiMemInterface.c

CPP_SRCS = \
./camera_test.cpp \
./gl_display.cpp \
./video_layer.cpp \
./video_layer_test.cpp \
./t7_camera_v4l2.cpp
#./t7_enc.cpp \
#./t7_enc_test.cpp

OBJS = \
./Debug/camera_test.o \
./Debug/t7_camera_v4l2.o \
./Debug/gl_display.o \
./Debug/video_layer.o \
./Debug/video_layer_test.o \
./Debug/DmaIon.o \
./Debug/sdklog.o \
./Debug/sunxiMemInterface.o


C_DEPS += \
./Debug/DmaIon.d \
./Debug/sdklog.d \
./Debug/sunxiMemInterface.d

CPP_DEPS += \
./Debug/camera_test.d \
./Debug/gl_display.d \
./Debug/video_layer.d \
./Debug/video_layer_test.d \
./Debug/t7_camera_v4l2.d
#./Debug/t7_enc.d \
#./Debug/t7_enc_test.d


SDK_ROOT_DIR = /home/tony/t7_reference/t7linux-auto
FBDEV_DIR = ${SDK_ROOT_DIR}/tools/pack/chips/sun8iw17p1/hal/gpu/fbdev

INC_DIR = -I ${SDK_ROOT_DIR}/buildroot-201611/target/user_rootfs_misc/sdk_lib/cedarx/include/libcedarc/include \
          -I ${SDK_ROOT_DIR}/buildroot-201611/target/user_rootfs_misc/sdk_lib/cedarx/include/external/include \
          -I ${SDK_ROOT_DIR}/buildroot-201611/output/build/mplayer-1.3.0/ffmpeg/ \
          -I ${SDK_ROOT_DIR}/buildroot-201611/target/user_rootfs_misc/sdk_lib/include \
          -I ${SDK_ROOT_DIR}/buildroot-201611/target/user_rootfs_misc/sdk_lib/include/disp2 \
          -I ${SDK_ROOT_DIR}/buildroot-201611/target/user_rootfs_misc/sdk_lib/include/utils/ \
          -I ${FBDEV_DIR}/include/
          
T7_SDK_LIB_PATH := ${SDK_ROOT_DIR}/buildroot-201611/target/user_rootfs_misc/sdk_lib

VIDEO_ENCODE_PATH := /home/tony/Downloads/gl_display_test_0920

INC_DIR += -I ${VIDEO_ENCODE_PATH}/ \
    -I ${VIDEO_ENCODE_PATH}/include/
    
CEDAR_INC :=
		
INC_DIR += $(CEDAR_INC)
		
#CF = -std=c++11 -c -O3 -Wall -fpermissive -DCPU_ONLY 
#FAST = -ffast-math  -march=armv7-a -mfpu=vfpv4-d16 -mfloat-abi=hard -mcpu=cortex-a7 -fopenmp

CXX_DEFINES := -DV4L2_NV21
CXX_FLAGS := -std=c++11 -fpermissive

C_DEFINES := -DV4L2_NV21
C_FLAGS :=

LIB_DIR = -L ${FBDEV_DIR}/lib/
LIBS = -lpthread -lEGL -lGLESv2

LIB_DIR += -L ${SDK_ROOT_DIR}/buildroot-201611/target/user_rootfs_misc/sdk_lib/cedarx/lib \
           -L ${SDK_ROOT_DIR}/buildroot-201611/target/user_rootfs_misc/sdk_lib/libs

#LIBS += -lMemAdapter -lsdk_g2d -lcdx_base -lvideoengine -lsdk_disp
LIBS +=

CMAKE_LIBS := -L/home/tony/t7_reference/t7linux-auto/tools/pack/chips/sun8iw17p1/hal/gpu/fbdev/lib \
-L/home/tony/t7_reference/t7linux-auto/buildroot-201611/target/user_rootfs_misc/sdk_lib/cedarx/lib \
-L/home/tony/t7_reference/t7linux-auto/buildroot-201611/target/user_rootfs_misc/sdk_lib/libs \
-Wl,-rpath=/home/tony/t7_reference/t7linux-auto/tools/pack/chips/sun8iw17p1/hal/gpu/fbdev/lib:/home/tony/t7_reference/t7linux-auto/buildroot-201611/target/user_rootfs_misc/sdk_lib/cedarx/lib:/home/tony/t7_reference/t7linux-auto/buildroot-201611/target/user_rootfs_misc/sdk_lib/libs: \
-lGLESv2 -lEGL -lpthread -lMemAdapter -lvencoder -lsdk_g2d -lcdx_base


# -lsdk_g2d -lcdx_base -lvideoengine -lsdk_disp

# Each subdirectory must supply rules for building sources it contributes
./Debug/%.o: ./Src/VideoAndDisp/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	$(CC) $(C_DEFINES) $(INC_DIR) $(C_FLAGS) -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
	
./Debug/%.o: ./Src/VideoAndDisp/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	echo "$(CPP)"
	$(CPP) $(CXX_DEFINES) $(INC_DIR) $(CXX_FLAGS)  -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"  -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
		


