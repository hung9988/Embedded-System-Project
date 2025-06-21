#include "tusb.h"
#include "usb_descriptors.h"
#include <hid.h>
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


void hid_init() {
	 tusb_rhport_init_t dev_init = {
	     .role = TUSB_ROLE_DEVICE,
	     .speed = TUSB_SPEED_AUTO
	  };
	  tusb_init(0, &dev_init); // initialize device stack on roothub port 0
}

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
    modifiers |= key->layers[layer].value;
    should_send_keyboard_report = 1;
    break;

  case KEY_TYPE_NORMAL:
    for (uint8_t i = 0; i < 6; i++) {
      if (keycodes[i] == 0) {
        keycodes[i] = key->layers[layer].value;
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
    consumer_report = key->layers[layer].value;
    should_send_consumer_report = 1;
    break;

  default:
    break;
  }
}

void hid_release_key(struct key *key, uint8_t layer) {
  switch (key->layers[layer].type) {
  case KEY_TYPE_MODIFIER:
    modifiers &= ~key->layers[layer].value;
    should_send_keyboard_report = 1;
    break;

  case KEY_TYPE_NORMAL:
    for (uint8_t i = 0; i < 6; i++) {
      if (keycodes[i] == key->layers[layer].value) {
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

  default:
    break;
  }
}

// Invoked when received SET_PROTOCOL request
// protocol is either HID_PROTOCOL_BOOT (0) or HID_PROTOCOL_REPORT (1)
void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol) {
  (void)instance;
  (void)protocol;

  // nothing to do since we use the same compatible boot report for both Boot and Report mode.
  // TOOD set a indicator for user
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
//uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
//{
//  // TODO not Implemented
//  (void)instance;
//  (void)report_id;
//  (void)report_type;
//  (void)buffer;
//  (void)reqlen;
//
//  return 0;
//}
//
//// Invoked when received SET_REPORT control request or
//// received data on OUT endpoint ( Report ID = 0, Type = 0 )
//void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
//{
//  (void)report_id;
//
//  // keyboard interface
//  if (instance == HID_INSTANCE_KEYBOARD)
//  {
//    // Set keyboard LED e.g Capslock, Numlock etc...
//    if (report_type == HID_REPORT_TYPE_OUTPUT)
//    {
//      // bufsize should be (at least) 1
//      if (bufsize < 1)
//        return;
//
//      uint8_t const kbd_leds = buffer[0];
//
//      if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
//      {
//        // Capslock On: disable blink, turn led on
//
//        echo_all((uint8_t *)"CapsON\n", 7);
//      }
//      else
//      {
//        echo_all((uint8_t *)"CapsOFF\n", 8);
//      }
//    }
//  }
//}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) instance;
  (void) len;

  uint8_t report_id = report[0];

  switch (report_id)
  {
    case REPORT_ID_KEYBOARD:
      // Keyboard report sent successfully
      break;

    case REPORT_ID_CONSUMER_CONTROL:
      // Consumer control report sent successfully
      break;

    default:
      break;
  }
}

// CDC

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  (void)itf;

  // connected
  if (dtr && rts)
  {
    // print initial message when connected
    tud_cdc_write_str("\r\nTinyUSB WebUSB device example\r\n");
  }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
  (void)itf;
}
