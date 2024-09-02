# This file is part of the MicroPython project, http://micropython.org/
# The MIT License (MIT)
# Copyright (c) 2022-2023 Damien P. George
#
# This is a very simple makefile that demonstrates how to build the embed port.
# All it needs to do is build all *.c files in the micropython_embed directory.
# This makefile would be replaced with your custom build system.

EMBED_DIR = micropython_embed
PROG = embed

CFLAGS += -I.
CFLAGS += -I$(EMBED_DIR)
CFLAGS += -I$(EMBED_DIR)/port
CFLAGS += -Wall -Og -fno-common

CXXFLAGS += -I.
CXXFLAGS += -I$(EMBED_DIR)
CXXFLAGS += -I$(EMBED_DIR)/port
CXXFLAGS += -Wall -Og -fno-common -std=c++17

SRC += main.c
SRC += $(wildcard $(EMBED_DIR)/*/*.c) $(wildcard $(EMBED_DIR)/*/*/*.c)
CPP_SRC += $(wildcard $(EMBED_DIR)/*/*.cpp) $(wildcard $(EMBED_DIR)/*/*/*.cpp)
OBJ += $(SRC:.c=.o)
OBJ += $(CPP_SRC:.cpp=.o)

$(PROG): $(OBJ)
	$(CC) -o $@ $^

clean:
	/bin/rm -f $(OBJ) $(PROG)
