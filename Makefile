CXX = clang++
FLAGS = -std=c++17 -Wall -Wextra -g -Iinclude
TARGET = riscv-iss
SRCS = $(wildcard src/*.cc)

all:
	$(CXX) $(FLAGS) $(SRCS) -o $(TARGET)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)