# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS := \
./VideoAndDisply_Src/t7_camera_v4l2.c

CPP_SRCS := \
./VideoAndDisply_Src/gl_display.cpp

OBJS := \
./debug/t7_camera_v4l2.o \
./debug/gl_display.o

C_DEPS := \
./debug/t7_camera_v4l2.d

CPP_DEPS := \
./debug/gl_display.d

USER_OBJS +=

INC_DIR := -I./VideoAndDisply_Src/fbdev/include -I../ShmCommon/

LIBS += -L ./VideoAndDisply_Src/fbdev/lib -lpthread -lEGL -lGLESv2


# Each subdirectory must supply rules for building sources it contributes
./debug/%.o: ./VideoAndDisply_Src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	$(CC) -O0 -g3 -Wall -c $(INC_DIR) -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
./debug/%.o: ./VideoAndDisply_Src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	$(CPP) -std=c++11 -O0 -g3 -Wall -c $(INC_DIR) -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"  -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '