#ifndef __CDC_H
#define __CDC_H
#include "keyboard.h"
typedef void (*command_handler_t)(char *args);
typedef struct {
  const char *name;
  command_handler_t handler;
  const char *usage; // Usage string for the command
} command_entry_t;

// Command mapping array

typedef enum {
  SPECIAL_NONE = 0,
  SPECIAL_BACKSPACE,
  SPECIAL_CTRL_C,
  SPECIAL_CTRL_D,
  SPECIAL_ENTER,
} special_char_t;

typedef enum {
  CMD_HELP,
  CMD_CONFIG,
  CMD_KEYMAP,
  CMD_MACRO,
  CMD_SAVE,
  CMD_LOAD,
  CMD_RESET,
  CMD_STREAM_START,
  CMD_STREAM_STOP,
} COMMAND;

static void cmd_help(char *args);
static void cmd_show(char *args);
static void cmd_showkey(char *args);
static void cmd_set(char *args);
static void cmd_setkey(char *args);
static void cmd_save(char *args);
static void cmd_load(char *args);
static void cmd_reset(char *args);
// static void cmd_setkeyparam(char *args);

static void cdc_write_string_chunked(const char *str);
static void cdc_write_flush_wait(void);
void cdc_performance_measure(uint32_t started_at);
static void process_command(char *cmd);
void cdc_task(void);
void cdc_performance_measure(uint32_t started_at);

static const command_entry_t command_table[] = {
    {"help", cmd_help, ""},
    {"show", cmd_show, ""},
    {"set", cmd_set, "<parameter> <value>"},
    // {"keymap", cmd_keymap, "<layer>"},
    {"setkey", cmd_setkey, "<layer> <adc> <amux> <parameter> <value>"},
    // {"setmacro", cmd_setmacro, "<layer> <row> <col> <value1> [value2] [value3] [value4]"},
    {"save", cmd_save, ""},
    {"load", cmd_load, ""},
    {"reset", cmd_reset, ""},
    // {"cycle", cmd_cycle, ""},
    // {"setkeyparam", cmd_setkeyparam, ""},
    {"showkey", cmd_showkey, "<layer> <adc> <amux>"},
};
#endif /* __CDC_H */
