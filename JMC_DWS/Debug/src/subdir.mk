################################################################################
# Automatically-generated file. Do not edit!
################################################################################
CPPFLAGS := -std=c++11 -DADAS -DNOWIN -DDLIB_USE_BLAS -O3 -DCPU_ONLY
CFLAGS :=
INC_DIR = -I/home/tony/Public/include -I/home/tony/Public/include/opencv/ -I/home/tony/Public/adas/Include \
-I/home/tony/Downloads/libxml2-2.7.2/install/include/libxml2
 
LIB_DIR =-L/home/tony/Public/lib/arm -L/home/tony/Downloads/libxml2-2.7.2/install/lib

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/app_main.cpp \
../src/v4l2_tvin.cpp \

C_SRCS += \
../src/serial_port_commu.c \
../src/serial_pack_parse.c \
../src/driving_behav_analys.c \
../src/timer.c \
../src/timer_unix.c \
../src/gpio_operation.c \
../src/watchdog.c \
../src/single_daemon_running.c \
../src/xml_operation.c \
../src/bootloader.c \
../src/kfifo.c

OBJS += \
./src/app_main.o \
./src/serial_port_commu.o \
./src/v4l2_tvin.o \
./src/serial_pack_parse.o \
./src/driving_behav_analys.o \
./src/timer.o \
./src/timer_unix.o \
./src/gpio_operation.o \
./src/watchdog.o \
./src/single_daemon_running.o \
./src/xml_operation.o \
./src/bootloader.o \
./src/kfifo.o

CPP_DEPS += \
./src/app_main.d \
./src/v4l2_tvin.d

C_DEPS += \
./src/serial_port_commu.d \
./src/serial_pack_parse.d \
./src/driving_behav_analys.d \
./src/timer.d \
./src/timer_unix.d \
./src/gpio_operation.d \
./src/watchdog.d \
./src/single_daemon_running.d \
./src/xml_operation.d \
./src/bootloader.d \
./src/kfifo.d


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


