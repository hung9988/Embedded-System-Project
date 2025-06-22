#include "hid.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include <keyboard.h>
#include <stdlib.h>

extern uint8_t const desc_ms_os_20[];
extern struct key keyboard_keys[ADC_CHANNEL_COUNT][AMUX_CHANNEL_COUNT];
extern struct user_config keyboard_user_config;
extern uint32_t keyboard_last_cycle_duration;

static uint8_t should_send_consumer_report = 0;
static uint8_t should_send_keyboard_report = 0;

static uint8_t modifiers = 0;
static uint8_t keycodes[6] = {0};
// static uint8_t is_screaming = 0;
static uint8_t consumer_report = 0;

void hid_task() {

  if ((should_send_consumer_report || should_send_keyboard_report) && tud_hid_ready()) {
    if (tud_suspended()) {
      tud_remote_wakeup();
    } else {
      if (should_send_consumer_report) {
        should_send_consumer_report = 0;
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &consumer_report, 2);
      } else if (should_send_keyboard_report) {
        should_send_keyboard_report = 0;
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifiers, keycodes);
      }
    }
  }
}

void hid_press_key(struct key *key, uint8_t layer) {
  switch (key->layers[layer].type) {
  case KEY_TYPE_MODIFIER:
    modifiers |= key->layers[layer].value[0];
    should_send_keyboard_report = 1;
    break;

  case KEY_TYPE_NORMAL:
    for (uint8_t i = 0; i < 6; i++) {
      if (keycodes[i] == 0) {
        keycodes[i] = key->layers[layer].value[0];
        // if the key is violently pressed, automatically add the MAJ modifier :)
        // if (is_screaming) {
        //   is_screaming = 0;
        //   modifiers &= ~get_bitmask_for_modifier(HID_KEY_SHIFT_LEFT);
        // } else if (i == 0 && key->state.velocity > keyboard_user_config.screaming_velocity_trigger) {
        //   is_screaming = 1;
        //   modifiers |= get_bitmask_for_modifier(HID_KEY_SHIFT_LEFT);
        // }
        should_send_keyboard_report = 1;
        break;
      }
    }
    break;

  case KEY_TYPE_CONSUMER_CONTROL:
    consumer_report = key->layers[layer].value[0];
    should_send_consumer_report = 1;
    break;

  case KEY_TYPE_MACRO:
    // Count how many non-zero macro values we have (excluding modifiers)
    uint8_t macro_count = 0;
    for (uint8_t i = 0; i < MAX_MACRO_LEN; i++) {
      if (key->layers[layer].value[i] != ____) {
        // Check if this is a modifier
        uint8_t bitmask = get_bitmask_for_modifier(key->layers[layer].value[i]);
        if (!bitmask) {
          macro_count++; // Only count non-modifier keys
        }
      }
    }

    // Find the first empty slot for the macro
    uint8_t start_slot = 0;
    for (uint8_t i = 0; i < 6; i++) {
      if (keycodes[i] == 0) {
        start_slot = i;
        break;
      }
    }

    // Check if we have enough consecutive empty slots for the non-modifier keys
    uint8_t available_slots = 0;
    for (uint8_t i = start_slot; i < 6; i++) {
      if (keycodes[i] == 0) {
        available_slots++;
      } else {
        break; // Stop counting if we hit a non-empty slot
      }
    }

    // Only proceed if we have enough slots for the non-modifier keys
    if (available_slots >= macro_count) {
      // Process macro values
      uint8_t macro_idx = 0;
      for (uint8_t i = start_slot; i < 6 && macro_idx < MAX_MACRO_LEN; macro_idx++) {
        if (key->layers[layer].value[macro_idx] != ____) {
          // Check if this is a modifier
          uint8_t bitmask = get_bitmask_for_modifier(key->layers[layer].value[macro_idx]);
          if (bitmask) {
            // Set modifier bit
            modifiers |= bitmask;
            should_send_keyboard_report = 1;
          } else {
            // Place non-modifier key in keycodes array
            keycodes[i] = key->layers[layer].value[macro_idx];
            i++; // Move to next keycodes slot
            should_send_keyboard_report = 1;
          }
        }
      }
    }
    break;

  default:
    break;
  }
}

void hid_release_key(struct key *key, uint8_t layer) {
  switch (key->layers[layer].type) {
  case KEY_TYPE_MODIFIER:
    modifiers &= ~key->layers[layer].value[0];
    should_send_keyboard_report = 1;
    break;

  case KEY_TYPE_NORMAL:
    for (uint8_t i = 0; i < 6; i++) {
      if (keycodes[i] == key->layers[layer].value[0]) {
        keycodes[i] = 0;
        // if (is_screaming) {
        //   is_screaming = 0;
        //   modifiers &= ~get_bitmask_for_modifier(HID_KEY_SHIFT_LEFT);
        // }
        should_send_keyboard_report = 1;
        break;
      }
    }
    break;

  case KEY_TYPE_CONSUMER_CONTROL:
    consumer_report = 0;
    should_send_consumer_report = 1;
    break;

  case KEY_TYPE_MACRO:
    // Process macro values for release
    for (uint8_t macro_idx = 0; macro_idx < MAX_MACRO_LEN; macro_idx++) {
      if (key->layers[layer].value[macro_idx] != ____) {
        // Check if this is a modifier
        uint8_t bitmask = get_bitmask_for_modifier(key->layers[layer].value[macro_idx]);
        if (bitmask) {
          // Clear modifier bit
          modifiers &= ~bitmask;
          should_send_keyboard_report = 1;
        } else {
          // Remove non-modifier key from keycodes array
          for (uint8_t i = 0; i < 6; i++) {
            if (keycodes[i] == key->layers[layer].value[macro_idx]) {
              keycodes[i] = 0;
              should_send_keyboard_report = 1;
              break;
            }
          }
        }
      }
    }
    break;

  default:
    break;
  }
}

// Invoked when received SET_PROTOCOL request
// protocol is either HID_PROTOCOL_BOOT (0) or HID_PROTOCOL_REPORT (1)

/// CALLBACKS

void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol) {
  (void)instance;
  (void)protocol;
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len) {
  (void)instance;
  (void)len;
}
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
  (void)instance;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
  (void)instance;
  (void)report_type;
  (void)buffer;
  (void)bufsize;
}
