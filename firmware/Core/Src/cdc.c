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

// Streaming control variables
static bool streaming_active = false;
static uint32_t last_stream_time = 0;
static const uint32_t STREAM_INTERVAL_MS = 1; // 1ms for ~1kHz

// Function prototypes
void cdc_performance_measure(uint32_t started_at);
static void process_command(char *cmd);
static void print_help(void);
static void print_config(void);
static void set_config_value(char *param, char *value);
static void print_keymap(uint8_t layer);
static void set_keymap_value(uint8_t layer, uint8_t row, uint8_t col, uint16_t value);
static void set_macro_keymap_value(uint8_t layer, uint8_t row, uint8_t col, uint16_t values[MAX_MACRO_LEN]);
static void save_config(void);
static void load_config(void);
static void reset_config(void);
static void cdc_write_string_chunked(const char *str);
static void cdc_write_flush_wait(void);
static void start_streaming(void);
static void stop_streaming(void);
static void handle_streaming(void);

extern uint32_t started_at; // Define this somewhere in your code

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
    // Handle streaming if active
    handle_streaming();

    if (tud_cdc_available()) {
      uint8_t buf[CFG_TUD_CDC_RX_BUFSIZE];
      uint32_t count = tud_cdc_read(buf, sizeof(buf));

      for (uint32_t i = 0; i < count; i++) {
        char c = buf[i];

        // Handle Ctrl+C to stop streaming
        if (c == 3) { // Ctrl+C ASCII code
          if (streaming_active) {
            stop_streaming();
            cdc_write_string_chunked("\r\nStreaming stopped\r\n");
            cdc_write_string_chunked("Ready> ");
          }
          continue;
        }

        // Handle backspace
        if (c == '\b' || c == 127) {
          if (cmd_index > 0 && !streaming_active) {
            cmd_index--;
            cdc_write_string_chunked("\b \b"); // Erase character
          }
        }
        // Handle enter/newline
        else if (c == '\r' || c == '\n') {
          if (!streaming_active) {
            cdc_write_string_chunked("\r\n");
            cmd_buffer[cmd_index] = '\0';

            if (cmd_index > 0) {
              process_command(cmd_buffer);
              cmd_index = 0;
            }

            if (!streaming_active) {
              cdc_write_string_chunked("Ready> ");
            }
          }
        }
        // Handle printable characters
        else if (c >= 32 && c <= 126 && cmd_index < sizeof(cmd_buffer) - 1 && !streaming_active) {
          cmd_buffer[cmd_index++] = c;
          tud_cdc_write(&c, 1); // Echo character
        }
      }

      if (!streaming_active) {
        cdc_write_flush_wait();
      }
    }
  } else {
    // Reset flags when disconnected
    cmd_index = 0;
    streaming_active = false;
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
    tud_task(); // Process USB tasks
    timeout++;
  }
}

static void start_streaming(void) {
  streaming_active = true;
  last_stream_time = HAL_GetTick();
  cdc_write_string_chunked("Starting ADC stream (Press Ctrl+C to stop)...\r\n");
}

static void stop_streaming(void) {
  streaming_active = false;
}

static void handle_streaming(void) {
  if (!streaming_active || !tud_cdc_connected()) {
    return;
  }

  uint32_t current_time = HAL_GetTick();

  // Check if it's time to send data (1ms interval for ~1kHz)
  if (current_time - last_stream_time >= STREAM_INTERVAL_MS) {
    last_stream_time = current_time;

    // Build CSV string with all keyboard_keys state.value data
    char csv_buffer[512];
    int pos = 0;

    // Iterate through the keyboard_keys array and collect state.value for each key
    bool first = true;
    for (uint8_t adc_ch = 0; adc_ch < ADC_CHANNEL_COUNT; adc_ch++) {
      for (uint8_t amux_ch = 0; amux_ch < AMUX_CHANNEL_COUNT; amux_ch++) {
        if (!first) {
          pos += snprintf(csv_buffer + pos, sizeof(csv_buffer) - pos, ",");
        }
        first = false;

        // Get the state.value from the current key
        uint16_t value = keyboard_keys[adc_ch][amux_ch].state.value;
        pos += snprintf(csv_buffer + pos, sizeof(csv_buffer) - pos, "%u", value);

        // Safety check to prevent buffer overflow
        if (pos >= sizeof(csv_buffer) - 10) {
          break;
        }
      }
      if (pos >= sizeof(csv_buffer) - 10) {
        break;
      }
    }

    // Add newline
    pos += snprintf(csv_buffer + pos, sizeof(csv_buffer) - pos, "\r\n");

    // Send the CSV data
    if (tud_cdc_write_available() >= pos) {
      tud_cdc_write(csv_buffer, pos);
      tud_cdc_write_flush();
    }
  }
}

