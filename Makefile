CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic
PYTHON ?= python3

emulator: src/main.cpp src/vfs.cpp src/vfs.h
	$(CXX) $(CXXFLAGS) src/main.cpp src/vfs.cpp -o emulator

clean:
	rm -f emulator

test: emulator
	$(PYTHON) -m unittest discover -s tests -p 'test_*.py' -v

.PHONY: clean test
