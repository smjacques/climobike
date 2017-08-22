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
#include <Wire.h>
#include <TinyGPS.h>
#include <SPI.h>
#include <SD.h>
#include <DHT.h>

// Defining Variables

//Humidity and Temperature - DHT22
#define dhtPin A1 // pino que estamos conectado
#define DHTTYPE DHT11 // DHT 11

//#define DHTTYPE DHT22
//#define dhtPin 10
DHT dht(dhtPin, DHTTYPE);

float Umidade = 0;           //umidade relativa (%)
float TempAr = 0;            //temperatura do ar (degrees F)
float IndexCalor = 0; //indice de calor (degrees F)

/*Dust Sensor (Shinyei PPD42NS)
 pin 1 -> Arduino GND
 pin 3 -> Arduino 5V
 pin 2 -> Pin ~7 (P1 signal out (large particulates, 3-10 µm)
 pin 4 -> Pin ~8 (P2 signal out (small particulates, 1-2 µm)
 pin 5 -> unused!
*/
int valP1 = 7;
int valP2 = 8;

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
//#define voltage_regulator_digital_out_pin 9 (not used)
#define MQ7_analog_in_pin 0
int CO_value;

//Defining RTC
virtuabotixRTC RTC_data(2, 5, 6); //pin order(clock, data, rst)

//GPS Module
TinyGPS gps;
SoftwareSerial gpsSerial(3,4);

static void smartdelay(unsigned long ms);
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
static void print_str(const char *str, int len);

/*SD card
MOSI D11
MISO D12
CLK  D13
CS   D10
*/
#define chipSelect 10
File logfile;


void setup()
{
  Serial.begin(9600);
  gpsSerial.begin(9600);
  
  pinMode(MQ7_analog_in_pin, INPUT);
  starttime = millis();
  pinMode(valP1,INPUT);
  pinMode(valP1,INPUT);
  RTC_data.setDS1302Time(00, 00, 22, 4, 19, 8, 2017);
  
//teste se o SD esta presente e inicia
  if (!SD.begin(chipSelect)) {
    Serial.println("SD com defeito ou ausente");        
  }
  
  Serial.println("SD iniciado");
  
  // criar novo arquivo
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // apenas abre um novo arquivo se ele nao existe
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // abandonar o loop!
    }
  }
  
  if (! logfile) {
    Serial.println("impossvel criar arquivo");
  }
  
  Serial.print("Logging to: ");
Serial.println(filename);
  
  //Serial Monitor Data
  Serial.print("Testing TinyGPS library v. "); Serial.println(TinyGPS::library_version());
  Serial.println();
  Serial.println("Sats HDOP Latitude  Longitude  Fix  Date       Time     Date Alt    Course Speed Card  Chars Sentences Checksum  Gas Level  Large Dust(3-10 µm) Small Dust(1-2 µm)  ");
  Serial.println("          (deg)     (deg)      Age                      Age  (m)    --- from GPS ----  RX    RX        Fail");
  Serial.println("----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");
  
  //SD card (log) data
  logfile.print("Testing TinyGPS library v. "); Serial.println(TinyGPS::library_version());
  logfile.println();
  logfile.println("Sats HDOP Latitude  Longitude  Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum  Gas Level  Large Dust(3-10 µm) Small Dust(1-2 µm)  ");
  logfile.println("          (deg)     (deg)      Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail");
  logfile.println("----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");
  
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

void loop()
{
/*========================================================================================/
                                            RTC
/=======================================================================================*/
RTC_data.updateTime();


/*=======================================================================================/
                                             GPS MODULE
/=======================================================================================*/
  float flat, flon;
  unsigned long age, date, time, chars = 0;
  unsigned short sentences = 0, failed = 0;
  static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;
 
/*==================================================================================/
                                       DHT22
/===================================================================================*/

Umidade = dht.readHumidity();
  delay(20);
  
  TempAr = dht.readTemperature(true);
  delay(20);
  
IndexCalor = dht.computeHeatIndex(TempAr,Umidade); //indice  calculado a partir da umidade e temp do ar.

/*
 ==================================================================================/
                                       MQ-7
/===================================================================================
*/
CO_value= analogRead(MQ7_analog_in_pin);//reads the analaog value from the CO sensor's AOUT pin

/*==================================================================================/
                                       Data Output
/===================================================================================*/

 
  print_int(gps.satellites(), TinyGPS::GPS_INVALID_SATELLITES, 5);
  print_int(gps.hdop(), TinyGPS::GPS_INVALID_HDOP, 5);
  gps.f_get_position(&flat, &flon, &age);
  
  print_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 9, 5);
  print_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 10, 5);
  print_int(age, TinyGPS::GPS_INVALID_AGE, 5);

  print_date(gps);

  print_float(gps.f_altitude(), TinyGPS::GPS_INVALID_F_ALTITUDE, 8, 2);
  print_float(gps.f_course(), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2);
  print_float(gps.f_speed_kmph(), TinyGPS::GPS_INVALID_F_SPEED, 6, 2);
  print_str(gps.f_course() == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(gps.f_course()), 6);

  gps.stats(&chars, &sentences, &failed);
  print_int(chars, 0xFFFFFFFF, 6);
  print_int(sentences, 0xFFFFFFFF, 10);
  print_int(failed, 0xFFFFFFFF, 9);
  Serial.println();
  
  smartdelay(1000);

  
  // log time
  logfile.print(RTC_data.year);
  logfile.print("/");
  logfile.print(RTC_data.month);
  logfile.print("/");
  logfile.print(RTC_data.dayofmonth);
  logfile.print(" ");
  logfile.print(RTC_data.hours);
  logfile.print(":");
  logfile.print(RTC_data.minutes);
  logfile.print(":");
  logfile.print(RTC_data.seconds);
  logfile.print(",");
 #if ECHO_TO_SERIAL
  Serial.print(",");
  Serial.print(RTC_data.year);
  Serial.print("/");
  Serial.print(RTC_data.month);
  Serial.print("/");
  Serial.print(RTC_data.dayofmonth);
  Serial.print(" ");
  Serial.print(RTC_data.hours);
  Serial.print(":");
  Serial.print(RTC_data.minutes);
  Serial.print(":");
  Serial.print(RTC_data.seconds);
  Serial.print(",");
#endif //ECHO_TO_SERIAL

  //Log variables
  gps.f_get_position(&flat, &flon, &age);
  logfile.print(gps.satellites(), 5);
  logfile.print(",");
  logfile.print(gps.hdop(), 5);
  logfile.print(",");  
  logfile.print(",");
  logfile.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
  logfile.print(",");
  logfile.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
  logfile.print(",");
  logfile.print(IndexCalor);
  logfile.print(",");
  logfile.print(TempAr);
  logfile.print(",");
  logfile.print(Umidade);
  logfile.print(",");
  logfile.print(IndexCalor);
  logfile.print(",");
  logfile.println(CO_value);//prints the CO value
  
#if ECHO_TO_SERIAL
  Serial.print(gps.satellites(), 5);
  Serial.print(",");
  Serial.print(gps.hdop(), 5);
  Serial.print(",");  
  Serial.print(",");
  Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
  Serial.print(",");
  Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
  Serial.print(",");
  Serial.print(IndexCalor);
  Serial.print(",");
  Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
  Serial.print(",");
  Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
  Serial.print(TempAr);
  Serial.print(",");
  Serial.print(Umidade);
  Serial.print(",");
  Serial.print(IndexCalor);
  Serial.print(",");
  Serial.println(CO_value);//prints the CO value
  
#endif

}
