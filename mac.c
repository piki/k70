#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <CoreFoundation/CFNumber.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOMessage.h>
#include <IOKit/hid/IOHIDDevicePlugIn.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include "os.h"

struct kbd {
};

// globals
static CFRunLoopRef mainloop = 0;
static IONotificationPortRef notify = 0;

typedef IOUSBDeviceInterface182 **usb_dev_t;
typedef IOHIDDeviceDeviceInterface **hid_dev_t;

// Hacky way of trying something over and over again until it works. 100ms intervals, max 1s
#define wait_loop(error, operation)  do {                                                                                   \
    int trial = 0;                                                                                                          \
    while(((error) = (operation)) != kIOReturnSuccess &&  operation != kIOReturnNoResources) {                              \
        if (++trial == 10)                                                                                                  \
            break;                                                                                                          \
	usleep(100000);                                                                                                     \
    } } while(0)

static struct kbd *add_usb(usb_dev_t handle, io_object_t **rm_notify);
static void iterate_devices_usb(void* context, io_iterator_t iterator);

struct kbd *usbopen(void) {
	int vendor = VENDOR;
	notify = IONotificationPortCreate(kIOMasterPortDefault);
	mainloop = CFRunLoopGetCurrent();
	CFRunLoopAddSource(mainloop, IONotificationPortGetRunLoopSource(notify), kCFRunLoopDefaultMode);
  
	CFMutableDictionaryRef match = IOServiceMatching(kIOUSBDeviceClassName);
	CFNumberRef cfvendor = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &vendor);
	CFDictionarySetValue(match, CFSTR(kUSBVendorName), cfvendor);
	CFRelease(cfvendor);
	CFMutableArrayRef cfproducts = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

	// for each supported product...
	int product = PRODUCT;
	CFNumberRef cfproduct = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &product);
	CFArrayAppendValue(cfproducts, cfproduct);
	CFRelease(cfproduct);

	CFDictionarySetValue(match, CFSTR(kUSBProductIdsArrayName), cfproducts);
	CFRelease(cfproducts);

	io_iterator_t iterator_usb = 0;
	IOReturn res = IOServiceAddMatchingNotification(notify, kIOMatchedNotification, match, iterate_devices_usb, 0, &iterator_usb);
	if (res != kIOReturnSuccess) {
		printf("Failed to list USB devices: %x\n", res);
		return NULL;
	}

	// Iterate existing devices
	if (iterator_usb)
		iterate_devices_usb(0, iterator_usb);

	// WRITEME: HID stuff from usbmain, usb_mac.c:1031 on

	return NULL;
}

void usbclose(struct kbd *kbd) {
}

void usbsend(struct kbd *kbd, const void *data) {
}

static struct kbd *add_usb(usb_dev_t handle, io_object_t **rm_notify) {
    UInt16 idvendor, idproduct;
    UInt32 location;
    (*handle)->GetDeviceVendor(handle, &idvendor);
    (*handle)->GetDeviceProduct(handle, &idproduct);
    (*handle)->GetLocationID(handle, &location);
		printf("got a USB device: vendor=%x product=%x location=%x\n", idvendor, idproduct, location);

    // WRITEME: borrow from add_usb

    return NULL;
}

static void iterate_devices_usb(void* context, io_iterator_t iterator) {
	io_service_t device;
	while((device = IOIteratorNext(iterator)) != 0) {
		IOCFPlugInInterface** plugin = 0;
		SInt32 score = 0;
		kern_return_t err;
		wait_loop(err, IOCreatePlugInInterfaceForService(device, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugin, &score));
		if (err != kIOReturnSuccess) {
			printf("Failed to create device plugin: %x\n", err);
			goto release;
		}
		// Get the device interface
		usb_dev_t handle;
		wait_loop(err, (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID182), (LPVOID*)&handle));
		if (err != kIOReturnSuccess) {
			printf("QueryInterface failed: %x\n", err);
			goto release;
		}
		// Plugin is no longer needed
		IODestroyPlugInInterface(plugin);

		err = (*handle)->USBDeviceOpenSeize(handle);
		if (err == kIOReturnExclusiveAccess) {
			// We can't send control transfers but most of the other functions should work
			printf("Unable to seize USB handle, continuing anyway...\n");
		} else if (err != kIOReturnSuccess) {
			printf("USBDeviceOpen failed: %x\n", err);
			continue;
		}
		// Connect it
		io_object_t* rm_notify = 0;
		struct kbd *kb = add_usb(handle, &rm_notify);
		if (!kb) {
			// If unsuccessful, release it now
			(*handle)->USBDeviceClose(handle);
		}
release:
		IOObjectRelease(device);
	}
}

#if 0
static void closeusb(struct kbd *kb) {
	//WRITEME
	// Close HID handles
	int count = kb->epcount_hid;
	for (int i=0; i<count; i++) {
		hid_dev_t iface = kb->ifhid[i];
		if (iface) {
			(*iface)->close(iface, kIOHIDOptionsTypeNone);
			(*iface)->Release(iface);
			kb->ifhid[i] = 0;
		}
	}
	kb->epcount_hid = 0;
	// Close USB handles
	count = kb->epcount_usb;
	for (int i=0; i<count; i++) {
		usb_iface_t iface = kb->ifusb[i];
		if (iface) {
			(*iface)->USBInterfaceClose(iface);
			(*iface)->Release(iface);
			kb->ifusb[i] = 0;
		}
	}
	kb->epcount_usb = 0;
	usb_dev_t iface = kb->handle;
	if (iface) {
		(*iface)->USBDeviceClose(iface);
		(*iface)->Release(iface);
		kb->handle = 0;
	}
	// Close input thread
	if (kb->input_loop) {
		CFRunLoopStop(kb->input_loop);
		kb->input_loop = 0;
	}
	// WRITEME: free kb
}
#endif