static void process_command(char *cmd) {
  // Convert to lowercase for case-insensitive commands
  for (int i = 0; cmd[i]; i++) {
    if (cmd[i] >= 'A' && cmd[i] <= 'Z') {
      cmd[i] += 32;
    }
  }

  char *token = strtok(cmd, " ");
  if (!token)
    return;

  if (strcmp(token, "help") == 0) {
    print_help();
  } else if (strcmp(token, "show") == 0) {
    print_config();
  } else if (strcmp(token, "stream") == 0) {
    start_streaming();
  } else if (strcmp(token, "set") == 0) {
    char *param = strtok(NULL, " ");
    char *value = strtok(NULL, " ");
    if (param && value) {
      set_config_value(param, value);
    } else {
      cdc_write_string_chunked("Usage: set <parameter> <value>\r\n");
    }
  } else if (strcmp(token, "keymap") == 0) {
    char *layer_str = strtok(NULL, " ");
    if (layer_str) {
      uint8_t layer = atoi(layer_str);
      if (layer < LAYERS_COUNT) {
        print_keymap(layer);
      } else {
        cdc_write_string_chunked("Invalid layer number\r\n");
      }
    } else {
      cdc_write_string_chunked("Usage: keymap <layer>\r\n");
    }
  } else if (strcmp(token, "setkey") == 0) {
    char *layer_str = strtok(NULL, " ");
    char *row_str = strtok(NULL, " ");
    char *col_str = strtok(NULL, " ");
    char *value_str = strtok(NULL, " ");

    if (layer_str && row_str && col_str && value_str) {
      uint8_t layer = atoi(layer_str);
      uint8_t row = atoi(row_str);
      uint8_t col = atoi(col_str);
      uint16_t value = atoi(value_str);

      if (layer < LAYERS_COUNT && row < MATRIX_ROWS && col < MATRIX_COLS) {
        set_keymap_value(layer, row, col, value);
      } else {
        cdc_write_string_chunked("Invalid layer/row/col values\r\n");
      }
    } else {
      cdc_write_string_chunked("Usage: setkey <layer> <row> <col> <value>\r\n");
    }
  } else if (strcmp(token, "setmacro") == 0) {
    char *layer_str = strtok(NULL, " ");
    char *row_str = strtok(NULL, " ");
    char *col_str = strtok(NULL, " ");

    if (layer_str && row_str && col_str) {
      uint8_t layer = atoi(layer_str);
      uint8_t row = atoi(row_str);
      uint8_t col = atoi(col_str);

      if (layer < LAYERS_COUNT && row < MATRIX_ROWS && col < MATRIX_COLS) {
        uint16_t values[MAX_MACRO_LEN];
        uint8_t value_count = 0;

        // Parse up to MAX_MACRO_LEN values
        char *value_str = strtok(NULL, " ");
        while (value_str && value_count < MAX_MACRO_LEN) {
          values[value_count] = atoi(value_str);
          value_count++;
          value_str = strtok(NULL, " ");
        }

        // Fill remaining slots with ____ if not enough values provided
        while (value_count < MAX_MACRO_LEN) {
          values[value_count] = ____;
          value_count++;
        }

        set_macro_keymap_value(layer, row, col, values);
      } else {
        cdc_write_string_chunked("Invalid layer/row/col values\r\n");
      }
    } else {
      cdc_write_string_chunked("Usage: setmacro <layer> <row> <col> <value1> [value2] [value3] [value4]\r\n");
    }
  } else if (strcmp(token, "save") == 0) {
    save_config();
  } else if (strcmp(token, "load") == 0) {
    load_config();
  } else if (strcmp(token, "reset") == 0) {
    reset_config();
  } else {
    cdc_write_string_chunked("Unknown command. Type 'help' for available commands\r\n");
  }
}

