# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS := \
./Algorithm_Src/warning_logic.c \
./Algorithm_Src/timer.c \
./Algorithm_Src/user_timer.c


CPP_SRCS := \
./Algorithm_Src/app_main.cpp



OBJS += \
./debug/app_main.o \
./debug/warning_logic.o \
./debug/timer.o \
./debug/user_timer.o


C_DEPS := \
./debug/warning_logic.d \
./debug/timer.d \
./debug/user_timer.d


CPP_DEPS := \
./debug/app_main.d 


USER_OBJS +=
INC_DIR += -I ./Algorithm_Src/algo -I ./VideoAndDisply_Src

# 系统根目录
ROOTFS_DIR = /home/tony/eclipse-workspace/T7_Protect/rootfs/

# 系统搜索文件目录
#LIB_DIR = /home/tony/eclipse-workspace/T7_Protect/rootfs/

#IF	    =   -I$(LIB_DIR)include				\
#		  	-I$(LIB_DIR)fbdev/include       	\
#		  	-I$(LIB_DIR)sdk_t7/cedarx/include/libcedarc/include \
#		  	-I$(LIB_DIR)opencv3/include        	\
#		  	-I$(LIB_DIR)ncnn/include        	\
#		  	-I$(LIB_DIR)caffe/include           \
#		  	-I$(LIB_DIR)caffe/include/openblas  \
#		  	-I./ -I$(ROOT_DIR)T7SdkLib/include -I$(ROOT_DIR)CommonLib/include
		  	
INC_DIR += -I$(ROOTFS_DIR)/include -I$(ROOTFS_DIR)caffe/include \
           -I$(ROOTFS_DIR)caffe/include/openblas -I$(ROOTFS_DIR)ncnn/include \
           -I$(ROOTFS_DIR)opencv3/include
           
CF  	= 	-std=c++11 -c -O3 -Wall -fpermissive -DCPU_ONLY 
FAST 	= 	-ffast-math  -march=armv7-a -mfpu=vfpv4-d16 -mfloat-abi=hard -mcpu=cortex-a7 -fopenmp

LIBS += -L$(ROOTFS_DIR)/opencv3/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lopencv_video -lopencv_videoio\
        -lopencv_calib3d -lopencv_objdetect \

LIBS += -L$(ROOTFS_DIR)/caffe/lib -lboost_system -lopenblas -lboost_filesystem -lboost_regex -lboost_atomic -lcaffe -lprotobuf -lgfortran \
      -lboost_thread -lhdf5 -lhdf5_hl -lpng16 -lglog -lgflags -lrt \

LIBS +=../ShmCommon/CommLib.a ./dsm.a ./libncnn.a

LF += -L/home/tony/eclipse-workspace/T7_Protect/rootfs/caffe/lib \
      -lboost_system -lopenblas -lboost_filesystem -lboost_regex -lboost_atomic -lcaffe -lprotobuf -lgfortran \
      -lboost_thread -lhdf5 -lhdf5_hl -lpng16 -lglog -lgflags \
	  -lrt \


# Each subdirectory must supply rules for building sources it contributes
./debug/%.o: ./Algorithm_Src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	$(CC) -O0 -g3 -Wall -c $(INC_DIR) -fmessage-length=0 -DCPU_ONLY -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
	
./debug/%.o: ./Algorithm_Src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	$(CPP)  $(CF) $(INC_DIR)  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"  -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '