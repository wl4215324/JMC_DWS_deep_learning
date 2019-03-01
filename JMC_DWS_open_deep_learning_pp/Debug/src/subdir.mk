################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 

CPPFLAGS := -std=c++11 -DADAS -DNOWIN -DDLIB_USE_BLAS -O3 -DCPU_ONLY

CFLAGS :=

INC_DIR = -I/home/tony/Downloads/libxml2-2.7.2/install/include/libxml2
 
LIB_DIR = -L/home/tony/Public/lib/arm_for_detect -L/home/tony/Downloads/libxml2-2.7.2/install/lib 


C_SRCS += \
../src/bootloader.c \
../src/v4l2_tvin.c  \
../src/driving_behav_analys.c \
../src/gpio_operation.c \
../src/kfifo.c \
../src/serial_pack_parse.c \
../src/serial_port_commu.c \
../src/single_daemon_running.c \
../src/timer_unix.c \
../src/timer.c \
../src/watchdog.c \
../src/xml_operation.c \
../src/sha_1.c \
../src/production_test.c \
../src/bootloader_new.c \
../src/file_operate.c \
../src/eol_funct_test.c

CPP_SRCS += \
../src/app_main.cpp \
../src/algorithm.cpp

OBJS += \
./src/app_main.o \
./src/v4l2_tvin.o  \
./src/algorithm.o \
./src/bootloader.o \
./src/driving_behav_analys.o \
./src/gpio_operation.o \
./src/kfifo.o \
./src/serial_pack_parse.o \
./src/serial_port_commu.o \
./src/single_daemon_running.o \
./src/timer_unix.o \
./src/timer.o \
./src/watchdog.o \
./src/xml_operation.o \
./src/sha_1.o \
./src/production_test.o \
./src/bootloader_new.o \
./src/file_operate.o \
./src/eol_funct_test.o

C_DEPS += \
./src/v4l2_tvin.d \
./src/bootloader.d \
./src/driving_behav_analys.d \
./src/gpio_operation.d \
./src/kfifo.d \
./src/serial_pack_parse.d \
./src/serial_port_commu.d \
./src/single_daemon_running.d \
./src/timer_unix.d \
./src/timer.d \
./src/watchdog.d \
./src/xml_operation.d \
./src/sha_1.d \
./src/production_test.d \
./src/bootloader_new.d \
./src/file_operate.d \
./src/eol_funct_test.d

CPP_DEPS += \
./src/app_main.d \
./src/algorithm.d


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	#arm-linux-gnueabihf-g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	arm-linux-gnueabihf-g++-4.8 $(CPPFLAGS) $(INC_DIR) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	#arm-linux-gnueabihf-gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	arm-linux-gnueabihf-gcc-4.8 $(CFLAGS) $(INC_DIR) -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


