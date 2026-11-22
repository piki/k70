This is a work in progress.

The goal is to create a simple command-line utility that can send a static
colormap to a Corsair K70 RGB keyboard.

## Building

### Linux

You need a C compiler, `make`, `pkg-config`, and the `libudev` development
headers. On Debian/Ubuntu:

```bash
sudo apt install build-essential pkg-config libudev-dev
make
```

### macOS

Install the Xcode command line tools.
Run `make`

### Windows (MSYS2 UCRT64)

Install [MSYS2](https://www.msys2.org/), open the **MSYS2 UCRT64** shell, and
install the toolchain and HIDAPI:

```bash
pacman -Syu
pacman -S --needed \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-hidapi \
  mingw-w64-ucrt-x86_64-pkgconf \
  make \
  git
```

Then build:

```bash
git clone https://github.com/piki/k70.git
cd k70
make
```

This produces `k70.exe`. Run it from the same shell, for example:

```bash
./k70.exe rainbow
```

The Windows build uses HIDAPI to talk to the keyboard's lighting interface
(interface 1, usage page `0xFFC2`). Close Corsair iCUE or any other software
that may have that interface open before running.
