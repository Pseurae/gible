CC			:= x86_64-w64-mingw32-gcc
CFLAGS		:= -I. -O3 -ffunction-sections -Wall -Wextra -MMD

HEADERS 	:= $(shell find . -name "*.h")
SRC			:= $(shell find . -name "*.c")

OBJ_DIR 	:= build
OBJ_NAMES	:= $(SRC:.c=.o)
OBJ_PATHS	:= $(OBJ_NAMES:./%=$(OBJ_DIR)/%)
DEP_NAMES	:= $(OBJ_PATHS:%.o=%.d)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(shell dirname "$@")
	$(CC) $(CFLAGS) -c $< -o $@

all: $(DEP_PATHS) $(OBJ_PATHS)
	$(CC) $(CFLAGS) -o gible $(OBJ_PATHS)
	@strip gible
	@echo Done.

.PHONY: all

-include $(DEP_NAMES)

clean:
	rm -rf $(OBJ_DIR)
	rm -f gible
