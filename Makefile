CFLAGS=-Wall -Werror

uname_S := $(shell uname -s)
ifeq ($(uname_S),Darwin)
OS=mac
LDLIBS=-framework IOKit -framework CoreFoundation -framework AppKit
else
OS=linux
LDLIBS=-ludev
endif

all: k70

k70: $(OS).o common.o
	$(CC) -o $@ $^ $(LDLIBS)

install: k70
	sed -e 's!<CMD>!'`pwd`'/k70 planetscale!g' < udev.rules > /etc/udev/rules.d/99-k70.rules
	chmod 755 /etc/udev/rules.d/99-k70.rules
	udevadm control --reload-rules
	udevadm trigger

clean:
	rm -f *.o k70
