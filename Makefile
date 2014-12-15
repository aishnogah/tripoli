CXX := g++
CXXFLAGS := -ggdb3 -std=c++11 # -Wall
LIB := -L/usr/local/lib -lfst -ldl -lfstscript
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
FST := data/input.txt
# FST := examples/linear.txt
PDT := data/pdt.txt
# PDT := examples/translate.txt
RUN_CMD := src/main $(FST) $(PDT) data/arc-labels.txt data/grammar-symbols.txt data/rules.txt data/states.txt data/parens.txt output.fst

all: $(TARGET)

# Compilation

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INC) -Isrc -c $^ -o $@

# Linking

$(TARGET): $(SRC_OBJECTS) $(TARGET).o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIB)

$(TEST_TARGET): $(OBJECTS) $(TEST_TARGET).o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIB) -lgtest

# Phony

run: $(TARGET)
	$(RUN_CMD)

debug: $(TARGET)
	gdb $(GDB_FLAGS) --args $(RUN_CMD)

test: $(TEST_TARGET)
	$<

clean:
	rm -rf $(OBJECTS) $(MAINS) $(TARGETS) $(patsubst %,%.dSYM,$(MAINS)) 

valgrind:
	valgrind $(RUN_CMD)

.PHONY: run test clean debug valgrind
