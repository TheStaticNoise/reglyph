# wtf is this? idk, is it portable? Yes
# 

.PHONY: all debug release build

CXX      ?= g++
CC       ?= gcc
PKG_CONFIG ?= pkg-config

ROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD := $(ROOT)

GLAD_SRC := $(ROOT)../external/glad/src/gl.c
GLAD_INC := $(ROOT)../external/glad/include

PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags glfw3 freetype2)
PKG_LIBS   := $(shell $(PKG_CONFIG) --libs glfw3 freetype2 gl)

CPPFLAGS += -I$(GLAD_INC) $(PKG_CFLAGS)
CXXFLAGS ?= -Wall -Wextra -g -O0
CFLAGS   ?= -Wall -Wextra
LDLIBS   += $(PKG_LIBS)


all: debug

debug:
	$(MAKE) CXXFLAGS="-Wall -Wextra -g -O0" build

release:
	$(MAKE) CXXFLAGS="-Wall -Wextra -O3 -DNDEBUG" build
# FUCK CHATGPT FOR CHANGING THE OUTPUT PATH 2 MFCKING TIMES
# going back to ./reglyph XD
# lesson: don't rely on chatgpt without looking at his shit under a microscope
build:
	@mkdir -p $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) \
		main.cpp ../external/glad/src/gl.c \
		$(LDFLAGS) $(LDLIBS) \
		-o $(ROOT)/reglyph
