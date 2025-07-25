#include "keyboard.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include <cdc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct user_config keyboard_user_config;
extern struct key keyboard_keys[ADC_CHANNEL_COUNT][AMUX_CHANNEL_COUNT];
// Command buffer for parsing
static char cmd_buffer[CFG_TUD_CDC_RX_BUFSIZE];
static uint8_t cmd_index = 0;
extern uint32_t started_at;

// Map ASCII codes to special character enum
static special_char_t get_special_char(char c) {
  switch (c) {
  case 3:
    return SPECIAL_CTRL_C; // Ctrl+C
  case 4:
    return SPECIAL_CTRL_D; // Ctrl+D
  case '\b':
  case 127:
    return SPECIAL_BACKSPACE; // Backspace
  case '\r':
  case '\n':
    return SPECIAL_ENTER; // Enter
  default:
    return SPECIAL_NONE;
  }
}

// Handle special character actions
static void handle_special_char(special_char_t sc) {
  switch (sc) {
  case SPECIAL_BACKSPACE:
    if (cmd_index > 0) {
      cmd_index--;
      // Send backspace sequence: move cursor back, write space to erase, move cursor back again
      // This works properly with CR/LF terminals
      char backspace_seq[] = {'\b', ' ', '\b', '\0'};
      cdc_write_string_chunked(backspace_seq);
      cdc_write_flush_wait();
    }
    break;
  case SPECIAL_CTRL_C:
    // if (streaming_active) {
    //   stop_streaming();
    //   cdc_write_string_chunked("\r\nStreaming stopped\r\n");
    //   cdc_write_string_chunked("Ready> ");
    // }
    break;
  case SPECIAL_CTRL_D:
    cdc_write_string_chunked("\r\nTerminal exit (Ctrl+D)\r\n");
    // Optionally reset state or disconnect
    cdc_write_flush_wait();
    tud_disconnect(); // Disconnect the USB CDC device from the host
    cmd_index = 0;
    break;
  case SPECIAL_ENTER:
    cdc_write_string_chunked("\r\n");
    cmd_buffer[cmd_index] = '\0';
    if (cmd_index > 0) {
      process_command(cmd_buffer);
      cmd_index = 0;
    }
    cdc_write_string_chunked("Ready> ");
    break;
  default:
    break;
  }
}
void cdc_performance_measure(uint32_t started_at) {
  if (!tud_cdc_connected())
    return;

  uint32_t now = HAL_GetTick();
  uint32_t difference = now - started_at;

  char msg[64];
  int len = snprintf(msg, sizeof(msg), "One cycle duration: %lu\r\n", difference);

  tud_cdc_write(msg, len);
  tud_cdc_write_flush(); // Make sure data is sent
}

void cdc_task(void) {
  if (tud_cdc_connected()) {
    if (tud_cdc_available()) {
      uint8_t buf[CFG_TUD_CDC_RX_BUFSIZE];
      uint32_t count = tud_cdc_read(buf, sizeof(buf));
      for (uint32_t i = 0; i < count; i++) {
        char c = buf[i];
        special_char_t sc = get_special_char(c);

        if (sc != SPECIAL_NONE) {
          handle_special_char(sc);
        } else if (c >= 32 && c <= 126 && cmd_index < sizeof(cmd_buffer) - 1) {
          cmd_buffer[cmd_index++] = c;
          tud_cdc_write(&c, 1); // Echo character
          tud_cdc_write_flush();
        }
      }
    }
  } else {
    cmd_index = 0;
  }
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
  (void)itf;

  // Check if terminal is connecting (DTR asserted)
  if (dtr) {
    // Give a small delay to ensure connection is stable
    for (volatile int i = 0; i < 10000; i++)
      ;

    cdc_write_string_chunked("\r\n=== HE16 Configuration Interface ===\r\n");
    cdc_write_string_chunked("Type 'help' for available commands\r\n");
    cdc_write_string_chunked("Ready> ");
    cdc_write_flush_wait();
  }
}

