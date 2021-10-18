#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libudev.h>
#include <linux/usbdevice_fs.h>
#include <sys/ioctl.h>
#include "os.h"

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
