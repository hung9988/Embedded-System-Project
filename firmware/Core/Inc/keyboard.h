#ifndef __KEYBOARD_H
#define __KEYBOARD_H

#include "config.h"
#include <stdint.h>

#define CALIBRATION_CYCLES 20

#define IDLE_VALUE_APPROX 1800
#define MAX_DISTANCE_APPROX 500
#define IDLE_VALUE_OFFSET 10
#define MAX_DISTANCE_OFFSET 60
#define IDLE_CYCLES_UNTIL_SLEEP 15

#define XXXX 0xff
#define ____ 0x00

#define SPECIAL(X) (0b1000000000000000 | X)

struct __attribute__((__packed__)) calibration {
  uint16_t cycles_count;
  uint16_t idle_value;
  uint16_t max_distance;
};

enum direction {
  GOING_UP,
  GOING_DOWN,
};

struct __attribute__((__packed__)) state {
  uint16_t value;
  uint16_t distance;
  uint8_t distance_8bits;
  float filtered_distance;
  int8_t velocity;
  uint8_t filtered_distance_8bits;
};

enum actuation_status {
  STATUS_MIGHT_BE_TAP,
  STATUS_TAP,
  STATUS_TRIGGERED,
  STATUS_RESET,
  STATUS_RAPID_TRIGGER_RESET
};

struct __attribute__((__packed__)) actuation {
  enum direction direction;
  uint8_t direction_changed_point;
  enum actuation_status status;
  uint8_t reset_offset;
  uint8_t trigger_offset;
  uint8_t rapid_trigger_offset;
  uint32_t triggered_at;
};

enum key_type {
  KEY_TYPE_EMPTY,
  KEY_TYPE_NORMAL,
  KEY_TYPE_MODIFIER,
  KEY_TYPE_CONSUMER_CONTROL,
  KEY_TYPE_MACRO,
};

struct __attribute__((__packed__)) layer {
  enum key_type type;
  uint16_t value[MAX_MACRO_LEN];
};

enum {
  _BASE_LAYER,
  _TAP_LAYER,
  LAYERS_COUNT
};

struct __attribute__((__packed__)) key {
  uint8_t is_enabled;
  uint8_t row;
  uint8_t column;
  uint8_t idle_counter;
  uint8_t is_idle;
  struct layer layers[LAYERS_COUNT];
  struct calibration calibration;
  struct state state;
  struct actuation actuation;
};

struct user_config {
  uint8_t reverse_magnet_pole;
  uint8_t trigger_offset;
  uint8_t reset_threshold;
  uint8_t rapid_trigger_offset;
  uint16_t tap_timeout;
  uint16_t keymaps[LAYERS_COUNT][MATRIX_ROWS][MATRIX_COLS][MAX_MACRO_LEN];
};

void keyboard_task();
void keyboard_init_keys();
extern void keyboard_read_config();
extern uint8_t keyboard_write_config(uint8_t *buffer, uint16_t offset, uint16_t size);
extern void keyboard_select_amux(uint8_t amux_channel);
extern void keyboard_select_adc(uint8_t adc_channel);
extern uint16_t keyboard_read_adc();
extern void keyboard_close_adc();
extern uint32_t keyboard_get_time();

#endif /* __KEYBOARD_H */
