# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS := \
./Algorithm_Src/app_main.c

CPP_SRCS := \

OBJS += \
./debug/app_main.o


C_DEPS := \
./debug/app_main.d

CPP_DEPS := 

USER_OBJS :=
INC_DIR ?=

# Each subdirectory must supply rules for building sources it contributes
./debug/%.o: ./Algorithm_Src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	$(CC) -O0 -g3 -Wall -c -I$(INC_DIR) -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
./debug/%.o: ./Algorithm_Src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	$(CPP) -std=c++11 -O0 -g3 -Wall -c -I$(INC_DIR) -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"  -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '