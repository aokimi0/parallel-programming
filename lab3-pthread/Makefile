CXX = g++
CXXFLAGS = -std=c++11 -O2 -g -Wall -Wextra
LDFLAGS_PTHREAD = -lpthread
LDFLAGS_OPENMP = -fopenmp

# Common object file
COMMON_OBJ = src/common_crt_ntt.o

# Source files for executables
SRC_SERIAL = src/ntt_crt_arbitrary_serial.cpp
SRC_PTHREAD = src/ntt_crt_arbitrary_pthread.cpp
SRC_OPENMP = src/ntt_crt_arbitrary_openmp.cpp

# Executables
EXEC_SERIAL = ntt_crt_arbitrary_serial
EXEC_PTHREAD = ntt_crt_arbitrary_pthread
EXEC_OPENMP = ntt_crt_arbitrary_openmp

.PHONY: all clean

all: $(EXEC_SERIAL) $(EXEC_PTHREAD) $(EXEC_OPENMP)

$(COMMON_OBJ): src/common_crt_ntt.cpp src/common_crt_ntt.h
	$(CXX) $(CXXFLAGS) -c src/common_crt_ntt.cpp -o $(COMMON_OBJ)

$(EXEC_SERIAL): $(SRC_SERIAL) $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(SRC_SERIAL) $(COMMON_OBJ) -o $(EXEC_SERIAL)

$(EXEC_PTHREAD): $(SRC_PTHREAD) $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(SRC_PTHREAD) $(COMMON_OBJ) $(LDFLAGS_PTHREAD) -o $(EXEC_PTHREAD)

$(EXEC_OPENMP): $(SRC_OPENMP) $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS_OPENMP) $(SRC_OPENMP) $(COMMON_OBJ) $(LDFLAGS_OPENMP) -o $(EXEC_OPENMP)

clean:
	rm -f $(COMMON_OBJ) $(EXEC_SERIAL) $(EXEC_PTHREAD) $(EXEC_OPENMP) src/*.o *.o

# Example test runs (can be expanded in a script)
run_tests:
	@echo "Running serial test..."
	./$(EXEC_SERIAL) 131072 1337006139375617
	@echo "\nRunning pthread test..."
	./$(EXEC_PTHREAD) 131072 1337006139375617 4
	@echo "\nRunning openmp test..."
	./$(EXEC_OPENMP) 131072 1337006139375617 4 