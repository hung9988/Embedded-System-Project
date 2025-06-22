#ifndef __HID_H
#define __HID_H

#include "keyboard.h"
#include <stdint.h>

void hid_init();
void hid_task();
void hid_press_key(struct key *key, uint8_t layer);
void hid_release_key(struct key *key, uint8_t layer);

#endif /* __HID_H */
