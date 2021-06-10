CFLAGS=-Wall -Werror

uname_S := $(shell uname -s)
ifeq ($(uname_S),Darwin)
LDLIBS=-framework IOKit -framework CoreFoundation -framework AppKit
all: mac
else
LDLIBS=-ludev
all: linux
endif

linux: linux.o common.o
	$(CC) -o $@ $^ $(LDLIBS)

mac: mac.o common.o
	$(CC) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o linux mac
