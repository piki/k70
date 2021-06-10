#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <CoreFoundation/CFNumber.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include "os.h"

// globals
static CFRunLoopRef mainloop = 0;
static IONotificationPortRef notify = 0;

typedef IOUSBDeviceInterface182** usb_dev_t;

// Hacky way of trying something over and over again until it works. 100ms intervals, max 1s
#define wait_loop(error, operation)  do {                                                                                   \
    int trial = 0;                                                                                                          \
    while(((error) = (operation)) != kIOReturnSuccess &&  operation != kIOReturnNoResources){                               \
        if(++trial == 10)                                                                                                   \
            break;                                                                                                          \
	usleep(100000);                                                                                                     \
    } } while(0)

static void iterate_devices_usb(void* context, io_iterator_t iterator){
	io_service_t device;
	while((device = IOIteratorNext(iterator)) != 0){
		IOCFPlugInInterface** plugin = 0;
		SInt32 score = 0;
		kern_return_t err;
		wait_loop(err, IOCreatePlugInInterfaceForService(device, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugin, &score));
		if(err != kIOReturnSuccess){
			printf("Failed to create device plugin: %x\n", err);
			goto release;
		}
		// Get the device interface
		usb_dev_t handle;
		wait_loop(err, (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID182), (LPVOID*)&handle));
		if(err != kIOReturnSuccess){
			printf("QueryInterface failed: %x\n", err);
			goto release;
		}
		// Plugin is no longer needed
		IODestroyPlugInInterface(plugin);

		err = (*handle)->USBDeviceOpenSeize(handle);
		if(err == kIOReturnExclusiveAccess){
			// We can't send control transfers but most of the other functions should work
			printf("Unable to seize USB handle, continuing anyway...\n");
		} else if(err != kIOReturnSuccess){
			printf("USBDeviceOpen failed: %x\n", err);
			continue;
		}
#if 0
		// Connect it
		io_object_t* rm_notify = 0;
		usbdevice* kb = add_usb(handle, &rm_notify);
		if(kb){
			// If successful, register for removal notification
			IOServiceAddInterestNotification(notify, device, kIOGeneralInterest, remove_device, kb, rm_notify);
		} else
			// Otherwise, release it now
#else
    UInt16 idvendor, idproduct;
    UInt32 location;
    (*handle)->GetDeviceVendor(handle, &idvendor);
    (*handle)->GetDeviceProduct(handle, &idproduct);
    (*handle)->GetLocationID(handle, &location);
		printf("got a USB device: vendor=%x product=%x location=%x\n", idvendor, idproduct, location);
#endif
			(*handle)->USBDeviceClose(handle);
release:
		IOObjectRelease(device);
	}
}


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
	if(iterator_usb)
		iterate_devices_usb(0, iterator_usb);



	return NULL;
}

void usbclose(struct kbd *kbd) {
}

void usbsend(struct kbd *kbd, const void *data) {
}
