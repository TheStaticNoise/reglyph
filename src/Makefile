# wtf is this? idk, is it portable? Yes

.PHONY: all release clean

CXX      ?= g++
CC       ?= gcc
PKG_CONFIG ?= pkg-config

ROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD := $(ROOT)build

GLAD_SRC := $(ROOT)../external/glad/src/gl.c
GLAD_INC := $(ROOT)../external/glad/include

PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags glfw3 freetype2)
PKG_LIBS   := $(shell $(PKG_CONFIG) --libs glfw3 freetype2 gl)

CPPFLAGS += -I$(GLAD_INC) $(PKG_CFLAGS)
CXXFLAGS ?= -Wall -Wextra -g -O0
CFLAGS   ?= -Wall -Wextra
LDLIBS   += $(PKG_LIBS)

all: $(BUILD)/reglyph

release: CXXFLAGS := -Wall -Wextra -O3 -DNDEBUG
release: clean all

$(BUILD):
	mkdir -p $@

$(BUILD)/gl.o: $(GLAD_SRC) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/main.o: $(ROOT)main.cpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/reglyph: $(BUILD)/main.o $(BUILD)/gl.o
	$(CXX) $^ $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -rf $(BUILD)

