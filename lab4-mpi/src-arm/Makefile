CXX = mpicxx
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra
LDFLAGS = 

SRCDIR = .
TARGET = ntt_mpi_arm
COMMON_OBJS = common_mpi_ntt.o

.PHONY: all clean test

all: $(TARGET)

$(TARGET): ntt_mpi_arm.cpp $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $< $(COMMON_OBJS) $(LDFLAGS)

common_mpi_ntt.o: common_mpi_ntt.cpp common_mpi_ntt.h
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(LDFLAGS)

test: $(TARGET)
	@echo "=== ARM MPI NTT 测试 ==="
	@echo "测试1进程:"
	mpiexec -n 1 ./$(TARGET)
	@echo ""
	@echo "测试2进程:"
	mpiexec -n 2 ./$(TARGET)
	@echo ""
	@echo "测试4进程:"
	mpiexec -n 4 ./$(TARGET)

benchmark: $(TARGET)
	@echo "=== ARM MPI NTT 性能测试 ==="
	@mkdir -p files
	@for proc in 1 2 4 8; do \
		echo "测试 $$proc 进程:"; \
		time mpiexec -n $$proc ./$(TARGET); \
		echo ""; \
	done

clean:
	rm -f $(TARGET) $(COMMON_OBJS)
	rm -rf files

help:
	@echo "可用目标:"
	@echo "  all        - 编译程序"
	@echo "  test       - 运行基本测试"
	@echo "  benchmark  - 运行性能测试"
	@echo "  clean      - 清理编译文件"
	@echo "  help       - 显示帮助信息" 