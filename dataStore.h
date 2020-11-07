#ifndef DATA_STORE_H
#define DATA_STORE_H

#include "Arduino.h"

/* SeaTalk message Formats recognised
    00 02 YZ XX XX Depth below transducer: XXXX/10 feet
        Display units: Y=0 => feet, Y=4 => meter
        Flags: Z&1 Shalow Depth Alarm (Z=1)
        Z&2 Transducer defective (Z=4)
        Corresponding NMEA sentences: DPT, DBT
    
   50 A2 XX YY YY LAT position: XX degrees, (YYYY & 0x7FFF)/100 minutes
   MSB of Y = YYYY & 0x8000 = South if set, North if cleared
   Corresponding NMEA sentences: RMC, GAA, GLL

   51 A2 XX YY YY LON position: XX degrees, (YYYY & 0x7FFF)/100 minutes
   MSB of Y = YYYY & 0x8000 = East if set, West if cleared
   Corresponding NMEA sentences: RMC, GAA, GLL

   52 01 XX XX Speed over Ground: XXXX/10 Knots
   Corresponding NMEA sentences: RMC, VTG
   53 X0 XX Course Magnetic: XXX/8 Degrees

   Least significant 2 bits are always 0,
   giving a resolution of 0.5 degrees
   Corresponding NMEA sentences: RMC, VTG

   54 S1 SS HH GMT-time: HH hours, SSS seconds
   Corresponding NMEA sentences: RMC, GAA, BWR, BWC

   9C U1 VW RR Compass heading and Rudder position (see also command 84)
    Compass heading in degrees:
    The two lower bits of U * 90 +
    the six lower bits of VW * 2 +
    the two higher bits of U / 2 =
    (U & 0x3) * 90 + (VW & 0x3F) * 2 + (U & 0xC) / 8

*/
#define MAX_TIME_VALID 60*1000 // 1 minute

class DataStore {
    typedef struct Position {
      struct Latitude {
        bool south;
        uint8_t degrees;
        uint8_t minutes;
        uint8_t decimal;
        unsigned long lastUpdated;
      } latitude;
      struct Longitude {
        bool east;
        uint8_t degrees;
        uint8_t minutes;
        uint8_t decimal;
        unsigned long lastUpdated;
      } longitude;
    } position;
    typedef struct Speed {
      struct Ground {
        uint8_t knots;
        uint8_t decKnots;
        unsigned long lastUpdated;
      } ground;
      struct Water {
        uint8_t knots;
        uint8_t decKnots;
        unsigned long lastUpdated;
      } water;
    } Speed;
    typedef struct Course {
      uint16_t degrees; // Store as magnetic
      uint8_t decimal;
      char variation;
      unsigned long lastUpdated;
    } course;
    typedef struct Date {
      uint8_t day;
      uint8_t month;
      uint16_t year;
      unsigned long lastUpdated;
    } date;
    typedef struct Time {
      uint8_t hour;
      uint8_t minute;
      uint8_t second;
      unsigned long lastUpdated;
    } time;
    typedef struct Depth {
      word metres;
      byte decimal;
      byte alarmFlags;
      unsigned long lastUpdated;
    } depth;
    typedef struct Wind {
      struct Angle {
        word degrees;
        unsigned long lastUpdated;
      } angle;
      struct Speed {
        byte knots;
        byte decimal;
        unsigned long lastUpdated;
      } speed;
    } wind;
    Position p;
    Speed spd;
    Course c;
    Date dt;
    Time tm;
    Depth d;
    Wind w;
    void storeDepth(byte units, byte flags, word depth);
    void storeApparentWindAngle(word degrees_2);
    void storeApparentWindSpeed(word knots, byte decimal);
    void storeLatitude(byte degrees, word minutes, bool north);
    void storeLongitude(byte degrees, word minutes, bool east);
    void storeTime(byte hours, word seconds);
    void storeDate(byte day, byte month, byte year);
  public:
    DataStore();
    bool decodeSeaTalk(const word *msg, byte len);
    char *printLatitude(char *);
    char *printLongitude(char *);
    char *storeApparentWindAngle(char *);
    char *storeApparentWindSpeed(char *);
    char *printSpeed(char *);
    char *printCourse(char *);
    char *printApparentWindAngle(char *);
    char *printApparentWindSpeed(char *);
    char *printTime(char *);
    char *printDate(char *);
    char *printDepth(char *);
    bool isValidLatitude();
    bool isValidLongitude();
    bool isValidPosition();
    bool isValidApparentWindAngle();
    bool isValidApparentWindSpeed();
    bool isValidApparentWind();
    bool isValidSpeed();
    bool isValidCourse();
    bool isValidTime();
    bool isValidDate();
    bool isValidDepth();
};

#endif
