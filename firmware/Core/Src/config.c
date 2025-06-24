#include "config.h"
#include "keyboard.h"
#include "stdlib.h"
#include "hid.h"
#include <class/hid/hid.h>

const struct user_config keyboard_default_user_config = {
    .reverse_magnet_pole = DEFAULT_REVERSE_MAGNET_POLE,
    .trigger_offset = DEFAULT_TRIGGER_OFFSET,
    .reset_threshold = DEFAULT_RESET_THRESHOLD,
    .rapid_trigger_offset = DEFAULT_RAPID_TRIGGER_OFFSET,
    .screaming_velocity_trigger = DEFAULT_SCREAMING_VELOCITY_TRIGGER,
    .tap_timeout = DEFAULT_TAP_TIMEOUT,
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

// {adc_channel, amux_channel}
const uint8_t channels_by_row_col[MATRIX_ROWS][MATRIX_COLS][2] = {
    {{0, 0}, {0, 1}, {0, 2}, {0, 3}},
    {{0, 4}, {0, 5}, {0, 6}, {0, 7}},
    {{0, 11}, {0, 10}, {0, 9}, {0, 8}},
    {{0, 15}, {0, 14}, {0, 13}, {0, 12}},
};
