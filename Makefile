CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

emulator: main.cpp vfs.cpp vfs.h
	$(CXX) $(CXXFLAGS) main.cpp vfs.cpp -o emulator

clean:
	rm -f emulator

.PHONY: clean