static void print_help(void) {
  cdc_write_string_chunked("Available commands:\r\n");
  cdc_write_string_chunked("  help                    - Show this help\r\n");
  cdc_write_string_chunked("  show                    - Show current configuration\r\n");
  cdc_write_string_chunked("  stream                  - Start streaming ADC values (Ctrl+C to stop)\r\n");
  cdc_write_string_chunked("  set <param> <value>     - Set configuration parameter\r\n");
  cdc_write_string_chunked("  keymap <layer>          - Show keymap for layer\r\n");
  cdc_write_string_chunked("  setkey <L> <R> <C> <V>  - Set key value (Layer/Row/Col/Value)\r\n");
  cdc_write_string_chunked("  setmacro <L> <R> <C> <V1> [V2] [V3] [V4]  - Set macro key value (Layer/Row/Col/Value1 [Value2] [Value3] [Value4])\r\n");
  cdc_write_string_chunked("  save                    - Save configuration to flash\r\n");
  cdc_write_string_chunked("  load                    - Load configuration from flash\r\n");
  cdc_write_string_chunked("  reset                   - Reset to default values\r\n");
  cdc_write_string_chunked("\r\nParameters:\r\n");
  cdc_write_string_chunked("  reverse_magnet_pole, trigger_offset, reset_threshold,\r\n");
  cdc_write_string_chunked("  rapid_trigger_offset, tap_timeout\r\n");
}

static void print_config(void) {
  char buffer[128];

  cdc_write_string_chunked("Current Configuration:\r\n");

  snprintf(buffer, sizeof(buffer), "  reverse_magnet_pole: %u\r\n", keyboard_user_config.reverse_magnet_pole);
  cdc_write_string_chunked(buffer);

  snprintf(buffer, sizeof(buffer), "  trigger_offset: %u\r\n", keyboard_user_config.trigger_offset);
  cdc_write_string_chunked(buffer);

  snprintf(buffer, sizeof(buffer), "  reset_threshold: %u\r\n", keyboard_user_config.reset_threshold);
  cdc_write_string_chunked(buffer);

  snprintf(buffer, sizeof(buffer), "  rapid_trigger_offset: %u\r\n", keyboard_user_config.rapid_trigger_offset);
  cdc_write_string_chunked(buffer);

  snprintf(buffer, sizeof(buffer), "  tap_timeout: %u\r\n", keyboard_user_config.tap_timeout);
  cdc_write_string_chunked(buffer);

  cdc_write_string_chunked("Use 'keymap <layer>' to view keymaps\r\n");
}

static void set_config_value(char *param, char *value) {
  char buffer[64];
  uint32_t val = atoi(value);

  if (strcmp(param, "reverse_magnet_pole") == 0) {
    keyboard_user_config.reverse_magnet_pole = (uint8_t)val;
    snprintf(buffer, sizeof(buffer), "Set reverse_magnet_pole to %u\r\n", keyboard_user_config.reverse_magnet_pole);
  } else if (strcmp(param, "trigger_offset") == 0) {
    keyboard_user_config.trigger_offset = (uint8_t)val;
    snprintf(buffer, sizeof(buffer), "Set trigger_offset to %u\r\n", keyboard_user_config.trigger_offset);
  } else if (strcmp(param, "reset_threshold") == 0) {
    keyboard_user_config.reset_threshold = (uint8_t)val;
    snprintf(buffer, sizeof(buffer), "Set reset_threshold to %u\r\n", keyboard_user_config.reset_threshold);
  } else if (strcmp(param, "rapid_trigger_offset") == 0) {
    keyboard_user_config.rapid_trigger_offset = (uint8_t)val;
    snprintf(buffer, sizeof(buffer), "Set rapid_trigger_offset to %u\r\n", keyboard_user_config.rapid_trigger_offset);
  } else if (strcmp(param, "tap_timeout") == 0) {
    keyboard_user_config.tap_timeout = (uint16_t)val;
    snprintf(buffer, sizeof(buffer), "Set tap_timeout to %u\r\n", keyboard_user_config.tap_timeout);
  } else {
    snprintf(buffer, sizeof(buffer), "Unknown parameter: %s\r\n", param);
  }

  cdc_write_string_chunked(buffer);
}

