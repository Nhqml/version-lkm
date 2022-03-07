#ifndef _VERSION_H
#define _VERSION_H

#include <linux/ioctl.h>

// THREEEEEE IS A MAGIIC NUMBER
#define MAGIC_NUMBER 3

// Define IOCTL
#define VERSION_MODIFIED _IOR(MAGIC_NUMBER, 0, int)
#define VERSION_RESET _IO(MAGIC_NUMBER, 1)

#endif /* _VERSION_H */