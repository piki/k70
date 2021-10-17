#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libudev.h>
#include <linux/usbdevice_fs.h>
#include <sys/ioctl.h>

#define MSG_SIZE 64
#define VENDOR 0x1b1c // Corsair
#define PRODUCT 0x1b13 // K70

static struct udev *udev;
struct kbd {
  int fd;
  struct udev_device *dev;
  int num_interfaces;
};

int match_device(struct udev_device *dev) {
  const char* vendor = udev_device_get_sysattr_value(dev, "idVendor");
  const char* product = udev_device_get_sysattr_value(dev, "idProduct");
  if (vendor == NULL || product == NULL)
    return 0;

  ushort pid, vid;
  pid = vid = 0;
  if(!(sscanf(vendor, "%04hx", &vid) == 1 && vid))
    return 0;
  if(!(sscanf(product, "%04hx", &pid) == 1 && pid))
    return 0;

  printf("vid=%x pid=%x\n", vid, pid);
  return vid == VENDOR && pid == PRODUCT;
}

struct kbd *usbopen_dev(struct udev_device *dev) {
  const char* path = udev_device_get_devnode(dev);
  const char* syspath = udev_device_get_syspath(dev);
  if(!path || !syspath || path[0] == 0 || syspath[0] == 0){
    perror("Failed to get device path");
    return NULL;
  }

  int fd = open(path, O_RDWR);
  if (fd < 0) {
    perror("open");
    return NULL;
  }

  const char* name = udev_device_get_sysattr_value(dev, "product");
  printf("Device name: \"%s\"\n", name);
  const char* serial = udev_device_get_sysattr_value(dev, "serial");
  printf("Serial number: \"%s\"\n", serial);
  const char* firmware = udev_device_get_sysattr_value(dev, "bcdDevice");
  printf("Firmware version: \"%s\"\n", firmware);
  const char* ep_str = udev_device_get_sysattr_value(dev, "bNumInterfaces");
  printf("Interface count: \"%s\"\n", firmware);

  struct kbd *ret = (struct kbd*)malloc(sizeof(struct kbd));
  ret->fd = fd;
  ret->dev = dev;
  ret->num_interfaces = ep_str ? atoi(ep_str) : 0;

  for(int i = 0; i < ret->num_interfaces; i++){
    struct usbdevfs_ioctl ctl = { i, USBDEVFS_DISCONNECT, 0 };
    ioctl(ret->fd, USBDEVFS_IOCTL, &ctl);
    if(ioctl(ret->fd, USBDEVFS_CLAIMINTERFACE, &i)) {
      fprintf(stderr, "Failed to claim interface %d: %s", i, strerror(errno));
    }
  }

  return ret;
}

struct kbd *usbopen() {
  udev = udev_new();
  if (!udev) {
    perror("udev_new");
    return NULL;
  }

  struct udev_enumerate* enumerator = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerator, "usb");
  udev_enumerate_scan_devices(enumerator);
  struct udev_list_entry* devices, *dev_list_entry;
  devices = udev_enumerate_get_list_entry(enumerator);

  struct kbd *ret = NULL;
  udev_list_entry_foreach(dev_list_entry, devices) {
    const char* path = udev_list_entry_get_name(dev_list_entry);
    if(!path) continue;
    struct udev_device* dev = udev_device_new_from_syspath(udev, path);
    if(!dev) continue;

    // If the device matches a recognized device ID, open it
    if (match_device(dev)) {
      ret = usbopen_dev(dev);
      // FIXME: leaking the dev here
    }
    if (ret) break;

    udev_device_unref(dev);
  }
  udev_enumerate_unref(enumerator);

  if (!ret)
    fprintf(stderr, "No matching devices found.\n");
  return ret;
}

void usbclose(struct kbd *kbd) {
  if (kbd) {
    for (int i = 0; i < kbd->num_interfaces; i++) {
        ioctl(kbd->fd, USBDEVFS_RELEASEINTERFACE, &i);
    }

    struct usbdevfs_ioctl ctl = { 0, USBDEVFS_CONNECT, 0 };
    ioctl(kbd->fd, USBDEVFS_IOCTL, &ctl);
    ctl.ifno = 1;
    ioctl(kbd->fd, USBDEVFS_IOCTL, &ctl);

    close(kbd->fd);
    udev_device_unref(kbd->dev);
    free(kbd);
  }

  if (udev) {
    udev_unref(udev);
    udev = NULL;
  }
}

