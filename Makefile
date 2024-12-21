# Makefile to build 'shipz' project

# Compiler and flags
CXX = g++
CXXFLAGS = -I/usr/local/include -lc++ -DSHAREPATH="\"./\""
LDFLAGS = -L/usr/local/lib -lSDL3 -lSDL3_image -lSDL3_mixer -lSDL3_net -lSDL3_ttf
RPATH = /usr/local/lib

# Source and object files
SRCDIR = source
SRC = $(wildcard $(SRCDIR)/*.cpp)
OBJ = $(SRC:.cpp=.o)
EXEC = shipz

# Build target
all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)
	export DYLD_LIBRARY_PATH=$(RPATH):$$DYLD_LIBRARY_PATH

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

# Clean up build files
clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: all clean

