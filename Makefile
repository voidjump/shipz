# Makefile to build 'shipz' project

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -I/usr/local/include -DSHAREPATH="\"./\"" -I./include/ -I/opt/homebrew/Cellar/googletest/1.15.2/include -I./source/
LDFLAGS = -L/usr/local/lib -lSDL3 -lSDL3_image -lSDL3_mixer -lSDL3_ttf -rpath /usr/local/lib -L/opt/homebrew/Cellar/googletest/1.15.2/lib/
RPATH = /usr/local/lib
DEBUG_FLAGS = -g -O0 -DDEBUG_BUILD

# Source and object files
SRC_DIR = source
SRC = $(shell find $(SRC_DIR) -name '*.cpp')
COMMON_SRC = $(filter-out source/main.cpp source/client_main.cpp source/server_main.cpp, $(SRC))
COMMON_OBJ = $(COMMON_SRC:.cpp=.o)

SERVER_OBJ = $(COMMON_OBJ) source/server_main.o
CLIENT_OBJ = $(COMMON_OBJ) source/client_main.o

SERVER_EXEC = server
CLIENT_EXEC = client

SRC_NO_MAIN = $(COMMON_SRC)
OBJ = $(SERVER_OBJ) $(CLIENT_OBJ)

# Test directory and test files
TEST_DIR = tests
TESTSRC = $(wildcard $(TEST_DIR)/*_test.cpp)
ALL_TEST_SRC = $(TESTSRC) $(SRC_NO_MAIN)
TESTOBJ = $(ALL_TEST_SRC:.cpp=.o)
TESTEXEC = test_program

# Build targets
all: $(SERVER_EXEC) $(CLIENT_EXEC)

$(SERVER_EXEC): $(SERVER_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(CLIENT_EXEC): $(CLIENT_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

# %.o: %.cpp
# 	$(CXX) -c $(CXXFLAGS) $< -o $@
%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) $< -o $@

# Debug build target
debug: CXXFLAGS += $(DEBUG_FLAGS)  # Add debug flags to CXXFLAGS
debug: $(SERVER_EXEC) $(CLIENT_EXEC)

# Compile and run all gtest files
test: $(TESTOBJ) 
	$(CXX) -o $(TESTEXEC) $(TESTOBJ) $(LDFLAGS) -lgtest -lgtest_main -pthread
	./$(TESTEXEC)

$(TESTDIR)/%.o: $(TESTDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

# Clean up build files
clean:
	        rm -f $(OBJ) $(SERVER_EXEC) $(CLIENT_EXEC) $(TESTOBJ) $(TESTEXEC)

.PHONY: all clean