// Write string in chunks to avoid buffer overflow
static void cdc_write_string_chunked(const char *str) {
  if (!str)
    return;

  size_t len = strlen(str);
  size_t sent = 0;
  const size_t chunk_size = CFG_TUD_CDC_EP_BUFSIZE - 8; // Leave some margin

  while (sent < len) {
    size_t to_send = (len - sent > chunk_size) ? chunk_size : (len - sent);

    // Wait for space in buffer
    while (tud_cdc_write_available() < to_send) {
      tud_task(); // Process USB tasks
    }

    tud_cdc_write(str + sent, to_send);
    sent += to_send;

    // Flush if buffer is getting full or we're done
    if (tud_cdc_write_available() < chunk_size || sent >= len) {
      cdc_write_flush_wait();
    }
  }
}

// Wait for flush to complete
static void cdc_write_flush_wait(void) {
  tud_cdc_write_flush();

  // Wait for data to be sent
  uint32_t timeout = 0;
  while (tud_cdc_write_available() < CFG_TUD_CDC_TX_BUFSIZE && timeout < 10000) {
    tud_task();
    timeout++;
  }
}

static void process_command(char *cmd) {
  // Convert to lowercase for case-insensitive commands
  for (int i = 0; cmd[i]; i++) {
    if (cmd[i] >= 'A' && cmd[i] <= 'Z') {
      cmd[i] += 32;
    }
  }

  // Get command name
  char *token = strtok(cmd, " ");
  if (!token)
    return;

  char *args = strtok(NULL, ""); // Get rest of the string as arguments

  // Iterate over command table
  for (size_t i = 0; i < sizeof(command_table) / sizeof(command_table[0]); i++) {
    if (strcmp(token, command_table[i].name) == 0) {
      command_table[i].handler(args);
      return;
    }
  }

  cdc_write_string_chunked("Unknown command. Type 'help' for available commands\r\n");
}

static void cmd_help(char *args) {
  (void)args;
  cdc_write_string_chunked("Available commands:\r\n");
  for (size_t i = 0; i < sizeof(command_table) / sizeof(command_table[0]); i++) {
    cdc_write_string_chunked(command_table[i].name);
    cdc_write_string_chunked(" ");
    cdc_write_string_chunked(command_table[i].usage);
    cdc_write_string_chunked("\r\n");
  }
}

static void cmd_show(char *args) {
  (void)args;
  cdc_write_string_chunked("Current configuration:\r\n");
  char buf[64]; // Buffer for formatted strings
  snprintf(buf, sizeof(buf), "Reverse Magnet Pole: %d\r\n", keyboard_user_config.reverse_magnet_pole);
  cdc_write_string_chunked(buf);
  snprintf(buf, sizeof(buf), "Trigger Offset: %d\r\n", keyboard_user_config.trigger_offset);
  cdc_write_string_chunked(buf);
  snprintf(buf, sizeof(buf), "Reset Threshold: %d\r\n", keyboard_user_config.reset_threshold);
  cdc_write_string_chunked(buf);
  snprintf(buf, sizeof(buf), "Rapid Trigger Offset: %d\r\n", keyboard_user_config.rapid_trigger_offset);
  cdc_write_string_chunked(buf);
  snprintf(buf, sizeof(buf), "Tap Timeout: %d ms\r\n", keyboard_user_config.tap_timeout);
  cdc_write_string_chunked(buf);

  cdc_write_flush_wait();
  cdc_write_string_chunked("Keymaps:\r\n");
  for (uint8_t layer = 0; layer < LAYERS_COUNT; layer++) {
    char layer_buf[16];
    snprintf(layer_buf, sizeof(layer_buf), "Layer %d:\r\n", layer);
    cdc_write_string_chunked(layer_buf);
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
      for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        uint16_t value = keyboard_user_config.keymaps[layer][row][col][0];
        char key_info[64];
        if (value >= 0x04 && value <= 0x1D) { // HID keycodes for 'a'-'z'
          char ascii = 'a' + (value - 0x04);
          snprintf(key_info, sizeof(key_info), "Row %d, Col %d: 0x%02X ('%c')\r\n", row, col, value, ascii);
        } else {
          snprintf(key_info, sizeof(key_info), "Row %d, Col %d: 0x%02X\r\n", row, col, value);
        }
        cdc_write_string_chunked(key_info);
      }
    }
  }
}

