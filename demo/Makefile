CFLAGS := -std=gnu11 -g -Wall -I../include
DIR_LIBRARY := ../lib
LDFLAGS := -L$(DIR_LIBRARY) -lrpata -luuid -lpthread 

SRCS := main.c 
all := bin

.PHONY: bin
bin:
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o demo.bin

.PHONY: clean
clean:
	$(RM) demo.bin