void usbsend(struct kbd *kbd, const void *data) {
  struct usbdevfs_bulktransfer transfer = {0};
  transfer.ep = kbd->num_interfaces;
  transfer.len = MSG_SIZE;
  transfer.timeout = 5000;
  transfer.data = (void *)data;
  int res = ioctl(kbd->fd, USBDEVFS_BULK, &transfer);
  if (res < 0) {
    perror("usbsend");
  } else if (res < MSG_SIZE) {
    fprintf(stderr, "short send: res=%d\n", res);
  }
}

struct onekey {
  int r, g, b;
};

typedef struct onekey keymap[144];

/*
static unsigned char packets[5][64] = {
  { 0x7f, 0x01, 0x3c, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77 },
  { 0x7f, 0x02, 0x3c, 0x00, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x7f, 0x03, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x7f, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x07, 0x27, 0x00, 0x00, 0xd8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};
*/

void fill_packets(keymap *map, unsigned char packets[5][64]) {
  int i;
  for (i=0; i<5; i++)
    bzero(packets[i], 64);

  memcpy(packets[0], "\x7f\x01\x3c\0", 4);
  memcpy(packets[1], "\x7f\x02\x3c\0", 4);
  memcpy(packets[2], "\x7f\x03\x3c\0", 4);
  memcpy(packets[3], "\x7f\x04\x24\0", 4);
  memcpy(packets[4], "\x07\x27\0\0\xd8", 5);

  for (i=0; i<72; i++) {
    int ri = 0*72 + i;
    int gi = 1*72 + i;
    int bi = 2*72 + i;
    packets[ri/60][4+(ri%60)] = ((7-(*map)[i*2].r)<<4) + 7-(*map)[i*2+1].r;
    packets[gi/60][4+(gi%60)] = ((7-(*map)[i*2].g)<<4) + 7-(*map)[i*2+1].g;
    packets[bi/60][4+(bi%60)] = ((7-(*map)[i*2].b)<<4) + 7-(*map)[i*2+1].b;
  }
}

// 0: backtick/tilde
// 1: escape
// 2: caps lock
// 3: tab
// 4: lctrl
// 5: lshift
// 6: equals/plus
// 7: F12
// 8: KP_7
// 9: windows lock indicator
// 10: 
// 11: 
// 12: 1
// 13: F1
// 14: A
// 15: Q
// 16: lmeta
// 17: 
// 18: 
// 19: prtscn/sysrq
// 20: KP_8
// 21: mute
// 22: 
// 23: 
// 24: 2
// 25: F2
// 26: S
// 27: W
// 28: lalt
// 29: Z
// 30: backspace
// 31: scroll lock
// 32: KP_9
// 33: media_stop
// 34: 
// 35: 
// 36: 3
// 37: F3
// 38: D
// 39: E
// 40: 
// 41: X
// 42: Delete
// 43: pause/break
// 44: 
// 45: media_rewind
// 46: 
// 47: 
// 48: 4
// 49: F4
// 50: F
// 51: R
// 52: space
// 53: C
// 54: End
// 55: Insert
// 56: KP_4
// 57: media_play/pause
// 58: 
// 59: 
// 60: 5
// 61: F5
// 62: G
// 63: T
// 64: 
// 65: V
// 66: pgdown
// 67: Home
// 68: KP_5
// 69: media_ffwd
// 70: 
// 71: 
// 72: 6
// 73: F6
// 74: H
// 75: Y
// 76: 
// 77: B
// 78: rshift
// 79: Page Up
// 80: KP_6
// 81: Num Lock
// 82: 
// 83: 
// 84: 7
// 85: F7
// 86: J
// 87: U
// 88: ralt
// 89: N
// 90: rctrl
// 91: ] }
// 92: KP_1
// 93: KP_slash
// 94: 
// 95: 
// 96: 8
// 97: F8
// 98: K
// 99: I
// 100: rmeta
// 101: M
// 102: arrow_up
// 103: pipe/backslash
// 104: KP_2
// 105: KP_star
// 106: 
// 107: 
// 108: 9
// 109: F9
// 110: L
// 111: O
// 112: menu
// 113: <
// 114: arrow_left
// 115: 
// 116: KP_3
// 117: KP_minus
// 118: 
// 119: 
// 120: 0
// 121: F10
// 122: : ;
// 123: P
// 124: 
// 125: >
// 126: arrow_down
// 127: enter
// 128: KP_0
// 129: KP_plus
// 130: 
// 131: 
// 132: - _
// 133: F11
// 134: ' "
// 135: [ {
// 136: brightness
// 137: / ?
// 138: arrow_right
// 139: 
// 140: KP_period / delete
// 141: KP_enter
// 142: 
// 143: 

