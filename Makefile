TOOLCHAIN 	?= 

ifeq (windows, $(MAKECMDGOALS))
TOOLCHAIN	:= x86_64-w64-mingw32
endif

ifeq (,$(TOOLCHAIN))
CC			:= gcc
else
CC			:= $(TOOLCHAIN)-gcc
endif
CFLAGS		:= -I. -O3 -std=c99 -ffunction-sections -Wall -Wextra -MMD

SRC			:= $(shell find . -name "*.c")

OBJ_DIR 	:= build
OBJ_NAMES	:= $(SRC:.c=.o)
OBJ_PATHS	:= $(OBJ_NAMES:./%=$(OBJ_DIR)/%)
DEP_NAMES	:= $(OBJ_PATHS:%.o=%.d)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(shell dirname "$@")
	$(CC) $(CFLAGS) -c $< -o $@

windows: all

all: $(DEP_PATHS) $(OBJ_PATHS)
	$(CC) $(CFLAGS) -o gible $(OBJ_PATHS)
	@echo Done.

.PHONY: all

-include $(DEP_NAMES)

clean:
	rm -rf $(OBJ_DIR)
	rm -f gible
