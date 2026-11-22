#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hidapi.h>
#include "os.h"

/*
 * The Corsair K70 RGB (VID 0x1b1c, PID 0x1b13) exposes its lighting
 * control interface as HID interface 1 with usage page 0xFFC2.
 * HIDAPI cannot select an interface by number, so enumerate the
 * device and open the matching path.
 */
#define K70_LIGHTING_INTERFACE   1
#define K70_LIGHTING_USAGE_PAGE  0xFFC2

/*
 * common.c produces 64-byte protocol packets.  HID output reports
 * include a leading report ID (0x00 here), so the on-wire report is
 * 65 bytes.
 */
#define REPORT_SIZE (MSG_SIZE + 1)

struct kbd {
    hid_device *dev;
};

static int is_lighting_interface(const struct hid_device_info *info) {
    if (info->interface_number != K70_LIGHTING_INTERFACE)
        return 0;

    /*
     * Some HIDAPI backends leave usage_page unset.  In that case the
     * interface number is the best signal we have.
     */
    if (info->usage_page != 0 &&
        info->usage_page != K70_LIGHTING_USAGE_PAGE)
        return 0;

    return 1;
}

struct kbd *usbopen(void) {
    if (hid_init() != 0) {
        fprintf(stderr, "hid_init failed\n");
        return NULL;
    }

    struct hid_device_info *devs = hid_enumerate(VENDOR, PRODUCT);
    struct hid_device_info *cur;
    struct hid_device_info *match = NULL;

    for (cur = devs; cur; cur = cur->next) {
        fprintf(stderr,
                "hid: iface=%d usage_page=0x%04x usage=0x%04x "
                "product=\"%ls\" path=%s\n",
                cur->interface_number,
                (unsigned int)cur->usage_page,
                (unsigned int)cur->usage,
                cur->product_string ? cur->product_string : L"(null)",
                cur->path ? cur->path : "(null)");

        if (!match && is_lighting_interface(cur))
            match = cur;
    }

    if (!match) {
        fprintf(stderr,
                "No K70 RGB lighting interface found "
                "(VID 0x%04x PID 0x%04x, interface %d).\n",
                VENDOR, PRODUCT, K70_LIGHTING_INTERFACE);
        hid_free_enumeration(devs);
        return NULL;
    }

    hid_device *dev = hid_open_path(match->path);
    if (!dev) {
        fprintf(stderr, "hid_open_path failed for %s\n", match->path);
        hid_free_enumeration(devs);
        return NULL;
    }

    hid_free_enumeration(devs);

    struct kbd *kbd = calloc(1, sizeof(*kbd));
    if (!kbd) {
        hid_close(dev);
        return NULL;
    }
    kbd->dev = dev;

    /*
     * Switch the keyboard to software lighting mode.  Without this the
     * firmware may keep rendering its hardware profile and ignore the
     * color stream.  This is the same LightingControl packet OpenRGB
     * sends for Corsair peripherals.
     */
    unsigned char lighting[REPORT_SIZE];
    memset(lighting, 0, sizeof(lighting));
    lighting[0] = 0x00; /* report ID */
    lighting[1] = 0x07; /* command: write */
    lighting[2] = 0x05; /* property: lighting control */
    lighting[3] = 0x02; /* software mode */
    lighting[5] = 0x03; /* keyboard brightness */
    if (hid_write(kbd->dev, lighting, sizeof(lighting)) < 0) {
        fprintf(stderr, "hid_write(lighting control): %ls\n",
                hid_error(kbd->dev));
        /* Non-fatal: some firmware accepts color packets directly. */
    }

    return kbd;
}

void usbclose(struct kbd *kbd) {
    if (kbd) {
        if (kbd->dev)
            hid_close(kbd->dev);
        free(kbd);
    }
    hid_exit();
}

void usbsend(struct kbd *kbd, const void *data) {
    if (!kbd || !kbd->dev)
        return;

    unsigned char report[REPORT_SIZE];
    report[0] = 0x00; /* report ID */
    memcpy(&report[1], data, MSG_SIZE);

    int res = hid_write(kbd->dev, report, sizeof(report));
    if (res < 0) {
        fprintf(stderr, "hid_write: %ls\n", hid_error(kbd->dev));
    } else if (res != (int)sizeof(report)) {
        fprintf(stderr, "short hid_write: %d of %zu bytes\n",
                res, sizeof(report));
    }
}
