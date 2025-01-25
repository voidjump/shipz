# Makefile to build 'shipz' project

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -I/usr/local/include -DSHAREPATH="\"./\"" -I./include/ -I/opt/homebrew/Cellar/googletest/1.15.2/include -I./source/
LDFLAGS = -L/usr/local/lib -lSDL3 -lSDL3_image -lSDL3_mixer -lSDL3_net -lSDL3_ttf -rpath /usr/local/lib -L/opt/homebrew/Cellar/googletest/1.15.2/lib/
RPATH = /usr/local/lib
DEBUG_FLAGS = -g -O0 -DDEBUG_BUILD

# Source and object files
SRC_DIR = source
SRC = $(wildcard $(SRC_DIR)/*.cpp)
SRC_NO_MAIN = $(filter-out source/main.cpp, $(SRC))
OBJ = $(SRC:.cpp=.o)
EXEC = shipz

# Test directory and test files
TEST_DIR = tests
TESTSRC = $(wildcard $(TEST_DIR)/*_test.cpp)
ALL_TEST_SRC = $(TESTSRC) $(SRC_NO_MAIN)
TESTOBJ = $(ALL_TEST_SRC:.cpp=.o)
TESTEXEC = test_program

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

# Compile and run all gtest files
test: $(TESTOBJ) 
	$(CXX) -o $(TESTEXEC) $(TESTOBJ) $(LDFLAGS) -lgtest -lgtest_main -pthread
	./$(TESTEXEC)

$(TESTDIR)/%.o: $(TESTDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

# Clean up build files
clean:
	rm -f $(OBJ) $(EXEC) $(TESTOBJ) $(TESTEXEC)

.PHONY: all clean

