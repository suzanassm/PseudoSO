CXX ?= c++
AR ?= ar
CPPFLAGS += -Iinclude
CXXFLAGS ?= -std=c++11 -Wall -Wextra -Wpedantic -O2

BUILD_DIR := build
SOURCES := $(filter-out src/main.cpp,$(wildcard src/*.cpp))
OBJECTS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
LIBRARY := $(BUILD_DIR)/libpseudoso.a
FILE_TEST := $(BUILD_DIR)/teste_arquivos

.PHONY: all test test-files dispatcher clean

# Enquanto o despachante não existe, compila os módulos como biblioteca.
all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(AR) rcs $@ $^

$(BUILD_DIR)/%.o: src/%.cpp
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(FILE_TEST): testes/teste_arquivos.cpp $(LIBRARY) include/arquivos.hpp include/processo.hpp include/fila.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< $(LIBRARY) $(LDFLAGS) $(LDLIBS) -o $@

test: test-files

test-files: $(FILE_TEST)
	./$(FILE_TEST)

# Disponível quando a Pessoa 3 adicionar src/main.cpp.
dispatcher: $(BUILD_DIR)/dispatcher

$(BUILD_DIR)/dispatcher: $(BUILD_DIR)/main.o $(LIBRARY)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

clean:
	$(RM) -r $(BUILD_DIR)

-include $(OBJECTS:.o=.d) $(BUILD_DIR)/main.d
