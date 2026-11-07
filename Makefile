CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -I.

TARGET = tests/test_hugeint

.PHONY: all test clean

all: test

test: $(TARGET)
	./$(TARGET)

$(TARGET): tests/test_hugeint.cpp HugeInt.cpp HugeInt.h
	$(CXX) $(CXXFLAGS) tests/test_hugeint.cpp HugeInt.cpp -o $@

clean:
	rm -f $(TARGET) $(TARGET).exe
