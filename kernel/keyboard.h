#ifndef __OBOS_KEYBOARD_H
#define __OBOS_KEYBOARD_H

#include "types.h"

// Zero on success, and -1 if the driver was already initialized.
INT kKeyboardInit(SIZE_T bufferSize);

#endif