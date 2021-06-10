#ifndef OS_H
#define OS_H

#define MSG_SIZE 64
#define VENDOR 0x1b1c // Corsair
#define PRODUCT 0x1b13 // K70

struct kbd;

extern struct kbd *usbopen(void);
extern void usbclose(struct kbd *kbd);
extern void usbsend(struct kbd *kbd, const void *data);

#endif
