
NVCC = nvcc
CXXFLAGS = -std=c++17 -O3
NVCCFLAGS = -std=c++17 -O3 -arch=sm_75 --use_fast_math
SRC_DIR = src
SOURCES = $(SRC_DIR)/ntt_gpu.cu $(SRC_DIR)/ntt_serial.cpp
OBJS = $(patsubst %.cu, %.o, $(filter %.cu, $(SOURCES))) $(patsubst %.cpp, %.o, $(filter %.cpp, $(SOURCES)))

TARGET = ntt_gpu
all: $(TARGET)

$(TARGET): $(OBJS)
	$(NVCC) $(NVCCFLAGS) -o $@ $^

%.o: %.cu
	$(NVCC) $(NVCCFLAGS) -c -o $@ $<

%.o: %.cpp
	$(NVCC) $(NVCCFLAGS) -x cu -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS) output_*.txt output.txt

.PHONY: all clean 