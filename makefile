# Define the compiler
CC = gcc

# Define the compiler flags
CFLAGS = $(shell pkg-config --cflags gtk4) -Wall -Iinclude

# Define the linker flags
LDFLAGS = $(shell pkg-config --libs gtk4)

# Define the source files
SRCS = $(wildcard src/*.c)

# Define the object files directory
OBJ_DIR = builds/object_files

# Define the object files
OBJS = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Define the output directory and executable name
BUILD_DIR = builds
EXECUTABLE = $(BUILD_DIR)/chess_game_3.1

# Define the default target
all: $(EXECUTABLE)

# Rule to create the output directories if they don't exist
$(BUILD_DIR) $(OBJ_DIR):
	mkdir -p $@

# Rule to link the object files into the executable
$(EXECUTABLE): $(OBJS) | $(BUILD_DIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Rule to compile the source files into object files
$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean up the build artifacts
clean:
	rm -f $(OBJS) $(EXECUTABLE)

# Rule to remove the output directories and all build artifacts
distclean: clean
	rmdir $(OBJ_DIR) $(BUILD_DIR)

# Define the debug target
debug: CFLAGS += -g
debug: $(EXECUTABLE)

# Phony targets to avoid conflicts with files of the same name
.PHONY: all clean distclean debug