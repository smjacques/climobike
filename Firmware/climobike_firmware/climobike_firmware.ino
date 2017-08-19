/* This is the Climobike project firmware.

   It requires the use of SoftwareSerial, TinySerial and ?? library and assumes that you have
   a 4800-baud serial GPS device hooked up on pins 4(rx sensor) and 3(tx sensor).
   You can find out more info about this project and how to make your own monitoring system at
   https://github.com/smjacques/climobike
*/

//Add Library
#include <SoftwareSerial.h>
#include <Adafruit_Sensor.h>
#include <virtuabotixRTC.h>
#include <TinyGPS.h>
#include <SPI.h>
#include <SD.h>
#include <DHT.h>

// Defining Variables

//Humidity and Temperature - DHT22
#define DHTTYPE DHT22
#define dhtPin 2
DHT dht(dhtPin, DHTTYPE);

/*Dust Sensor (Shinyei PPD42NS)
 pin 1 (grey)  -> Arduino GND
 pin 3 (blue) -> Arduino 5V
 pin 2 (green) -> Pin ~3 (P1 signal out (large particulates, 3-10 µm)
 pin 4 (white) -> Pin ~6 (P2 signal out (small particulates, 1-2 µm)
 pin 5 (red)   -> unused!
*/
int valP1 = 3;
int valP2 = 6;

float voMeasured1 = 0;
float voMeasured2 = 0;
float calcVoltage1 = 0;
float calcVoltage2 = 0;
float dustDensity1 = 0;
float dustDensity2 = 0;

unsigned long starttime;
unsigned long durationP1;
unsigned long durationP2;

unsigned long sampletime_ms = 2600; //intervalo entre medida
unsigned long lowpulseoccupancyP1 = 0;
unsigned long lowpulseoccupancyP2 = 0;

#define ECHO_TO_SERIAL 1 ////Envia registro de dados para serial se 1, nada se 0


// Carbon Monoxide (MQ-7)
#define voltage_regulator_digital_out_pin 8 
#define MQ7_analog_in_pin 0

#define MQ7_heater_5v_time_millis 60000
#define MQ7_heater_1_4_V_time_millis 90000
#define gas_level_reading_period_millis 1000

unsigned long startMillis;
unsigned long switchTimeMillis;
boolean heaterInHighPhase;

//Defining RTC
virtuabotixRTC RTC_data(10, 9, 8); //pin order(clock, data, rst)

//GPS Module
TinyGPS gps;
SoftwareSerial gpsSerial(3,4);

static void smartdelay(unsigned long ms);
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
static void print_str(const char *str, int len);

//SD card
File logfile;

void setup()
{
  Serial.begin(9600);
  gpsSerial.begin(9600);
  
  pinMode(voltage_regulator_digital_out_pin, OUTPUT);
  starttime = millis();
  turnHeaterHigh();
  pinMode(valP1,INPUT);
  pinMode(valP1,INPUT);
  RTC_data.setDS1302Time(00, 00, 22, 4, 19, 8, 2017);
  
  //Serial Monitor Data
  Serial.print("Testing TinyGPS library v. "); Serial.println(TinyGPS::library_version());
  Serial.println();
  Serial.println("Sats HDOP Latitude  Longitude  Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum  Gas Level  Large Dust(3-10 µm) Small Dust(1-2 µm)  ");
  Serial.println("          (deg)     (deg)      Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail");
  Serial.println("----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");
  
  //SD card (log) data
  logfile.print("Testing TinyGPS library v. "); Serial.println(TinyGPS::library_version());
  logfile.println();
  logfile.println("Sats HDOP Latitude  Longitude  Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum  Gas Level  Large Dust(3-10 µm) Small Dust(1-2 µm)  ");
  logfile.println("          (deg)     (deg)      Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail");
  logfile.println("----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");
  
}

