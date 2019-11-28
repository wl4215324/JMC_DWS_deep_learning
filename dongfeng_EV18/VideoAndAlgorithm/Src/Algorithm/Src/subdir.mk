################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS = \
./warning_logic.c

CPP_SRCS = \
./run_algorithm.cpp

OBJS += \
./Debug/warning_logic.o \
./Debug/run_algorithm.o

C_DEPS += \
./Debug/warning_logic.d

CPP_DEPS += \
./Debug/run_algorithm.d


# 系统根目录
ROOTFS_DIR = /home/tony/eclipse-workspace/T7_Protect/rootfs/

INC_DIR += -I /home/tony/eclipse-workspace/dongfeng_EV18/VideoAndAlgorithm/Src/Algorithm/algo
INC_DIR += -I$(ROOTFS_DIR)/include -I$(ROOTFS_DIR)caffe/include \
           -I$(ROOTFS_DIR)caffe/include/openblas -I$(ROOTFS_DIR)ncnn/include \
           -I$(ROOTFS_DIR)opencv3/include \
           -I/home/tony/eclipse-workspace/dongfeng_EV18/ShmCommon

CF = -std=c++11 -c -O3 -Wall -fpermissive -DCPU_ONLY 
FAST = -ffast-math  -march=armv7-a -mfpu=vfpv4-d16 -mfloat-abi=hard -mcpu=cortex-a7 -fopenmp

ALGORITHM_LIBS += -L$(ROOTFS_DIR)/opencv3/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_video -lopencv_videoio\
        -lopencv_calib3d -lopencv_objdetect \

ALGORITHM_LIBS += -L$(ROOTFS_DIR)/caffe/lib -lboost_system -lopenblas -lboost_filesystem -lboost_regex -lboost_atomic -lcaffe -lprotobuf -lgfortran \
      -lboost_thread -lhdf5 -lhdf5_hl -lpng16 -lglog -lgflags -lrt \

ALGORITHM_LIBS += /home/tony/eclipse-workspace/dongfeng_EV18/VideoAndAlgorithm/Src/Algorithm/Src/dsm.a \
          /home/tony/eclipse-workspace/dongfeng_EV18/VideoAndAlgorithm/Src/Algorithm/Src/libncnn.a \
          /home/tony/eclipse-workspace/dongfeng_EV18/ShmCommon/CommLib.a


# Each subdirectory must supply rules for building sources it contributes
./Debug/%.o: ./Src/Algorithm/Src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	$(CC) -O0 -g3 -Wall -c $(INC_DIR) -fmessage-length=0 -DCPU_ONLY -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
	
./Debug/%.o: ./Src/Algorithm/Src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	$(CPP) $(CF) $(INC_DIR)  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"  -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
		


