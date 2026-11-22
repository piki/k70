CFLAGS=-Wall -Werror

uname_S := $(shell uname -s)
ifeq ($(uname_S),Darwin)
OS=mac
LDLIBS=-framework IOKit -framework CoreFoundation -framework AppKit
else ifneq (,$(findstring MINGW,$(uname_S))$(findstring MSYS,$(uname_S)))
OS=windows
CFLAGS += $(shell pkg-config --cflags hidapi)
LDLIBS := $(shell pkg-config --libs hidapi)
else
OS=linux
LDLIBS=-ludev
endif

all: k70

k70: $(OS).o common.o
	$(CC) -o $@ $^ $(LDLIBS)

ifeq ($(OS),linux)
install: k70
	sed -e 's!<CMD>!'`pwd`'/k70 planetscale!g' < udev.rules > /etc/udev/rules.d/99-k70.rules
	chmod 755 /etc/udev/rules.d/99-k70.rules
	udevadm control --reload-rules
	udevadm trigger
endif

clean:
	rm -f *.o k70 k70.exe
