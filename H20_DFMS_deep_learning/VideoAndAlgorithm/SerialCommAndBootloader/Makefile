-include ./debug/subdir.mk

LIB_DIR :=
LIBS :=

all: SerialCommAndBootloader

SerialCommAndBootloader: $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	$(CC) -o $@  $(OBJS)  $(USER_OBJS)  $(LIB_DIR) $(LIBS) ../ShmCommon/CommLib.a
	@echo 'Finished building target: $@'
	@echo ' '
	
clean:
	$(RM) $(OBJS) $(C_DEPS) $(CPP_DEPS) SerialCommAndBootloader
	
.PHONY := all clean
	
