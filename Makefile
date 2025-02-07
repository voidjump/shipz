# Makefile to build 'shipz' project

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -I/home/steve/builds/include -I/usr/local/include -DSHAREPATH="\"./\"" -I./include/
LDFLAGS = -L/usr/local/lib -L/home/steve/builds/lib64 -lSDL3 -lSDL3_image -lSDL3_mixer -lSDL3_ttf 
RPATH = /usr/local/lib
DEBUG_FLAGS = -g -O0
HEADLESS_FLAGS = -DHEADLESS

# Source and object files
SRCDIR = source
SRC = $(wildcard $(SRCDIR)/*.cpp)
OBJ = $(SRC:.cpp=.o)
EXEC = shipz

# Build target
all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)
	# export DYLD_LIBRARY_PATH=$(RPATH)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

# Debug build target
debug: CXXFLAGS += $(DEBUG_FLAGS)  # Add debug flags to CXXFLAGS
debug: $(EXEC)

# Headless server build
headless: CXXFLAGS += $(HEADLESS_FLAGS) # Headless server build
headless: $(EXEC)

# Clean up build files
clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: all clean

