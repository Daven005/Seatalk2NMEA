#include "dataStore.h"


DataStore::DataStore() {
  p.latitude.lastUpdated = millis() - 999999;
  p.longitude.lastUpdated = millis() - 999999;
  spd.ground.lastUpdated = millis() - 999999;
  spd.water.lastUpdated = millis() - 999999;
  c.lastUpdated = millis() - 999999;
  dt.lastUpdated = millis() - 999999;
  tm.lastUpdated = millis() - 999999;
  d.lastUpdated = millis() - 999999;
  w.angle.lastUpdated = millis() - 999999;
  w.speed.lastUpdated = millis() - 999999;
}

bool DataStore::decodeSeaTalk(const word *msg, byte len) {
  switch (msg[0]) {
    case 0x100: // depth
      storeDepth(msg[2] >> 4, msg[2] & 0xf, (msg[4] << 4) | msg[3]);
      break;
    case 0x110: // Apparent Wind Angle
      storeApparentWindAngle((msg[2] << 8) | msg[3]);
      break;
    case 0x111: // Apparent Wind speed
      storeApparentWindSpeed(msg[2] & 0x7f, msg[3]);
      break;
    case 0x120: // Speed through Water
      break;
    case 0x150: // Latitude
      storeLatitude(msg[2], ((msg[4] << 8) | msg[3]) & 0x7fff, msg[4] >> 7);
      break;
    case 0x151: // Longitude
      storeLongitude(msg[2], ((msg[4] << 8) | msg[3]) & 0x7fff, msg[4] >> 7);
      break;
    case 0x152: // Speed over Ground
      break;
    case 0x153: // Course Magnetic
      break;
    case 0x154: // Time
      storeTime(msg[2] << 4 | (msg[1]  & 0xf), msg[3]);
      break;
    case 0x156: // Date
      storeDate(msg[2], msg[1] >> 4, msg[3]);
      break;
    case 0x19c: // Compass heading
      break;
  }
}

void DataStore::storeDepth(byte units, byte flags, word depth) {
  long millimetres_10;
  d.alarmFlags = flags;
  millimetres_10 = depth * 305;
  d.metres = millimetres_10 / 10000;
  d.decimal = (millimetres_10 / 100) - (d.metres * 100);
  d.lastUpdated = millis();
}

char *DataStore::printDepth(char *bfr) {
  sprintf(bfr, "%d.%02d", d.metres, d.decimal);
  return bfr + strlen(bfr);
}

bool DataStore::isValidDepth() {
  return (millis() - d.lastUpdated) < MAX_TIME_VALID;
}

void DataStore::storeApparentWindAngle(word degrees_2) {
  w.angle.degrees = degrees_2 / 2;
  w.angle.lastUpdated = millis();
}

void DataStore::storeApparentWindSpeed(word knots, byte decimal) {
  w.speed.knots = knots;
  w.speed.decimal = decimal;
  w.speed.lastUpdated = millis();
}

bool DataStore::isValidApparentWindAngle() {
  return (millis() - w.angle.lastUpdated) < MAX_TIME_VALID;
}

bool DataStore::isValidApparentWindSpeed() {
  return (millis() - w.speed.lastUpdated) < MAX_TIME_VALID;
}

bool DataStore::isValidApparentWind() {
  return isValidApparentWindAngle() && isValidApparentWindSpeed();
}

char *DataStore::printApparentWindAngle(char *bfr) {
  sprintf(bfr, "%03d", w.angle.degrees);
  return bfr + 3;
}

char *DataStore::printApparentWindSpeed(char *bfr) {
  sprintf(bfr, "%02d.%1d", w.speed.knots, w.speed.decimal);
  return bfr + 4;
}

void DataStore::storeLatitude(byte degrees, word minutes, bool south) {
  p.latitude.degrees = degrees;
  p.latitude.minutes = minutes / 100;
  p.latitude.decimal = minutes - p.latitude.minutes * 100;
  p.latitude.south = south;
  p.latitude.lastUpdated = millis();
}

char *DataStore::printLatitude(char *bfr) {
  sprintf(bfr, "%02d%02d.%02d%c",
          p.latitude.degrees, p.latitude.minutes, p.latitude.decimal, (p.latitude.south) ? 'S' : 'N');
  return bfr + 8;
}

bool DataStore::isValidLatitude() {
  return (millis() - p.latitude.lastUpdated) < MAX_TIME_VALID;
}

void DataStore::storeLongitude(byte degrees, word minutes, bool east) {
  p.longitude.degrees = degrees;
  p.longitude.minutes = minutes / 100;
  p.longitude.decimal = minutes - p.longitude.minutes * 100;
  p.longitude.east = east;
  p.longitude.lastUpdated = millis();
}

char *DataStore::printLongitude(char *bfr) {
  sprintf(bfr, "%02d%02d.%02d%c",
          p.longitude.degrees, p.longitude.minutes, p.longitude.decimal, (p.longitude.east) ? 'E' : 'W');
  return bfr + 8;
}

bool DataStore::isValidLongitude() {
  return (millis() - p.longitude.lastUpdated) < MAX_TIME_VALID;
}

bool DataStore::isValidPosition() {
  return isValidLongitude() && isValidLatitude();
}

void DataStore::storeTime(byte hours, word seconds) {
  tm.hour = hours;
  tm.minute = seconds / 60;
  tm.second = seconds - tm.second * 60;
  tm.lastUpdated = millis();
}

char *DataStore::printTime(char *bfr) {
  sprintf(bfr, "%02d%02d%02d.00", tm.hour, tm.minute, tm.second);
  return bfr + 9;
}

void DataStore::storeDate(byte day, byte month, byte year) {
  dt.day = day;
  dt.month = month;
  dt.year = year;
  dt.lastUpdated = millis();
}
