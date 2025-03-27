#include "rys8833.h"

DateTime GPS_DT_Reference(2024, 1, 1, 0, 0, 0);

#define LGTH_CMDs 6
const char* GPS_CMDs[LGTH_CMDs] = {
  "@GSTP",          //? Idle mode
  "@ABUP 0",        //? Disable backup data
  "@GSOP 1 1000 0",  //? Normal mode config
  "@BSSL 0x80",     //? Use only ZDA NMEA
  "@GNS  0x01",     //? Use only GPS satellites
  "@GCD"            //? Cold init
};

const char* GPS_RSPs[LGTH_CMDs]{
"[GSTP] Done",
"[ABUP] Done",
"[GSOP] Done",
"[BSSL] Done",
"[GNS] Done",
"[GCD] Done"
};

enum e_GPS_CMDs : uint8_t {
  e_idle_mode = 0,
  e_cold_init = 5
};

void RYS8833::_delay_gps(uint32_t _time) {
  delay(_time);
}

void RYS8833::hardware_reset() {
  if (!is_pinout_config) return;

  const uint32_t time_nack = 1000u;
  pinMode(pin_rs, OUTPUT);
  digitalWrite(pin_rs, LOW);
  delay(time_nack);
  digitalWrite(pin_rs, HIGH);
  delay(time_nack);
}

String RYS8833::read_response() {
  String rsp = "";
  ulong ul_last_t = millis();
  while (!(millis() - ul_last_t > gps_port_timeout)) {
    if (gps_port->available()) {
      rsp = gps_port->readString();
      break;
    }
  }
  if (rsp != "") DEBUG_PRINTLN(rsp);
  return rsp;
}

void RYS8833::send_command(const char* cmd) {
  //? To send command is necesari add
  gps_port->println(cmd);
}

bool RYS8833::try_decode_zda_response(String _buff) {
  //!Format: $GNZDA,hhmmss.ss,xx,xx,xxxx,xx,xx*hh<CR><LF>
//*Example: $GNZDA,000255.00,06,01,1980,,*7D\r\n
  String _aux = _buff;
  int fI = _aux.indexOf("$");
  int sI = _aux.indexOf("\r\n");

  //!La cadena de caracteres no tiene el formato correcto
  if (fI == -1 || sI == -1) return false;
  _aux = _aux.substring(fI, sI);

  int tI = _aux.indexOf('*');
  if (tI == -1) return false;

  String _str_check_sum = _aux.substring(tI + 1);
  uint8_t _initial_check_sum = static_cast<uint8_t>(std::strtoul(_str_check_sum.c_str(), nullptr, 16));

  _aux = _aux.substring(fI + 1, tI);
  uint8_t check_sum = 0;
  //! First valide checkSum
  for (uint8_t i = 0; i < _aux.length(); i++) {
    check_sum ^= _aux.charAt(i);
  }

  if (check_sum != _initial_check_sum) return false;

  //? GNZDA,hhmmss.ss,xx,xx,xxxx,xx,xx

  uint8_t _hh = 0, _mm = 0, _ss = 0, _day = 0, _month = 0;
  uint16_t _year = 0;
  String str_split = "";

  uint8_t n_cont = 0;
  for (uint8_t i = 0; i < _aux.length(); i++) {
    if (_aux.charAt(i) == ',') {
      switch (n_cont) {
      case 1: {
        //? UTC
        double _time_double = atof(str_split.c_str());
        uint32_t _time_u32 = static_cast<int>(_time_double);
        _hh = _time_u32 / 10000;
        _mm = (_time_u32 / 100) % 100;
        _ss = _time_u32 % 100;
        break;
      }

      case 2: {
        //? Day
        _day = atoi(str_split.c_str());
        break;
      }

      case 3: {
        //? Month
        _month = atoi(str_split.c_str());
        break;
      }

      case 4: {
        //? Year
        _year = atoi(str_split.c_str());
        break;
      }

      default:
        break;
      }
      n_cont++;
      str_split = "";
    }
    else {
      str_split += _aux.charAt(i);
    }
  }

  if (_year >= GPS_DT_Reference.year()) {
    DateTime _dt(_year, _month, _day, _hh, _mm, _ss);
    DateTime _dt_horary_Central = _dt - TimeSpan(0, 6, 0, 0);
    _gps_date_time = _dt_horary_Central;
    return true;
  }
  return false;
}

RYS8833::RYS8833() : gps_port(nullptr) {
  is_pinout_config = false;
  is_port_config = false;
  is_begun = false;
  is_cold_init = false;
}

void RYS8833::set_pinout(uint8_t tx_d, uint8_t rx_d, uint8_t rs_d) {
  pin_tx = tx_d;
  pin_rx = rx_d;
  pin_rs = rs_d;

  is_pinout_config = true;
}

void RYS8833::set_port(SerialUART* port_d, uint32_t timeout_d, ulong baudrate_d) {
  gps_port = port_d;
  gps_port_timeout = timeout_d;
  gps_port_baudrate = baudrate_d;

  is_port_config = true;
}

bool RYS8833::test() {
  send_command("@VER");
  return (read_response().isEmpty()) ? false : true;
}

bool RYS8833::begin() {
  if (!is_pinout_config || !is_port_config) return false;

  hardware_reset();

  gps_port->setPinout(pin_tx, pin_rx);
  gps_port->setTimeout(100);
  gps_port->begin(gps_port_baudrate);

  DEBUG_PRINTLN("GPS Initializate");

  is_begun = test();

  return is_begun;
}

void RYS8833::configure() {
  if (!is_begun) return;
  for (uint8_t i = 0; i < LGTH_CMDs - 1; i++) {
    send_command(GPS_CMDs[i]);
    read_response();
    delay(200);
  }
}

bool RYS8833::idle_mode() {
  if (!is_begun) return false;

  //? Send "@GSTP"
  send_command(GPS_CMDs[e_idle_mode]);

  bool result = false;
  String rsp = read_response();
  if (rsp.indexOf(GPS_RSPs[e_idle_mode]) != -1) {
    result = true;
    is_cold_init = false;
  }
  return result;
}

bool RYS8833::cold_init() {
  if (!is_begun) return false;
  //? Send "@GCD" 
  send_command(GPS_CMDs[e_cold_init]);

  bool result = false;
  String rsp = read_response();
  if (rsp.indexOf(GPS_RSPs[e_cold_init]) != -1) {
    result = true;
    is_cold_init = true;
  }
  return result;
}

bool RYS8833::try_get_dt_sync() {
  if (!is_cold_init) return false;

  bool result = false;
  String zda_str = "";
  if (gps_port->available()) {
    zda_str = read_response();
    result = try_decode_zda_response(zda_str);
  }
  return result;
}
