
RM := rm -rf

-include ./Src/VideoAndDisp/subdir.mk
-include ./Src/Algorithm/Src/subdir.mk
-include ./Src/VideoStore/subdir.mk

FAST = -ffast-math  -march=armv7-a -mfpu=vfpv4-d16 -mfloat-abi=hard -mcpu=cortex-a7 -fopenmp


# All Target
all: VideoAndAlgorithm

# Tool invocations
VideoAndAlgorithm: $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross GCC Linker'
#	@echo $(ALGORITHM_LIBS)
	$(CPP) $(CXX_FLAGS) $(FAST) $(OBJS) -o $@ $(ALGORITHM_LIBS) $(CMAKE_LIBS) -Wl,-rpath=/root/libs_algo/hf-jianhua/
	@echo 'Finished building target: $@'
	@echo ' '


# Other Targets
clean:
	$(RM) $(OBJS) $(C_DEPS) $(CPP_DEPS) VideoAndAlgorithm
	@echo ' '