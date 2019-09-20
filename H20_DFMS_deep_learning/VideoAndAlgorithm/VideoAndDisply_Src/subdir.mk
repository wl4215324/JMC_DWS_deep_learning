# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS := \
./VideoAndDisply_Src/t7_camera_v4l2.c \
./VideoAndDisply_Src/sunxiMemInterface.c \
./VideoAndDisply_Src/sdklog.c \
./VideoAndDisply_Src/DmaIon.c 

CPP_SRCS := \
./VideoAndDisply_Src/gl_display.cpp \
./VideoAndDisply_Src/video_layer_test.cpp \
./VideoAndDisply_Src/video_layer.cpp

OBJS := \
./debug/t7_camera_v4l2.o \
./debug/gl_display.o \
./debug/video_layer_test.o \
./debug/sunxiMemInterface.o \
./debug/sdklog.o \
./debug/DmaIon.o \
./debug/video_layer.o

C_DEPS := \
./debug/t7_camera_v4l2.d \
./debug/sunxiMemInterface.d \
./debug/sdklog.d \
./debug/DmaIon.d

CPP_DEPS := \
./debug/gl_display.d \
./debug/video_layer_test.d \
./debug/video_layer.d

USER_OBJS +=

INC_DIR := -I./VideoAndDisply_Src/fbdev/include -I../ShmCommon/ -I./Algorithm_Src/algo/ \
           -I ~/t7_reference/t7linux-auto/buildroot-201611/target/user_rootfs_misc/sdk_lib/cedarx/include/libcedarc/include/ \
           -I ~/t7_reference/t7linux-auto/buildroot-201611/target/target/user_rootfs_misc/sdk_lib/cedarx/include/libcedarc/

LIBS += -L ./VideoAndDisply_Src/fbdev/lib -lpthread -lEGL -lGLESv2  \
        -L ~/t7_reference/t7linux-auto/buildroot-201611/target/user_rootfs_misc/sdk_lib/libs -lsdk_disp -lsdk_g2d \
        -L ~/t7_reference/t7linux-auto/buildroot-201611/target/user_rootfs_misc/sdk_lib/cedarx/lib -lMemAdapter -lvencoder -lcdx_base -lVE -lcdc_base


# Each subdirectory must supply rules for building sources it contributes
./debug/%.o: ./VideoAndDisply_Src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	$(CC) -O0 -g3 -Wall -c $(INC_DIR) -fmessage-length=0 -DCPU_ONLY -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
./debug/%.o: ./VideoAndDisply_Src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	$(CPP) -std=c++11 -O0 -g3 -Wall -c $(INC_DIR) -fmessage-length=0 -DCPU_ONLY -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"  -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '