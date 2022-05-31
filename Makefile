CC			:= gcc
CFLAGS		:= -I. -Os

HEADERS 	:= $(shell find . -name "*.h")
SRC			:= $(shell find . -name "*.c")

OBJ_DIR 	:= obj
OBJ_NAMES	:= $(SRC:.c=.o)
OBJ_PATHS	:= $(OBJ_NAMES:./%=$(OBJ_DIR)/%)

$(OBJ_DIR)/%.o: %.c $(SRC) $(HEADERS)
	@mkdir -p $(shell dirname "$@")
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(OBJ_PATHS)
	$(CC) $(CFLAGS) -o gible $(OBJ_PATHS)
	@echo Done.

.PHONY: all

clean:
	rm -rf $(OBJ_DIR)
	rm -f gible
