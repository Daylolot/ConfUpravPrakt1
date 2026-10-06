CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

emulator: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o emulator

clean:
	rm -f emulator

.PHONY: clean
