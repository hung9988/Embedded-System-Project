#include "keyboard.h"
#include "hid.h"
#include <class/hid/hid.h>
#include <stdio.h>
#include <stdlib.h>

struct key keyboard_keys[ADC_CHANNEL_COUNT][AMUX_CHANNEL_COUNT] = {0};
struct user_config keyboard_user_config = {
    .reverse_magnet_pole = 0,
    .trigger_offset = 150,
    .reset_threshold = 3,
    .rapid_trigger_offset = 40,
    .tap_timeout = 200,
    .keymaps = {
        // clang-format off
		        [_BASE_LAYER] = {
								{{HID_KEY_0}, {HID_KEY_1}, {HID_KEY_2}, {HID_KEY_3}},
    			                {{HID_KEY_4}, {HID_KEY_5}, {HID_KEY_6}, {HID_KEY_7}},
    			                {{HID_KEY_8}, {HID_KEY_9}, {HID_KEY_A}, {HID_KEY_D}},
    			                {{HID_KEY_CONTROL_LEFT}, {HID_KEY_SHIFT_LEFT}, {HID_LAYER_CHANGE}, {HID_MODE_CHANGE}},
    			            },
            [_TAP_LAYER] = {
							  {{____}, {____}, {____}, {____}},
                {{____}, {____}, {____}, {____}},
                {{____}, {____}, {____}, {____}},
                {{____}, {____}, {____}, {____}},
            },
            [_ALT_LAYER] = {
                {{HID_KEY_A}, {HID_KEY_B}, {HID_KEY_C}, {HID_KEY_D}},
                {{HID_KEY_E}, {HID_KEY_F}, {HID_KEY_G}, {HID_KEY_H}},
                {{HID_KEY_J}, {HID_KEY_K}, {HID_KEY_L}, {HID_KEY_M}},
                {{HID_KEY_CONTROL_LEFT}, {HID_KEY_SHIFT_LEFT}, {HID_LAYER_CHANGE}, {HID_MODE_CHANGE}},
            },
            [_ALT_LAYER_2] = {
                { {HID_KEY_N}, {HID_KEY_O}, {HID_KEY_P}, {HID_KEY_Q} },
                { {HID_KEY_R}, {HID_KEY_S}, {HID_KEY_T}, {HID_KEY_U} },
                { {HID_KEY_V}, {HID_KEY_W}, {HID_KEY_X}, {HID_KEY_Y} },
                { {HID_KEY_Z}, {HID_KEY_CONTROL_LEFT}, {HID_LAYER_CHANGE}, {HID_MODE_CHANGE} },
            },
        // clang-format on
    }};

uint32_t keyboard_last_cycle_duration = 0;

static uint8_t key_triggered = 0;

struct key *current_pressed_key = NULL;

uint8_t get_bitmask_for_modifier(uint8_t keycode) {
  switch (keycode) {
  case HID_KEY_CONTROL_LEFT:
    return 0b00000001;
  case HID_KEY_SHIFT_LEFT:
    return 0b00000010;
  case HID_KEY_ALT_LEFT:
    return 0b00000100;
  case HID_KEY_GUI_LEFT:
    return 0b00001000;
  case HID_KEY_CONTROL_RIGHT:
    return 0b00010000;
  case HID_KEY_SHIFT_RIGHT:
    return 0b00100000;
  case HID_KEY_ALT_RIGHT:
    return 0b01000000;
  case HID_KEY_GUI_RIGHT:
    return 0b10000000;
  default:
    return 0b00000000;
  }
}

uint16_t get_usage_consumer_control(uint16_t value) {
  if (value > 0xFF) {
    return value & 0b0111111111111111;
  } else {
    return 0;
  }
}