static void cmd_set(char *args) {
  if (!args) {
    cdc_write_string_chunked("Usage: set <parameter> <value>\r\n");
    return;
  }
  char *param = strtok(args, " ");
  char *value = strtok(NULL, " ");
  if (param && value) {
    if (strcmp(param, "reverse_magnet_pole") == 0) {
      keyboard_user_config.reverse_magnet_pole = atoi(value);
      cdc_write_string_chunked("Reverse Magnet Pole set to ");
      cdc_write_string_chunked(value);
    } else if (strcmp(param, "trigger_offset") == 0) {
      keyboard_user_config.trigger_offset = atoi(value);
      cdc_write_string_chunked("Trigger Offset set to ");
      cdc_write_string_chunked(value);
      cdc_write_string_chunked("\r\n");
    } else if (strcmp(param, "reset_threshold") == 0) {
      keyboard_user_config.reset_threshold = atoi(value);
      cdc_write_string_chunked("Reset Threshold set to ");
      cdc_write_string_chunked(value);
      cdc_write_string_chunked("\r\n");
    } else if (strcmp(param, "rapid_trigger_offset") == 0) {
      keyboard_user_config.rapid_trigger_offset = atoi(value);
      cdc_write_string_chunked("Rapid Trigger Offset set to ");
      cdc_write_string_chunked(value);
      cdc_write_string_chunked("\r\n");
    } else if (strcmp(param, "tap_timeout") == 0) {
      keyboard_user_config.tap_timeout = atoi(value);
      cdc_write_string_chunked("Tap Timeout set to ");
      cdc_write_string_chunked(value);
      cdc_write_string_chunked("\r\n");
    } else {
      cdc_write_string_chunked("Unknown parameter\r\n");
    }
    cmd_save(NULL); // Save changes to config

  } else {
    cdc_write_string_chunked("Usage: set <parameter> <value>\r\n");
  }
}

static void cmd_save(char *args) {
  (void)args;

  keyboard_write_config(&keyboard_user_config, 0, sizeof keyboard_user_config);
  keyboard_init_keys();
  cdc_write_string_chunked("Configuration saved to flash\r\n");
}

static void cmd_load(char *args) {
  (void)args;
  keyboard_init_keys();
  cdc_write_string_chunked("Configuration loaded from flash\r\n");
}

static void cmd_reset(char *args) {
  (void)args;
  // Reset to default values
  keyboard_write_config(&keyboard_default_user_config, 0, sizeof keyboard_default_user_config);
  keyboard_init_keys();
  cdc_write_string_chunked("Configuration reset to defaults\r\n");
}
static void cmd_setkey(char *args) {
  if (!args) {
    cdc_write_string_chunked("Usage: setkey <layer> <row> <col> <parameter> <value>\r\n");
    return;
  }
  char *layer_str = strtok(args, " ");
  char *adc_str = strtok(NULL, " ");
  char *amux_str = strtok(NULL, " ");
  char *param = strtok(NULL, " ");
  char *value = strtok(NULL, " ");

  if (layer_str && adc_str && amux_str && param && value) {
    uint8_t layer = atoi(layer_str);
    uint8_t adc = atoi(adc_str);
    uint8_t amux = atoi(amux_str);
    uint16_t val = (uint16_t)atoi(value);

    if (layer < LAYERS_COUNT && adc < ADC_CHANNEL_COUNT && amux < AMUX_CHANNEL_COUNT) {
      if (strcmp(param, "keymap") == 0) {
        keyboard_keys[adc][amux].layers[layer].value[0] = val;
        cdc_write_string_chunked("Keymap updated\r\n");
      } else {
        if (strcmp(param, "is_enabled") == 0) {
          keyboard_keys[adc][amux].is_enabled = val ? 1 : 0;
          cdc_write_string_chunked("Key enabled state updated\r\n");
        } else if (strcmp(param, "trigger_offset") == 0) {
          keyboard_keys[adc][amux].actuation.trigger_offset = val;
          cdc_write_string_chunked("Trigger offset updated\r\n");
        } else if (strcmp(param, "reset_offset") == 0) {
          keyboard_keys[adc][amux].actuation.reset_offset = val;
          cdc_write_string_chunked("Reset offset updated\r\n");
        } else if (strcmp(param, "rapid_trigger_offset") == 0) {
          keyboard_keys[adc][amux].actuation.rapid_trigger_offset = val;
          cdc_write_string_chunked("Rapid trigger offset updated\r\n");
        } else {
          cdc_write_string_chunked("Unknown parameter\r\n");
        }
      }
      // cmd_save(NULL); // Save changes to config
    } else {
      cdc_write_string_chunked("Invalid layer/row/col\r\n");
    }
  } else {
    cdc_write_string_chunked("Usage: setkey <layer> <row> <col> <parameter> <value>\r\n");
  }
}

