#ifndef DEFS_H
#define DEFS_H

#define RED_LED 19
#define YELLOW_LED 18
#define GREEN_LED 5
#define STIN 14
#define STOUT 12
#define NMEA_OUT 27
#define NMEA_IN 26
#define NMEA_TX 25
#define LCD_RST_ 16
#define LCD_CE_ 4
#define LCD_DC 23
#define LCD_DIN 22
#define LCD_CLK 21
#define LCD_LT_ 34

typedef enum { PRINT_ACTION, COMMAND_ACTION, SEATALK_ACTION } NmeaAction_t;
typedef void (*ProcessNmeaInput_t) (NmeaAction_t action, const char *data);

#endif
