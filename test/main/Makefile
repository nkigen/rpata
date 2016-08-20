CFLAGS := -std=gnu11 -g -I../../include -I../../src
LDFLAGS := -luuid -lpthread
all := bin

.PHONY: bin
bin:
	$(CC) $(CFLAGS) main.c ../../src/impl.c $(LDFLAGS) -o test.bin

.PHONY: clean
clean:
	$(RM) test.bin