static void print_keymap(uint8_t layer) {
  char buffer[128];

  snprintf(buffer, sizeof(buffer), "Keymap for Layer %u:\r\n", layer);
  cdc_write_string_chunked(buffer);

  // Print keymap row by row with proper chunking
  for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
    // Build the row string first
    char row_buffer[512]; // Larger buffer for macro display
    int pos = 0;

    pos += snprintf(row_buffer + pos, sizeof(row_buffer) - pos, "Row %u: ", row);

    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
      // Check if this is a macro (multiple non-zero values)
      uint8_t macro_count = 0;
      for (uint8_t i = 0; i < MAX_MACRO_LEN; i++) {
        if (keyboard_user_config.keymaps[layer][row][col][i] != ____) {
          macro_count++;
        }
      }

      if (macro_count > 1) {
        // This is a macro - show all values in brackets
        pos += snprintf(row_buffer + pos, sizeof(row_buffer) - pos, "[");
        for (uint8_t i = 0; i < MAX_MACRO_LEN; i++) {
          if (i > 0)
            pos += snprintf(row_buffer + pos, sizeof(row_buffer) - pos, ",");
          pos += snprintf(row_buffer + pos, sizeof(row_buffer) - pos, "%u",
                          keyboard_user_config.keymaps[layer][row][col][i]);
        }
        pos += snprintf(row_buffer + pos, sizeof(row_buffer) - pos, "] ");
      } else {
        // Single key - show just the first value
        pos += snprintf(row_buffer + pos, sizeof(row_buffer) - pos, "%4u ",
                        keyboard_user_config.keymaps[layer][row][col][0]);
      }
    }

    pos += snprintf(row_buffer + pos, sizeof(row_buffer) - pos, "\r\n");

    // Send the complete row
    cdc_write_string_chunked(row_buffer);
  }
}

static void set_keymap_value(uint8_t layer, uint8_t row, uint8_t col, uint16_t value) {
  char buffer[64];

  keyboard_user_config.keymaps[layer][row][col][0] = value;
  // Clear remaining macro slots
  for (uint8_t i = 1; i < MAX_MACRO_LEN; i++) {
    keyboard_user_config.keymaps[layer][row][col][i] = ____;
  }
  keyboard_write_config(&keyboard_user_config, 0, sizeof keyboard_user_config);
  keyboard_init_keys();

  snprintf(buffer, sizeof(buffer), "Set keymap[%u][%u][%u] to %u\r\n", layer, row, col, value);
  cdc_write_string_chunked(buffer);
}

static void set_macro_keymap_value(uint8_t layer, uint8_t row, uint8_t col, uint16_t values[MAX_MACRO_LEN]) {
  char buffer[128];

  // Copy all macro values
  for (uint8_t i = 0; i < MAX_MACRO_LEN; i++) {
    keyboard_user_config.keymaps[layer][row][col][i] = values[i];
  }
  keyboard_write_config(&keyboard_user_config, 0, sizeof keyboard_user_config);
  keyboard_init_keys();

  // Build response message showing all macro values
  int pos = snprintf(buffer, sizeof(buffer), "Set macro keymap[%u][%u][%u] to [", layer, row, col);
  for (uint8_t i = 0; i < MAX_MACRO_LEN; i++) {
    if (i > 0)
      pos += snprintf(buffer + pos, sizeof(buffer) - pos, ", ");
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%u", values[i]);
  }
  pos += snprintf(buffer + pos, sizeof(buffer) - pos, "]\r\n");
  cdc_write_string_chunked(buffer);
}

static void save_config(void) {
  // TODO: Implement flash save functionality
  // This would typically write the config struct to flash memory
  keyboard_write_config(&keyboard_user_config, 0, sizeof keyboard_user_config);
  keyboard_init_keys();
  cdc_write_string_chunked("Configuration saved to flash\r\n");
}

static void load_config(void) {
  // TODO: Implement flash load functionality
  // This would typically read the config struct from flash memory
  keyboard_read_config();
  cdc_write_string_chunked("Configuration loaded from flash\r\n");
}

static void reset_config(void) {
  // Reset to default values
  keyboard_write_config(&keyboard_default_user_config, 0, sizeof keyboard_default_user_config);
  keyboard_read_config();
  keyboard_init_keys();

  cdc_write_string_chunked("Configuration reset to defaults\r\n");
}

// Getter function for other modules to access configuration
struct user_config *get_user_config(void) {
  return &keyboard_user_config;
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

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf) {
  (void)itf;
  // Data handling is done in cdc_task()
}
