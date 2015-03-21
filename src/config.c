#include "config.h"

static bool s_arr[NUM_SETTINGS];

void config_init() {
  // Set defaults
  if(!persist_exists(PERSIST_DEFAULTS_SET)) {
    persist_write_bool(PERSIST_DEFAULTS_SET, true);

    persist_write_bool(PERSIST_KEY_DATE, true);
    persist_write_bool(PERSIST_KEY_DAY, true);
    persist_write_bool(PERSIST_KEY_BT, true);
    persist_write_bool(PERSIST_KEY_BATTERY, true);
    persist_write_bool(PERSIST_KEY_SECOND_HAND, true);
  }

  for(int i = 0; i < NUM_SETTINGS; i++) {
    s_arr[i] = persist_read_bool(i);
  }
}

bool config_get(int key) {
  return s_arr[key];
}