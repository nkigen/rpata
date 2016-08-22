CROSS_COMPILE :=
CC := $(CROSS_COMPILE)gcc

DIR_SOURCE  := src
DIR_INCLUDE := include
DIR_LIBRARY := lib

CFLAGS  ?= -g -O0 -std=gnu11 -Wall -Wextra -fpic
CFLAGS  += -I$(DIR_INCLUDE)
LDFLAGS ?=
LDFLAGS += -shared
LDFLAGS += -luuid -lpthread

LIBS := $(DIR_LIBRARY)/librpata.so $(DIR_LIBRARY)/librpata.a

SRCS :=
include $(DIR_SOURCE)/Makefile

OBJS := $(SRCS:.c=.o)

default: rpata_ini $(OBJS) $(LIBS) demo

.PHONY: rpata_ini
rpata_ini:
	mkdir -p $(DIR_LIBRARY)

$(DIR_LIBRARY)/%.a: $(OBJS)
	mkdir -p $(DIR_LIBRARY)
	$(AR) rcs $@ $^

$(DIR_LIBRARY)/%.so: $(OBJS)
	mkdir -p $(DIR_LIBRARY)
	$(LD) $^ $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $< -c $(CFLAGS) -o $@

.PHONY: demo
demo: $(LIBS)
	$(MAKE) -C demo/

.PHONY: clean
clean:
	$(RM) -r $(DIR_LIBRARY)
	$(RM) -r $(OBJS)
	$(MAKE) clean -C demo
