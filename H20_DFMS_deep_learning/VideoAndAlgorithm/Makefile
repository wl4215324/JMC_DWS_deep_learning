-include ./VideoAndDisply_Src/subdir.mk
-include ./Algorithm_Src/subdir.mk


LIB_DIR +=
LIBS +=

all: VideoAndAlgorithm

VideoAndAlgorithm: $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	$(CPP) -o $@ $(OBJS) $(USER_OBJS) $(LIB_DIR) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	
clean:
	$(RM) $(OBJS) $(C_DEPS) $(CPP_DEPS) VideoAndAlgorithm
	
.PHONY := all clean