#include "comm.h"

static void in_recv_handler(DictionaryIterator *iter, void *context) {
  Tuple *t = dict_read_first(iter);
  while(t) {
    persist_write_bool(t->key, strcmp(t->value->cstring, "true") == 0 ? true : false);
    t = dict_read_next(iter);
  }

  // Refresh live store
  config_init();
  vibes_short_pulse();

  window_stack_pop_all(true);
}

void comm_init() {
  app_message_register_inbox_received(in_recv_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}