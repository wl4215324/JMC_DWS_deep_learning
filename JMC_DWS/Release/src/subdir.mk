################################################################################
# Automatically-generated file. Do not edit!
################################################################################

CPPFLAGS := -std=c++11 -DADAS -DNOWIN -DDLIB_USE_BLAS -O3 -DCPU_ONLY
CFLAGS :=
INC_DIR = -I/home/tony/Public/include -I/home/tony/Public/include/opencv/ -I/home/tony/Public/adas/Include \
-I/home/tony/Downloads/libxml2-2.7.2/install/include/libxml2/
 
LIB_DIR =-L/home/tony/Public/lib/arm -L/home/tony/Downloads/libxml2-2.7.2/install/lib


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/driving_behav_analys.c \
../src/gpio_operation.c \
../src/serial_pack_parse.c \
../src/serial_port_commu.c \
../src/single_daemon_running.c \
../src/timer.c \
../src/timer_unix.c \
../src/watchdog.c \
../src/xml_operation.c \
../src/bootloader.c

CPP_SRCS += \
../src/app_main.cpp \
../src/v4l2_tvin.cpp 

OBJS += \
./src/app_main.o \
./src/driving_behav_analys.o \
./src/gpio_operation.o \
./src/serial_pack_parse.o \
./src/serial_port_commu.o \
./src/single_daemon_running.o \
./src/timer.o \
./src/timer_unix.o \
./src/v4l2_tvin.o \
./src/watchdog.o \
./src/xml_operation.o \
./src/bootloader.o

C_DEPS += \
./src/driving_behav_analys.d \
./src/gpio_operation.d \
./src/serial_pack_parse.d \
./src/serial_port_commu.d \
./src/single_daemon_running.d \
./src/timer.d \
./src/timer_unix.d \
./src/watchdog.d \
./src/xml_operation.d \
./src/bootloader.d

CPP_DEPS += \
./src/app_main.d \
./src/v4l2_tvin.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	arm-linux-gnueabihf-g++ $(CPPFLAGS) $(INC_DIR) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc $(CFLAGS) $(INC_DIR) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