static void cmd_showkey(char *args) {
  if (!args) {
    cdc_write_string_chunked("Usage: showkey <layer> <adc> <amux>\r\n");
    return;
  }
  char *layer_str = strtok(args, " ");
  char *adc_str = strtok(NULL, " ");
  char *amux_str = strtok(NULL, " ");

  if (layer_str && adc_str && amux_str) {
    uint8_t layer = atoi(layer_str);
    uint8_t adc = atoi(adc_str);
    uint8_t amux = atoi(amux_str);

    if (layer < LAYERS_COUNT && adc < ADC_CHANNEL_COUNT && amux < AMUX_CHANNEL_COUNT) {
      char buf[128];
      snprintf(buf, sizeof(buf), "Available parameter for key of layer %d, adc %d, amux %d:\r\n", layer, adc, amux);
      cdc_write_string_chunked(buf);
      if (keyboard_keys[adc][amux].layers[layer].type == KEY_TYPE_EMPTY) {
        cdc_write_string_chunked("This key is empty\r\n");
        return;
      } else if (keyboard_keys[adc][amux].layers[layer].type == KEY_TYPE_NORMAL) {
        snprintf(buf, sizeof(buf), "Keymap: 0x%04X\r\n", keyboard_keys[adc][amux].layers[layer].value[0]);
        cdc_write_string_chunked(buf);
      } else if (keyboard_keys[adc][amux].layers[layer].type == KEY_TYPE_MODIFIER) {
        snprintf(buf, sizeof(buf), "Modifier Keymap: 0x%04X\r\n", keyboard_keys[adc][amux].layers[layer].value[0]);
        cdc_write_string_chunked(buf);
      } else if (keyboard_keys[adc][amux].layers[layer].type == KEY_TYPE_CONSUMER_CONTROL) {
        snprintf(buf, sizeof(buf), "Consumer Control Keymap: 0x%04X\r\n", keyboard_keys[adc][amux].layers[layer].value[0]);
        cdc_write_string_chunked(buf);
      } else if (keyboard_keys[adc][amux].layers[layer].type == KEY_TYPE_MACRO) {
        cdc_write_string_chunked("Macro Keymap: ");
        for (int i = 0; i < MAX_MACRO_LEN; i++) {
          snprintf(buf, sizeof(buf), "0x%04X ", keyboard_keys[adc][amux].layers[layer].value[i]);
          cdc_write_string_chunked(buf);
        }
        cdc_write_string_chunked("\r\n");
      }

      cdc_write_string_chunked(buf);
      snprintf(buf, sizeof(buf), "Enabled: %d\r\n", keyboard_keys[adc][amux].is_enabled);
      cdc_write_string_chunked(buf);
      snprintf(buf, sizeof(buf), "Trigger Offset: %d\r\n", keyboard_keys[adc][amux].actuation.trigger_offset);
      cdc_write_string_chunked(buf);
      snprintf(buf, sizeof(buf), "Reset Offset: %d\r\n", keyboard_keys[adc][amux].actuation.reset_offset);
      cdc_write_string_chunked(buf);
      snprintf(buf, sizeof(buf), "Rapid Trigger Offset: %d\r\n", keyboard_keys[adc][amux].actuation.rapid_trigger_offset);
      cdc_write_string_chunked(buf);
      cdc_write_string_chunked("You can set these parameters using 'setkey' command\r\n");
      cdc_write_flush_wait();
    } else {
      cdc_write_string_chunked("Invalid layer/row/col\r\n");
    }
  } else {
    cdc_write_string_chunked("Usage: showkey <layer> <adc> <amux>\r\n");
  }
}