void init_key(uint8_t adc_channel, uint8_t amux_channel, uint8_t row, uint8_t column) {

  struct key *key = &keyboard_keys[adc_channel][amux_channel];

  key->is_enabled = 1;
  key->is_idle = 0;
  key->row = row;
  key->column = column;

  key->calibration.cycles_count = 0;
  key->calibration.idle_value = IDLE_VALUE_APPROX;
  key->calibration.max_distance = MAX_DISTANCE_APPROX;

  key->actuation.status = STATUS_RESET;
  key->actuation.trigger_offset = keyboard_user_config.trigger_offset;
  key->actuation.reset_offset = keyboard_user_config.trigger_offset - keyboard_user_config.reset_threshold;
  key->actuation.rapid_trigger_offset = keyboard_user_config.rapid_trigger_offset;

  for (uint8_t i = 0; i < LAYERS_COUNT; i++) {
    if (keyboard_user_config.keymaps[i][row][column][0] != ____) {
      // Check if this is a macro (multiple non-zero elements)
      uint8_t macro_count = 0;
      for (uint8_t j = 0; j < MAX_MACRO_LEN; j++) {
        if (keyboard_user_config.keymaps[i][row][column][j] != ____) {
          macro_count++;
        }
      }

      if (macro_count > 1) {
        // This is a macro - copy all values
        key->layers[i].type = KEY_TYPE_MACRO;
        for (uint8_t j = 0; j < MAX_MACRO_LEN; j++) {
          key->layers[i].value[j] = keyboard_user_config.keymaps[i][row][column][j];
        }
      } else {
        // Single key - check type
        uint16_t usage_consumer_control = get_usage_consumer_control(
            keyboard_user_config.keymaps[i][row][column][0]);
        if (usage_consumer_control) {
          key->layers[i].type = KEY_TYPE_CONSUMER_CONTROL;
          key->layers[i].value[0] = usage_consumer_control;
        } else {
          uint8_t bitmask = get_bitmask_for_modifier(
              keyboard_user_config.keymaps[i][row][column][0]);
          if (bitmask) {
            key->layers[i].type = KEY_TYPE_MODIFIER;
            key->layers[i].value[0] = bitmask;
          } else {
            key->layers[i].type = KEY_TYPE_NORMAL;
            key->layers[i].value[0] =
                keyboard_user_config.keymaps[i][row][column][0];
          }
        }
      }
    }
  }
}

uint8_t update_key_state(struct key *key) {
  struct state state;

  // Get a reading
  state.value = keyboard_user_config.reverse_magnet_pole ? 4095 - keyboard_read_adc() : keyboard_read_adc();

  if (key->calibration.cycles_count < CALIBRATION_CYCLES) {
    // Calibrate idle value
    float delta = 0.6;
    key->calibration.idle_value = (1 - delta) * state.value + delta * key->calibration.idle_value;
    key->calibration.cycles_count++;

    return 0;
  }

  // Calibrate idle value
  if (state.value > key->calibration.idle_value) {
    // opti possible sur float
    float delta = 0.8;
    key->calibration.idle_value = (1 - delta) * state.value + delta * key->calibration.idle_value;
    state.value = key->calibration.idle_value;
  }

  // Do nothing if key is idle
  if (key->state.distance == 0 && state.value >= key->calibration.idle_value - IDLE_VALUE_OFFSET) {
    if (key->idle_counter >= IDLE_CYCLES_UNTIL_SLEEP) {
      key->is_idle = 1;
      return 0;
    }
    key->idle_counter++;
  }

  // Get distance from top
  if (state.value >= key->calibration.idle_value - IDLE_VALUE_OFFSET) {
    state.distance = 0;
    key->actuation.direction_changed_point = 0;
  } else {
    state.distance = key->calibration.idle_value - IDLE_VALUE_OFFSET - state.value;
    key->is_idle = 0;
    key->idle_counter = 0;
  }

  // Calibrate max distance value
  if (state.distance > key->calibration.max_distance) {
    key->calibration.max_distance = state.distance;
  }

  // Limit max distance
  if (state.distance >= key->calibration.max_distance - MAX_DISTANCE_OFFSET) {
    state.distance = key->calibration.max_distance;
  }

  // Map distance in percentages
  state.distance_8bits = (state.distance * 0xff) / key->calibration.max_distance;

  float delta = 0.8;
  state.filtered_distance = (1 - delta) * state.distance_8bits + delta * key->state.filtered_distance;
  state.filtered_distance_8bits = state.filtered_distance;

  // Update velocity
  state.velocity = state.filtered_distance_8bits - key->state.filtered_distance_8bits;

  // Update direction
  if (key->state.velocity > 0 && state.velocity > 0 && key->actuation.direction != GOING_DOWN) {
    key->actuation.direction = GOING_DOWN;
    if (key->actuation.direction_changed_point != 0) {
      key->actuation.direction_changed_point = key->state.filtered_distance_8bits;
    }
  } else if (key->state.velocity < 0 && state.velocity < 0 && key->actuation.direction != GOING_UP) {
    key->actuation.direction = GOING_UP;
    if (key->actuation.direction_changed_point != 255) {
      key->actuation.direction_changed_point = key->state.filtered_distance_8bits;
    }
  }

  key->state = state;
  return 1;
}