void loop()
{
/*========================================================================================/
                                            RTC
/=======================================================================================*/


/*=========================================================================================/
                                             SD
/=========================================================================================*/





/*=======================================================================================/
                                             GPS MODULE
/=======================================================================================*/
  float flat, flon;
  unsigned long age, date, time, chars = 0;
  unsigned short sentences = 0, failed = 0;
  static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;
 
/*=====================================================================================/
                              MQ-7 Sensor
/====================================================================================*/
  
   if (heaterInHighPhase) {
    // 5v phase of cycle. see if need to switch low yet
    if (millis() > switchTimeMillis) {
      turnHeaterLow();
    }
  }
  else {
    // 1.4v phase of cycle. see if need to switch high yet
    if (millis() > switchTimeMillis) {
      turnHeaterHigh();
    }
  }

  readGasLevel();
  delay(gas_level_reading_period_millis); //Here I've to change delay to "gps data"

/*==================================================================================/
                                       DHT22
/===================================================================================*/

Umidade = dht.readHumidity();
  delay(20);
  
  TempAr = dht.readTemperature(true);
  delay(20);
  
IndexCalor = dht.computeHeatIndex(TempAr,Umidade); //indice  calculado a partir da umidade e temp do ar.


/*==================================================================================/
                                       Data Output
/===================================================================================*/

 
  print_int(gps.satellites(), TinyGPS::GPS_INVALID_SATELLITES, 5);
  print_int(gps.hdop(), TinyGPS::GPS_INVALID_HDOP, 5);
  gps.f_get_position(&flat, &flon, &age);
  print_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6);
  print_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6);
  print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
  print_date(gps);
  print_float(gps.f_altitude(), TinyGPS::GPS_INVALID_F_ALTITUDE, 7, 2);
  print_float(gps.f_course(), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2);
  print_float(gps.f_speed_kmph(), TinyGPS::GPS_INVALID_F_SPEED, 6, 2);
  print_str(gps.f_course() == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(gps.f_course()), 6);
  print_int(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0xFFFFFFFF : (unsigned long)TinyGPS::distance_between(flat, flon, LONDON_LAT, LONDON_LON) / 1000, 0xFFFFFFFF, 9);
  print_float(flat == TinyGPS::GPS_INVALID_F_ANGLE ? TinyGPS::GPS_INVALID_F_ANGLE : TinyGPS::course_to(flat, flon, LONDON_LAT, LONDON_LON), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2);
  print_str(flat == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(TinyGPS::course_to(flat, flon, LONDON_LAT, LONDON_LON)), 6);

  gps.stats(&chars, &sentences, &failed);
  print_int(chars, 0xFFFFFFFF, 6);
  print_int(sentences, 0xFFFFFFFF, 10);
  print_int(failed, 0xFFFFFFFF, 9);
  Serial.println();
  
  smartdelay(1000);
  

now = rtc.now();
  
  // log time
  logfile.print(now.unixtime()); // segundos desde 2000
  logfile.print(",");
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print(",");
 #if ECHO_TO_SERIAL
  Serial.print(now.unixtime()); // segundos desde 2000
  Serial.print(",");
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  Serial.print(",");
#endif //ECHO_TO_SERIAL

  //Log variables
  gps.f_get_position(&flat, &flon, &age);
  logfile.print(gps.satellites(), TinyGPS::GPS_INVALID_SATELLITES, 5);
  logfile.print(",");
  logfile.print(gps.hdop(), TinyGPS::GPS_INVALID_HDOP, 5);
  logfile.print(",");
  logfile.print(flat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6);
  logfile.print(",");
  logfile.print(flon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6);
  logfile.print(",");
  logfile.print(IndexCalor);
  logfile.print(",");
  logfile.print(LuzSol);
  logfile.print(",");
#if ECHO_TO_SERIAL
  Serial.print(TempSolo);
  Serial.print(",");
  Serial.print(TempAr);
  Serial.print(",");
  Serial.print(UmidSolo);
  Serial.print(",");
  Serial.print(Umidade);
  Serial.print(",");
  Serial.print(IndexCalor);
  Serial.print(",");
  Serial.print(LuzSol);
  Serial.print(",");
#endif

}

/*
======================== GPS functions ==============================================
*/

static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (gpsSerial.available())
      gps.encode(gpsSerial.read());
  } while (millis() - start < ms);
}

static void print_float(float val, float invalid, int len, int prec)
{
  if (val == invalid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(' ');
  }
  smartdelay(0);
}

static void print_int(unsigned long val, unsigned long invalid, int len)
{
  char sz[32];
  if (val == invalid)
    strcpy(sz, "*******");
  else
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0)
    sz[len-1] = ' ';
  Serial.print(sz);
  smartdelay(0);
}

static void print_date(TinyGPS &gps)
{
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  if (age == TinyGPS::GPS_INVALID_AGE)
    Serial.print("********** ******** ");
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d ",
        month, day, year, hour, minute, second);
    Serial.print(sz);
  }
  print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
  smartdelay(0);
}

static void print_str(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    Serial.print(i<slen ? str[i] : ' ');
  smartdelay(0);
}

/*
=================================== MQ-7 Functions =================================
*/

void turnHeaterHigh() {
  // 5v phase
  digitalWrite(voltage_regulator_digital_out_pin, LOW);
  heaterInHighPhase = true;
  switchTimeMillis = millis() + MQ7_heater_5v_time_millis;
}

void turnHeaterLow() {
  // 1.4v phase
  digitalWrite(voltage_regulator_digital_out_pin, HIGH);
  heaterInHighPhase = false;
  switchTimeMillis = millis() + MQ7_heater_1_4_V_time_millis;
}

void readGasLevel() {
  unsigned int gasLevel = analogRead(MQ7_analog_in_pin);
  unsigned int time = (millis() - startMillis) / 1000;

  Serial.print(time);
  Serial.print(",");
  Serial.println(gasLevel);
}
