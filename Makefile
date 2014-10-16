CXX := g++
CXXFLAGS := -g -std=c++11 # -Wall
LIB := -L/usr/local/lib -lfst -ldl
INC := -I/usr/local/include

TARGET := src/main
TEST_TARGET := test/all-tests
TARGETS := $(TARGET) $(TEST_TARGET)
MAINS := $(addsuffix .o,$(TARGETS))

SRC_SOURCES := $(shell find src -name '*.cpp')
SRC_OBJECTS := $(filter-out $(MAINS),$(SRC_SOURCES:.cpp=.o))

TST_SOURCES := $(shell find test -name '*.cpp')
TST_OBJECTS := $(filter-out $(MAINS),$(TST_SOURCES:.cpp=.o))

OBJECTS := $(SRC_OBJECTS) $(TST_OBJECTS)

all: $(TARGET)

# Compilation

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INC) -Isrc -c $^ -o $@

# Linking

$(TARGET): $(SRC_OBJECTS) $(TARGET).o
	$(CXX) $(CXXFLAGS) $(LIB) $^ -o $@

$(TEST_TARGET): $(OBJECTS) $(TEST_TARGET).o
	$(CXX) $(CXXFLAGS) $(LIB) -lgtest $^ -o $@

# Phony
	
test: $(TEST_TARGET)
	$<

clean:
	rm -rf $(OBJECTS) $(MAINS) $(TARGETS) $(patsubst %,%.dSYM,$(MAINS)) 

.PHONY: test clean