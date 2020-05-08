################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
curl_post.c

CPP_SRCS +=

OBJS += \
./Debug/curl_post.o

C_DEPS += \
./Debug/curl_post.d

CPP_DEPS +=

CURL_INC := /dongfeng_EV18/VideoAndAlgorithm/Src/CurlPost/include/

T7_SDK_LIB_PATH := ${SDK_ROOT_DIR}/buildroot-201611/target/user_rootfs_misc/sdk_lib

VIDEO_STORE_INC_DIR := ${T7_SDK_LIB_PATH}/cedarx/include/libcedarc/include/
ALGORITHM_LIBS += -L /home/tony/eclipse-workspace/dongfeng_EV18/VideoAndAlgorithm/Src/CurlPost/lib -lcurl -lssl -lcrypto
		

# Each subdirectory must supply rules for building sources it contributes
./Debug/%.o: ./Src/CurlPost/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	$(CC)  -I$(CURL_INC) $(C_FLAGS) -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
	
./Debug/%.o: ./Src/CurlPost/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	$(CPP) -I$(CURL_INC) $(CXX_FLAGS)  -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"  -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
		


