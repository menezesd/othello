# Makefile for Othello Game
# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
LDFLAGS = `pkg-config --cflags --libs sdl2`

# Target executable
TARGET = othello

# Source files
SOURCES = othello.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install SDL2 dependencies (for Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y libsdl2-dev pkg-config

# Run the game
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build with optimizations
release: CXXFLAGS += -O3 -DNDEBUG
release: clean $(TARGET)

# Check for memory leaks with valgrind
memcheck: debug
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Help target
help:
	@echo "Available targets:"
	@echo "  all         - Build the game (default)"
	@echo "  clean       - Remove build artifacts"
	@echo "  install-deps- Install SDL2 dependencies"
	@echo "  run         - Build and run the game"
	@echo "  debug       - Build with debug symbols"
	@echo "  release     - Build optimized release version"
	@echo "  memcheck    - Run with valgrind memory checking"
	@echo "  help        - Show this help message"

# Declare phony targets
.PHONY: all clean install-deps run debug release memcheck help