static const char *rows[7] = {
  "                                                   136 9          21",
  "1   13 25 37 49   61 73 85 97   109 121 133 7      19  31 43   33 45  57  69",
  "0   12 24 36 48 60 72 84 96 108 120   132 6 30     55  67 79   81 93  105 117",
  "3   15 27 39 51 63 75 87 99 111 123   135 91 103   42  54 66   8  20  32  129",
  "2   14 26 38 50 62 74 86 98 110    122 134 127                 56 68  80",
  "5      29 41 53 65 77 89 101   113 125 137 78         102      92 104 116 141",
  "4 16 28           52           88 100 112 90      114 126 138  128    140",
};

static const struct { const char *name, *values; } keysets[] = {
  { "function keys", "13 25 37 49   61 73 85 97   109 121 133 7" },
  { "numbers", "12 24 36 48 60 72 84 96 108 120" },
  { "letters", "15 27 39 51 63 75 87 99 111 123   14 26 38 50 62 74 86 98 110    29 41 53 65 77 89 101" },
  { "arrow keys", "102 114 126 138" },
  { "keypad", "81 93 105 117   8 20 32 129   56 68 80   92 104 116 141   128 140" },
};

void set_keys(const char *keylist, struct onekey clr, keymap *map) {
  char *dup = strdup(keylist);
  for (const char *p=strtok(dup, " "); p; p=strtok(NULL, " ")) {
    int code = atoi(p);
    (*map)[code] = clr;
  }
  free(dup);
}

void make_rainbow(keymap *map) {
  bzero(map, sizeof(*map));
  struct onekey row_colors[7] = {
    { .r=7, .g=7, .b=7 },
    { .r=7, .g=0, .b=0 },
    { .r=7, .g=3, .b=0 },
    { .r=7, .g=7, .b=0 },
    { .r=0, .g=7, .b=0 },
    { .r=0, .g=0, .b=7 },
    { .r=7, .g=0, .b=7 },
  };
  for (int i=0; i<7; i++) {
    set_keys(rows[i], row_colors[i], map);
  }
}

void make_sunset(keymap *map) {
  bzero(map, sizeof(*map));
  struct onekey blue = { .r=0, .g=0, .b=5 };
  struct onekey red = { .r=6, .g=0, .b=0 };
  struct onekey orange = { .r=6, .g=4, .b=0 };
  struct onekey yellow = { .r=6, .g=6, .b=0 };
  for (int i=0; i<144; i++) {
    (*map)[i] = blue;
  }

  set_keys(keysets[1].values, orange, map);
  set_keys(keysets[2].values, red, map);
  set_keys(keysets[3].values, yellow, map);
  set_keys(keysets[4].values, orange, map);
}

void usage(const char *prog) {
  printf("Usage:\n  %s {rainbow|drainbow|sunset}\n", prog);
  exit(1);
}

int main(int argc, char **argv) {
  keymap map;
  bzero(&map, sizeof(map));
  if (argc != 2)
    usage(argv[0]);

  if (!strcmp(argv[1], "rainbow"))
    make_rainbow(&map);
  else if (!strcmp(argv[1], "sunset"))
    make_sunset(&map);
  else
    usage(argv[0]);

  struct kbd *kbd = usbopen();
  if (!kbd) {
    usbclose(NULL);
    return 1;
  }

  unsigned char packets[5][64];

  fill_packets(&map, packets);

  usbsend(kbd, packets[0]);
  usbsend(kbd, packets[1]);
  usbsend(kbd, packets[2]);
  usbsend(kbd, packets[3]);
  usbsend(kbd, packets[4]);

  usbclose(kbd);

  return 0;
}
