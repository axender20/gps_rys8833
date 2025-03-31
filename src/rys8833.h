#pragma once
#ifndef _GPS_RYS8833_H_
#define _GPS_RYS8833_H_

#include "pico/stdlib.h"
#include "Arduino.h"
#include "freertos_definitions.h"
#include "date_time.h"

// #define _GPS_DEBUG_ENABLE_
#ifdef _GPS_DEBUG_ENABLE_ 
#include "uart_pio_debug.h"
#define GPS_DEBUG_INIT() DEBUG_INIT()
#define GPS_DEBUG_PRINT(_x) DEBUG_PRINT(_x)
#define GPS_DEBUG_PRINTLN(_x) DEBUG_PRINTLN(_x)
#else
#define GPS_DEBUG_INIT()
#define GPS_DEBUG_PRINT(_x)
#define GPS_DEBUG_PRINTLN(_x)
#endif

class RYS8833 {
private:
  //> Pinout config  
  uint8_t pin_tx;
  uint8_t pin_rx;
  uint8_t pin_rs;
  bool is_pinout_config;

  //> Port config
  SerialUART* gps_port;
  uint32_t gps_port_timeout;
  ulong gps_port_baudrate;
  bool is_port_config;
  bool is_cold_init;

  bool is_begun;
  void _delay_gps(uint32_t _time);
  void hardware_reset();

  String read_response();
  void send_command(const char* cmd);

  DateTime _gps_date_time;
  bool try_decode_zda_response(String _buff);
public:
  RYS8833();
  void set_pinout(uint8_t tx_d, uint8_t rx_d, uint8_t rs_d);
  void set_port(SerialUART* port_d, uint32_t timeout_d, ulong baudrate_d);

  bool test();

  bool begin();

  void configure();
  bool idle_mode();
  bool cold_init();

  bool try_get_dt_sync();
  DateTime get_datetime() { return this->_gps_date_time; }
};

#endif