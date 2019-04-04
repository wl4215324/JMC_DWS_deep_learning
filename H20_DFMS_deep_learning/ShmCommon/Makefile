# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS := \
$(SHM_COMM_DIR)/producer_consumer_shmfifo.c

OBJS := \
$(SHM_COMM_DIR)/producer_consumer_shmfifo.o

C_DEPS := \
$(SHM_COMM_DIR)/producer_consumer_shmfifo.d

all: Pd_Cu_Shm_fifo.a

Pd_Cu_Shm_fifo.a: $(OBJS)
	$(AR) rc $@ $(OBJS)


# Each subdirectory must supply rules for building sources it contributes
$(OBJS): $(C_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	$(CC) -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
clean:
	$(RM) $(OBJS) $(C_DEPS) Pd_Cu_Shm_fifo.a
	
.PHONY := all clean
