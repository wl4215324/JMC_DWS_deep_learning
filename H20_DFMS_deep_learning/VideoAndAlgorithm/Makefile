-include ./VideoAndDisply_Src/subdir.mk
-include ./Algorithm_Src/subdir.mk


	  
CF = -std=c++11 -c -O3 -Wall -fpermissive -DCPU_ONLY 
FAST = -ffast-math  -march=armv7-a -mfpu=vfpv4-d16 -mfloat-abi=hard -mcpu=cortex-a7 -fopenmp

all: VideoAndAlgorithm

VideoAndAlgorithm: $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	$(RM) $@
	$(CPP) $(FAST) -o $@ $(OBJS) $(USER_OBJS) $(LIBS) -Wl,-rpath=/root/libs_algo/hf-jianhua/
	$(STRIP) $@
	@echo 'Finished building target: $@'
	@echo ' '
	
clean:
	$(RM) $(OBJS) $(C_DEPS) $(CPP_DEPS) VideoAndAlgorithm
	
.PHONY := all clean