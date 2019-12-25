# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS := \
$(SHM_COMM_DIR)/producer_consumer_shmfifo.c \
$(SHM_COMM_DIR)/serial_pack_parse.c \
$(SHM_COMM_DIR)/bootloader.c \
$(SHM_COMM_DIR)/file_operate.c \
$(SHM_COMM_DIR)/serial_pack_parse.c \
$(SHM_COMM_DIR)/sha_1.c

OBJS := \
$(SHM_COMM_DIR)/producer_consumer_shmfifo.o \
$(SHM_COMM_DIR)/serial_pack_parse.o \
$(SHM_COMM_DIR)/bootloader.o \
$(SHM_COMM_DIR)/file_operate.o \
$(SHM_COMM_DIR)/serial_pack_parse.o \
$(SHM_COMM_DIR)/sha_1.o

C_DEPS := \
$(SHM_COMM_DIR)/producer_consumer_shmfifo.d \
$(SHM_COMM_DIR)/serial_pack_parse.d \
$(SHM_COMM_DIR)/bootloader.d \
$(SHM_COMM_DIR)/file_operate.d \
$(SHM_COMM_DIR)/serial_pack_parse.d \
$(SHM_COMM_DIR)/sha_1.d

#CFLAGS := -Wall -std=c++11 -O2 -shared -fPIC

all: CommLib.a
#all: libCommLib.so

#libCommLib.so: $(OBJS)
#	@echo 'Building target: $@'
#	@echo 'Invoking: Cross GCC Linker'
#	$(CC)  -shared -o $@ $(OBJS)
#	@echo 'Finished building target: $@'
#	@echo ' '

CommLib.a: $(OBJS)
	$(RM) CommLib.a
	$(AR) rc $@ $(OBJS)
	$(RM) $(OBJS) $(C_DEPS)
	
# Each subdirectory must supply rules for building sources it contributes
./%.o: ./%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	$(CC) -Wall -O2  -c $(INC_DIR) -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	#$(CC)  $(CFLAGS) -c $(INCLUDE) -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
	
clean:
	$(RM) $(OBJS) $(C_DEPS) CommLib.a
	
.PHONY := all clean
