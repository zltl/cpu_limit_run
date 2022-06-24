.PHONY: clean all cpu_limit_run test

.ONESHELL:

ROOT_DIR:=$(shell dirname $(realpath $(firstword) $(MAKEFILE_LIST)))
ifeq ($(TARGET_DIR),)
    TARGET_DIR = $(ROOT_DIR)/target
endif

$(shell mkdir -p $(TARGET_DIR))

ifneq ($(DEBUG),)
	DEBUG_FLAGS = -DDEBUG
	SANITIZER_FLAGS = -fsanitize=address -lasan
endif

COMMON_FLAGS += -Wall -Wextra -Werror -ggdb -Wno-unused-result \
	-I$(ROOT_DIR)/src $(DEBUG_FLAGS) $(SANITIZER_FLAGS)

CFLAGS += -std=c11 $(COMMON_FLAGS)
CXXFLAGS += -std=c++17 $(COMMON_FLAGS)
LDFLAGS ?=

export

cpu_limit_run:
	$(MAKE) -C $(ROOT_DIR)/src

$(TARGET_DIR)/loop: test/loop.c
	$(CC) $(CFLAGS) $^ -o $@

test: cpu_limit_run $(TARGET_DIR)/loop
	$(TARGET_DIR)/cpu_limit_run --percent 20 -- $(TARGET_DIR)/loop

clean:
	rm -rf $(ROOT_DIR)/target
