CFLAGS=-Wall -Werror

uname_S := $(shell uname -s)
ifeq ($(uname_S),Darwin)
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
