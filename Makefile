# Makefile to build 'shipz' project

# Compiler and flags
UNAME_S := $(shell uname -s)

CXX = g++
CXXFLAGS_BASE = -std=c++17 -I/usr/local/include -DSHAREPATH="\"./\"" -I./include/ -I./source/
LDFLAGS_BASE = -L/usr/local/lib -lSDL3 -lSDL3_image -lSDL3_mixer -lSDL3_ttf
RPATH = /usr/local/lib

ifeq ($(UNAME_S),Linux)
    CXXFLAGS = $(CXXFLAGS_BASE) -I/usr/include
    LDFLAGS = $(LDFLAGS_BASE) -Wl,-rpath,$(RPATH) -L/usr/lib/x86_64-linux-gnu
else ifeq ($(UNAME_S),Darwin)
    CXXFLAGS = $(CXXFLAGS_BASE) -I/opt/homebrew/Cellar/googletest/1.15.2/include
    LDFLAGS = $(LDFLAGS_BASE) -Wl,-rpath,$(RPATH) -L/opt/homebrew/Cellar/googletest/1.15.2/lib/
else
    CXXFLAGS = $(CXXFLAGS_BASE)
    LDFLAGS = $(LDFLAGS_BASE) -Wl,-rpath,$(RPATH)
endif

DEBUG_FLAGS = -g -O0 -DDEBUG_BUILD

# Source and object files
COMMON_SRC = \
    $(wildcard source/common/*.cpp) \
    $(wildcard source/messages/*.cpp) \
    $(wildcard source/net/*.cpp) \
    $(wildcard source/utils/*.cpp)

SERVER_SRC = $(COMMON_SRC) $(wildcard source/server/*.cpp) source/server_main.cpp
CLIENT_SRC = $(COMMON_SRC) $(wildcard source/client/*.cpp) source/client_main.cpp

SERVER_OBJ = $(SERVER_SRC:.cpp=.o)
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)

SERVER_EXEC = server
CLIENT_EXEC = client

SRC_NO_MAIN = $(COMMON_SRC) $(wildcard source/server/*.cpp) $(wildcard source/client/*.cpp)
OBJ = $(SERVER_OBJ) $(CLIENT_OBJ)

# Test directory and test files
TEST_DIR = tests
TESTSRC = $(wildcard $(TEST_DIR)/*_test.cpp)
ALL_TEST_SRC = $(TESTSRC) $(SRC_NO_MAIN)
TESTOBJ = $(ALL_TEST_SRC:.cpp=.o)
TESTEXEC = test_program

# Build targets
all: $(SERVER_EXEC) $(CLIENT_EXEC)

# Build only the server or only the client
server: $(SERVER_EXEC)
client: $(CLIENT_EXEC)

$(CLIENT_EXEC): CXXFLAGS += -DCLIENT

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

.PHONY: all clean server client

