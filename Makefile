LDLIBS=-ludev
CFLAGS=-Wall -Werror

all: linux

linux: linux.o
	$(CC) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o linux
