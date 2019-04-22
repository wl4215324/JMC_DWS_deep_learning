
# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS := \
./src/SerialCommuAndBtMain.c \
./src/bootloader.c \
./src/serial_port_commu.c \
./src/sha_1.c \
./src/file_operate.c

CPP_SRCS := \

OBJS := \
./debug/SerialCommuAndBtMain.o \
./debug/bootloader.o \
./debug/serial_port_commu.o \
./debug/sha_1.o \
./debug/file_operate.o 



C_DEPS := \
./debug/SerialCommuAndBtMain.d \
./debug/bootloader.d \
./debug/serial_port_commu.d \
./debug/sha_1.d \
./debug/file_operate.d 

CPP_DEPS := 

USER_OBJS :=
INC_DIR :=  ../ShmCommon/

# Each subdirectory must supply rules for building sources it contributes
./debug/%.o: ./src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	$(CC) -O0 -g3 -Wall -c -I$(INC_DIR) -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
./debug/%.o: ./src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	$(CPP) -std=c++11 -O0 -g3 -Wall -c -I$(INC_DIR) -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"  -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '