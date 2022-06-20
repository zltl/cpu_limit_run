tarname = cpu_limit_run

sources_c = $(wildcard *.c)
objs_c = $(patsubst %.c,$(TARGET_DIR)/%.c.o,$(sources_c))
$(shell mkdir -p $(TARGET_DIR))

all: $(TARGET_DIR)/cpu_limit_run

$(TARGET_DIR)/%.c.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
$(TARGET_DIR)/cpu_limit_run: $(objs_c)
	$(CC) $(CFLAGS) -o $@ $^
