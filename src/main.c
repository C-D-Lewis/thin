#include "globals.h"
#include "main_window.h"
#include "comm.h"
#include "config.h"

int main() {
  setlocale(LC_ALL, "");
  comm_init();
  config_init();

  main_window_push();
  
  app_event_loop();
}