void update_key_actuation(struct key *key, uint8_t layer) {
  /**
   * https://www.youtube.com/watch?v=_Sl-T6iQr8U&t
   *
   *                          -----   |--------|                           -
   *                            |     |        |                           |
   *    is_before_reset_offset  |     |        |                           |
   *                            |     |        |                           | Continuous rapid trigger domain (deactivated when full_reset)
   *                          -----   | ------ | <- reset_offset           |
   *                            |     |        |                           |
   *                          -----   | ------ | <- trigger_offset         -
   *                            |     |        |                           |
   *                            |     |        |                           |
   *   is_after_trigger_offset  |     |        |                           | Rapid trigger domain
   *                            |     |        |                           |
   *                            |     |        |                           |
   *                          -----   |--------|                           -
   *
   */

  // if rapid trigger enable, move trigger and reset offsets according to the distance taht began the trigger

  uint32_t now = keyboard_get_time();
  uint8_t is_after_trigger_offset = key->state.distance_8bits > key->actuation.trigger_offset;
  uint8_t is_before_reset_offset = key->state.distance_8bits < key->actuation.reset_offset;
  uint8_t has_rapid_trigger = key->actuation.rapid_trigger_offset != 0;
  uint8_t is_after_rapid_trigger_offset = key->state.distance_8bits > key->actuation.direction_changed_point - key->actuation.rapid_trigger_offset + keyboard_user_config.reset_threshold;
  uint8_t is_before_rapid_reset_offset = key->state.distance_8bits < key->actuation.direction_changed_point - key->actuation.rapid_trigger_offset;

  switch (key->actuation.status) {

  case STATUS_RESET:
    // if reset, can be triggered or tap
    if (is_after_trigger_offset) {
      if (key->layers[_TAP_LAYER].value[0]) {
        key->actuation.status = STATUS_MIGHT_BE_TAP;
        // key_triggered = 1;
      } else {
        key->actuation.status = STATUS_TRIGGERED;
        key_triggered = 1;
        hid_press_key(key, layer);
      }
      key->actuation.triggered_at = now;
    }
    break;

  case STATUS_RAPID_TRIGGER_RESET:
    if (!has_rapid_trigger) {
      key->actuation.status = STATUS_RESET;
      break;
    }
    // if reset, can be triggered or tap
    if (is_after_trigger_offset && key->actuation.direction == GOING_DOWN && is_after_rapid_trigger_offset) {
      if (key->layers[_TAP_LAYER].value[0]) {
        key->actuation.status = STATUS_MIGHT_BE_TAP;
        key_triggered = 1;
      } else {
        key->actuation.status = STATUS_TRIGGERED;
        key_triggered = 1;
        hid_press_key(key, layer);
      }
      key->actuation.triggered_at = now;
    } else if (is_before_reset_offset) {
      key->actuation.status = STATUS_RESET;
    }
    break;

  case STATUS_TAP:
    // if tap, can be reset
    key->actuation.status = STATUS_RESET;
    hid_release_key(key, _TAP_LAYER);
    break;

  case STATUS_TRIGGERED:
    // if triggered, can be reset
    if (is_before_reset_offset) {
      key->actuation.status = STATUS_RESET;
      hid_release_key(key, layer);
    } else if (has_rapid_trigger && key->actuation.direction == GOING_UP && is_before_rapid_reset_offset) {
      key->actuation.status = STATUS_RAPID_TRIGGER_RESET;
      hid_release_key(key, layer);
    }
    break;

  default:
    break;
  }
}

void update_key(struct key *key) {
  if (!update_key_state(key)) {
    return;
  }

  extern int current_layer;
  update_key_actuation(key, current_layer);
}

void keyboard_init_keys() {
  	keyboard_read_config();
  for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
      if (channels_by_row_col[row][col][0] != XXXX) {
        init_key(channels_by_row_col[row][col][0], channels_by_row_col[row][col][1], row, col);
      }
    }
  }
}

void keyboard_task() {
  uint32_t started_at = keyboard_get_time();
  key_triggered = 0;

  for (uint8_t amux_channel = 0; amux_channel < AMUX_CHANNEL_COUNT; amux_channel++) {
    keyboard_select_amux(amux_channel);

    for (uint8_t adc_channel = 0; adc_channel < ADC_CHANNEL_COUNT; adc_channel++) {
      if (keyboard_keys[adc_channel][amux_channel].is_enabled == 0) {
        continue;
      }
      keyboard_select_adc(adc_channel);

      update_key(&keyboard_keys[adc_channel][amux_channel]);

      keyboard_close_adc();
    }
  }

  // If a key might be tap and a non tap key has been triggered, then the might be tap key is a normal trigger
  for (uint8_t amux_channel = 0; amux_channel < AMUX_CHANNEL_COUNT; amux_channel++) {
    for (uint8_t adc_channel = 0; adc_channel < ADC_CHANNEL_COUNT; adc_channel++) {
      if (keyboard_keys[adc_channel][amux_channel].is_enabled == 0 || keyboard_keys[adc_channel][amux_channel].actuation.status != STATUS_MIGHT_BE_TAP) {
        continue;
      }

      struct key *key = &keyboard_keys[adc_channel][amux_channel];
      uint8_t is_before_reset_offset = key->state.distance_8bits < key->actuation.reset_offset;
      uint8_t is_before_timeout = keyboard_get_time() - key->actuation.triggered_at <= keyboard_user_config.tap_timeout;

      // if might be tap, can be tap or triggered
      if (is_before_reset_offset && is_before_timeout) {
        key->actuation.status = STATUS_TAP;
        hid_press_key(key, _TAP_LAYER);
      } else if (!is_before_timeout || key_triggered) {
        key->actuation.status = STATUS_TRIGGERED;
        hid_press_key(key, _BASE_LAYER);
      }
    }
  }

  keyboard_last_cycle_duration = keyboard_get_time() - started_at;
}

