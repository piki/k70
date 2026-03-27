#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <IOKit/hid/IOHIDLib.h>
#include "os.h"

// The K70's vendor-specific HID interface (lighting commands) uses this page.
#define CORSAIR_USAGE_PAGE 0xFFC2

struct kbd {
    IOHIDManagerRef mgr;
    IOHIDDeviceRef  dev;
};

static int cfnum(IOHIDDeviceRef dev, CFStringRef key) {
    CFNumberRef ref = IOHIDDeviceGetProperty(dev, key);
    int val = 0;
    if (ref) CFNumberGetValue(ref, kCFNumberIntType, &val);
    return val;
}

// CFSet order is non-deterministic; iterate all and prefer the vendor page.
static IOHIDDeviceRef pick_device(IOHIDManagerRef mgr) {
    CFSetRef set = IOHIDManagerCopyDevices(mgr);
    if (!set) return NULL;

    CFIndex count = CFSetGetCount(set);
    if (count == 0) { CFRelease(set); return NULL; }

    IOHIDDeviceRef *devs = calloc(count, sizeof(*devs));
    CFSetGetValues(set, (const void **)devs);
    CFRelease(set);

    IOHIDDeviceRef best = NULL;
    for (CFIndex i = 0; i < count; i++) {
        int page = cfnum(devs[i], CFSTR(kIOHIDPrimaryUsagePageKey));
        if (page == CORSAIR_USAGE_PAGE) { best = devs[i]; break; }
        if (!best) best = devs[i];  // fallback if vendor page not found
    }

    free(devs);
    return best;
}

struct kbd *usbopen(void) {
    IOHIDManagerRef mgr = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!mgr) { fprintf(stderr, "IOHIDManagerCreate failed\n"); return NULL; }

    CFMutableDictionaryRef matching = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    int vendor = VENDOR, product = PRODUCT;
    CFNumberRef vref = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &vendor);
    CFNumberRef pref = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &product);
    CFDictionarySetValue(matching, CFSTR(kIOHIDVendorIDKey),  vref);
    CFDictionarySetValue(matching, CFSTR(kIOHIDProductIDKey), pref);
    CFRelease(vref);
    CFRelease(pref);

    IOHIDManagerSetDeviceMatching(mgr, matching);
    CFRelease(matching);

    IOReturn ret = IOHIDManagerOpen(mgr, kIOHIDOptionsTypeSeizeDevice);
    if (ret != kIOReturnSuccess) {
        fprintf(stderr, "IOHIDManagerOpen(SeizeDevice) failed: 0x%x — retrying without seize\n", ret);
        ret = IOHIDManagerOpen(mgr, kIOHIDOptionsTypeNone);
        if (ret != kIOReturnSuccess) {
            fprintf(stderr, "IOHIDManagerOpen failed: 0x%x\n", ret);
            CFRelease(mgr);
            return NULL;
        }
    }

    IOHIDDeviceRef dev = pick_device(mgr);
    if (!dev) {
        fprintf(stderr, "No matching devices found.\n");
        IOHIDManagerClose(mgr, 0);
        CFRelease(mgr);
        return NULL;
    }

    CFStringRef nameRef = IOHIDDeviceGetProperty(dev, CFSTR(kIOHIDProductKey));
    if (nameRef) {
        char name[256];
        CFStringGetCString(nameRef, name, sizeof(name), kCFStringEncodingUTF8);
        printf("Device: \"%s\"\n", name);
    }

    struct kbd *kbd = malloc(sizeof(*kbd));
    kbd->mgr = mgr;
    kbd->dev = dev;
    return kbd;
}

void usbclose(struct kbd *kbd) {
    if (!kbd) return;
    if (kbd->mgr) {
        IOHIDManagerClose(kbd->mgr, 0);
        CFRelease(kbd->mgr);
    }
    free(kbd);
}

// Feature report (SET_REPORT on control endpoint), report ID = 0, full 64 bytes.
// The device doesn't use HID report IDs; bytes[0] is just the first data byte.
void usbsend(struct kbd *kbd, const void *data) {
    IOReturn ret = IOHIDDeviceSetReport(kbd->dev, kIOHIDReportTypeFeature,
                                        0, data, MSG_SIZE);
    if (ret != kIOReturnSuccess)
        fprintf(stderr, "usbsend failed: 0x%x\n", ret);
}
