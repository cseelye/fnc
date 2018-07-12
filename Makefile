# Default build type
BUILD?=debug

CXX=g++
CXXFLAGS.all=-Wall -Werror -std=c++17 -I. -I libs/catch2 -I libs/spdlog/include -I libs/CLI11 -I /opt/boost_1_67_0
CXXFLAGS.debug=-Og -g
CXXFLAGS.release=-O3
LDFLAGS=-pthread -static -L /opt/boost_1_67_0/stage/lib
LDLIBS=-lboost_system-mt
EXECUTABLE=fnc
TEST_EXECUTABLE=fnctest

# Set compile flags based on build type
CXXFLAGS=$(CXXFLAGS.all) $(CXXFLAGS.$(BUILD))

# Output directories
BUILD_BASE_DIR=build
OBJ_DIR=$(BUILD_BASE_DIR)/$(BUILD)/obj
BIN_DIR=$(BUILD_BASE_DIR)/$(BUILD)/bin
TEST_DIR=$(BUILD_BASE_DIR)/$(BUILD)/test
SRC_SUBDIRS = $(filter-out $(BUILD_BASE_DIR)/, $(wildcard */))
#DEP_DIR=$(BUILD_BASE_DIR)/$(BUILD)/dep
ALL_DIRS=$(BUILD_BASE_DIR) $(OBJ_DIR) $(BIN_DIR) $(TEST_DIR) $(DEP_DIR) $(SRC_SUBDIRS)

ALL_SOURCES=$(wildcard *.cpp) $(wildcard */*.cpp)
ALL_DEPS=$(addprefix $(OBJ_DIR)/,$(ALL_SOURCES:.cpp=.d))
#DEPS=$(addprefix $(OBJ_DIR)/,$(ALL_SOURCES:.cpp=.d))
BIN_SOURCES=$(filter-out $(wildcard test*), $(wildcard *.cpp)) $(filter-out $(wildcard test*) $(wildcard */*test.cpp), $(wildcard */*.cpp))
BIN_OBJECTS=$(addprefix $(OBJ_DIR)/,$(BIN_SOURCES:.cpp=.o))
TEST_SOURCES=$(filter-out main.cpp, $(wildcard *.cpp)) $(wildcard */*.cpp)
TEST_OBJECTS=$(addprefix $(OBJ_DIR)/,$(TEST_SOURCES:.cpp=.o))
#TEST_DEPS=$(addprefix $(OBJ_DIR)/,$(SOURCES:.cpp=.d))


# BUILD_DIR=build
# OUTPUT_DIR=$(BUILD_BASE_DIR)/$(BUILD)
# SOURCES=$(filter-out $(wildcard test*), $(wildcard *.cpp))
# OBJECTS=$(addprefix $(OUTPUT_DIR)/,$(SOURCES:.cpp=.o))
# DEPS=$(addprefix $(OUTPUT_DIR)/,$(SOURCES:.cpp=.d))
# EXECUTABLE=fnc

# TEST_DIR=$(BUILD_BASE_DIR)/test
# TEST_OUTPUT_DIR=$(TEST_DIR)/$(BUILD)
# TEST_SOURCES=$(filter-out main.cpp, $(wildcard *.cpp))
# TEST_OBJECTS=$(addprefix $(TEST_OUTPUT_DIR)/,$(TEST_SOURCES:.cpp=.o))
# TEST_DEPS=$(addprefix $(TEST_OUTPUT_DIR)/,$(TERST_SOURCES:.cpp=.d))

.DEFAULT_GOAL=all
.SECONDEXPANSION:
.PHONY: all clean doc rebuild retest

-include $(ALL_DEPS)
#-include $(DEPS)
#-include $(TEST_DEPS)

# Default target is the main executable
#all: $(SOURCES) $(OUTPUT_DIR)/$(EXECUTABLE)
all: $(EXECUTABLE)

# Helpful debug rule to print the value of a variable
print-%  : ; @echo $* = $($*)

# Create the output directories
#$(OUTPUT_DIR)/ $(TEST_OUTPUT_DIR)/:
$(BUILD_BASE_DIR)/ $(OBJ_DIR)/ $(BIN_DIR)/ $(TEST_DIR)/ $(DEP_DIR)/ $(OBJ_DIR)/$(SRC_SUBDIRS)/:
	mkdir -p $@


#$(OUTPUT_DIR)%/ $(TEST_OUTPUT_DIR)%/:
#$(BUILD_BASE_DIR)%/ $(OBJ_DIR)%/ $(BIN_DIR)%/ $(TEST_DIR)%/ $(DEP_DIR)%/:
#	mkdir -p $@

# Objects
#$(OUTPUT_DIR)/%.o: %.cpp | $$(@D)/
#$(OBJ_DIR)/%.o: %.cpp | $$(@D)/
$(OBJ_DIR)/%.o: %.cpp | $$(@D)/
	$(CXX) -MMD $(CXXFLAGS) $< -c -o $@

$(OBJ_DIR)/%.o: %.cpp | $$(@D)/
	$(CXX) -MMD $(CXXFLAGS) $< -c -o $@

#%.o: %.cpp $(OBJ_DIR)/
#	$(CXX) -MMD $(CXXFLAGS) $< -c -o $(OBJ_DIR)/$@


#$(EXECUTABLE) : $(OUTPUT_DIR)/$(EXECUTABLE)
$(EXECUTABLE) : $(BIN_DIR)/$(EXECUTABLE)

#$(OUTPUT_DIR)/$(EXECUTABLE): $(OBJECTS)
$(BIN_DIR)/$(EXECUTABLE): $(BIN_OBJECTS)  | $$(@D)/
	$(CXX) $(BIN_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@

# Test objects/binary
#$(TEST_OUTPUT_DIR)/%.o: %.cpp | $$(@D)/
#	$(CXX) -MMD $(CXXFLAGS) $< -c -o $@

#$(TEST_EXECUTABLE) : $(TEST_OUTPUT_DIR)/$(TEST_EXECUTABLE)
$(TEST_EXECUTABLE) : $(TEST_DIR)/$(TEST_EXECUTABLE)

#$(TEST_OUTPUT_DIR)/$(TEST_EXECUTABLE): $(TEST_OBJECTS)
$(TEST_DIR)/$(TEST_EXECUTABLE): $(TEST_OBJECTS)  | $$(@D)/
	$(CXX) $(TEST_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@

TEST_ARGS:=
TRACE:=0
DEBUG:=0
TEST_SPEC:=
ifeq ($(DEBUG), 1)
	TEST_ARGS+= --show-debug
endif
ifeq ($(TRACE), 1)
	TEST_ARGS+= --show-trace
endif
TEST_ARGS+= '$(TEST_SPEC)'

# Run tests
test: $(TEST_EXECUTABLE)
	./$(TEST_DIR)/$(TEST_EXECUTABLE) --durations=yes --use-colour=yes $(TEST_ARGS)

# Remove all artifacts
clean:
	$(RM) -r $(BUILD_BASE_DIR) docs *.o *.d
#	$(RM) -r $(TEST_DIR) $(BUILD_DIR)

# Force a rebuild of everything
rebuild: clean
	$(MAKE) all

retest: clean
	$(MAKE) test

# Create the documentation
doc:
	doxygen
