#ifndef OS_H
#define OS_H

struct kbd;

extern struct kbd *usbopen(void);
extern void usbclose(struct kbd *kbd);
extern void usbsend(struct kbd *kbd, const void *data);

#endif
