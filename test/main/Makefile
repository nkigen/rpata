CFLAGS := -std=gnu11 -g -Wall -I../../include -I../../src
LDFLAGS := -luuid -lpthread

DIR_SRC := ../../src
SRCS := $(wildcard $(DIR_SRC)/*.c) main.c 
all := bin

.PHONY: bin
bin:
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o test.bin

.PHONY: clean
clean:
	$(RM) test.bin
