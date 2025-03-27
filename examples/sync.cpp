#include "Arduino.h"
#include "uart_pio_debug.h"
#include "rys8833.h"

RYS8833 gps;

void setup() {
  DEBUG_INIT();
  DEBUG_PRINTLN("__Init code__");

  gps.set_pinout(0, 1, 14);
  gps.set_port(&Serial1, 1000, 115200);
  gps.begin();
  gps.configure();
  gps.cold_init();
}

void loop() {
  bool t_sync = gps.try_get_dt_sync();
  if (t_sync) {
    DateTime gps_now = gps.get_datetime();
    DEBUG_PRINTLN(gps_now.timestamp());
    gps.idle_mode();
  }
}