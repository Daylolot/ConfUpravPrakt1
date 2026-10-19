CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

emulator: src/main.cpp src/vfs.cpp src/vfs.h
	$(CXX) $(CXXFLAGS) src/main.cpp src/vfs.cpp -o emulator

clean:
	rm -f emulator

.PHONY: clean