void snaptap_task() {
  uint32_t started_at = keyboard_get_time();
  key_triggered = 0;

  for (uint8_t amux_channel = 0; amux_channel < AMUX_CHANNEL_COUNT; amux_channel++) {
    keyboard_select_amux(amux_channel);

    for (uint8_t adc_channel = 0; adc_channel < ADC_CHANNEL_COUNT; adc_channel++) {
      if (keyboard_keys[adc_channel][amux_channel].is_enabled == 0) {
        continue;
      }
      keyboard_select_adc(adc_channel);

      update_key(&keyboard_keys[adc_channel][amux_channel]);

      keyboard_close_adc();
    }
  }

  // If a key might be tap and a non tap key has been triggered, then the might be tap key is a normal trigger
  for (uint8_t amux_channel = 0; amux_channel < AMUX_CHANNEL_COUNT; amux_channel++) {
    for (uint8_t adc_channel = 0; adc_channel < ADC_CHANNEL_COUNT; adc_channel++) {
      if (keyboard_keys[adc_channel][amux_channel].is_enabled == 0 || keyboard_keys[adc_channel][amux_channel].actuation.status != STATUS_MIGHT_BE_TAP) {
        continue;
      }

      struct key *key = &keyboard_keys[adc_channel][amux_channel];
      uint8_t is_before_reset_offset = key->state.distance_8bits < key->actuation.reset_offset;
      uint8_t is_before_timeout = keyboard_get_time() - key->actuation.triggered_at <= keyboard_user_config.tap_timeout;

      // if might be tap, can be tap or triggered
      if (is_before_reset_offset && is_before_timeout) {
        key->actuation.status = STATUS_TAP;
        hid_press_key(key, _TAP_LAYER);
      } else if (!is_before_timeout || key_triggered) {
        key->actuation.status = STATUS_TRIGGERED;
        hid_press_key(key, _BASE_LAYER);
      }
    }
  }

  keyboard_last_cycle_duration = keyboard_get_time() - started_at;

  // Snaptap logic - chỉ xử lý việc thả phím cũ, không nhấn phím mới
  static struct key* current_pressed_key = NULL;
  uint32_t latest_triggered_time = 0;
  static struct key* new_pressed_key = NULL;
  for (uint8_t amux_channel = 0; amux_channel < AMUX_CHANNEL_COUNT; amux_channel++) {
      for (uint8_t adc_channel = 0; adc_channel < ADC_CHANNEL_COUNT; adc_channel++) {
          struct key* key = &keyboard_keys[adc_channel][amux_channel];
          if (key->is_enabled && key->actuation.status == STATUS_TRIGGERED) {
              if (key->actuation.triggered_at >= latest_triggered_time) {
                  latest_triggered_time = key->actuation.triggered_at;
                  new_pressed_key = key;
              }
          }
      }
  }
  
  // Xử lý snaptap - chỉ thả phím cũ, không nhấn phím mới
  if (new_pressed_key != current_pressed_key) {
      if (current_pressed_key) {
          hid_release_key(current_pressed_key, _BASE_LAYER);
      }
      // KHÔNG gọi hid_press_key() ở đây vì phím đã được nhấn trong update_key_actuation()
      current_pressed_key = new_pressed_key;
  }
}

void check_snaptap_debug() {
  static struct key *last_pressed_key = NULL;
  struct key *current_pressed_key = NULL;

  // Tìm phím đang được giữ (trạng thái TRIGGERED)
  for (uint8_t amux_channel = 0; amux_channel < AMUX_CHANNEL_COUNT; amux_channel++) {
    for (uint8_t adc_channel = 0; adc_channel < ADC_CHANNEL_COUNT; adc_channel++) {
      struct key *key = &keyboard_keys[adc_channel][amux_channel];
      if (key->is_enabled && key->actuation.status == STATUS_TRIGGERED) {
        current_pressed_key = key;
        break;
      }
    }
    if (current_pressed_key)
      break;
  }

  // Nếu phím mới khác phím cũ, in log
  if (current_pressed_key != last_pressed_key) {
    if (last_pressed_key) {
      printf("Release key: row=%d, col=%d\n", last_pressed_key->row, last_pressed_key->column);
    }
    if (current_pressed_key) {
      printf("Press key: row=%d, col=%d\n", current_pressed_key->row, current_pressed_key->column);
    }
    last_pressed_key = current_pressed_key;
  }
}
