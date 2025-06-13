CXX = mpicxx
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra
LDFLAGS = 

SRCDIR = .
MAIN_TARGETS = ntt_mpi generate_test_data ntt_serial_reference ntt_mpi_arm
COMMON_OBJS = common_mpi_ntt.o

.PHONY: all clean test

all: $(MAIN_TARGETS)

common_mpi_ntt.o: common_mpi_ntt.cpp common_mpi_ntt.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

ntt_mpi: ntt_mpi.cpp $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

generate_test_data: generate_test_data.cpp
	g++ $(CXXFLAGS) -o $@ $< $(LDFLAGS)

ntt_serial_reference: ntt_serial_reference.cpp
	g++ $(CXXFLAGS) -o $@ $< $(LDFLAGS)

ntt_mpi_arm: ntt_mpi_arm.cpp $(COMMON_OBJS)
	mpicxx $(CXXFLAGS) -o $@ $< $(COMMON_OBJS) $(LDFLAGS)

clean:
	rm -f $(MAIN_TARGETS) $(COMMON_OBJS)

test: ntt_mpi
	mpiexec -n 4 ./ntt_mpi

.PHONY: test 