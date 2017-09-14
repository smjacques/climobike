/* This is the Climobike project firmware.

 (insert more project info here)
   https://github.com/smjacques/climobike
*/

//adicionando bibliotecas
#include <SoftwareSerial.h>
#include <Adafruit_Sensor.h>
#include <virtuabotixRTC.h>
#include <Wire.h>
#include <TinyGPS.h>
#include <SPI.h>
#include <SD.h>
#include <DHT.h>

// Defining Variables

//Umidade e Temperatura - DHT22
#define DHTTYPE DHT22
#define dhtPin 5
DHT dht(dhtPin, DHTTYPE);

float Umidade = 0;           //umidade relativa (%)
float TempAr = 0;            //temperatura do ar (degrees F)
float IndexCalor = 0;       //indice de calor (degrees F)

/* Sensor de poeira(Shinyei PPD42NS/DSM501A)
 pin 1 -> Arduino GND
 pin 3 -> Arduino 5V
 pin 2 -> Pin ~7 (P1 signal out (particulado grande, 3-10 µm)
 pin 4 -> Pin ~8 (P2 signal out (particulado pequeno, 1-2 µm)
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

//#define ECHO_TO_SERIAL  ////comente esta linha para desligar o ECHO

// Carbon Monoxide (MQ-7)
//digital output (not used)
int CO_MQ7 = A0;

//GPS Module
//   Conectar o TX do GPS (transmit) na Digital 3 do Arduino
//   Conectar o RX do GPS (receive) na Digital 4 do Arduino
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
  
  pinMode(CO_MQ7, INPUT);
  starttime = millis();
  pinMode(valP1,INPUT);
  pinMode(valP1,INPUT);
  
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
      // abre arquivo se inexistente
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // abandonar o loop!
    }
  }
  
  if (! logfile) {
    Serial.println("impossvel criar arquivo");
  }
  
  Serial.print("Logging to: ");
Serial.println(filename);
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

//GPS MODULE
  float flat, flon;
  unsigned long age, date, time, chars = 0;
  unsigned short sentences = 0, failed = 0;
  static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;
 
//DHT22
Umidade = dht.readHumidity();
  delay(20);  
TempAr = dht.readTemperature(); //use true se farenheit
  delay(20);  
IndexCalor = dht.computeHeatIndex(TempAr,Umidade); //indice  calculado a partir da umidade e temp do ar.

//MQ-7
//apenas leitura analogica do sensor de CO (pino AOUT do sensor)
int CO_value = analogRead(CO_MQ7);

//Sensor de Particulado
float ratio = 0;
  float concentration = 0;
  float ratio2 = 0;
  float concentration2 = 0;
  float temp1 = 0;
  float temp2 = 0;
  voMeasured1 = analogRead(valP1);
  voMeasured2 = analogRead(valP2);
  
  
//Sensor1 (particulado grande, 3-10 µm)
  lowpulseoccupancyP1 = lowpulseoccupancyP1 + durationP1;
  ratio = lowpulseoccupancyP1/(sampletime_ms*10.0);     // int percentage 0 to 100
  concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // spec sheet curve
  temp1 = voMeasured1 * (5.0 / 1024.0);

//Sensor2 (particulado pequeno, 1-2 µm)
    lowpulseoccupancyP2 = lowpulseoccupancyP2 + durationP2;
    ratio2 = lowpulseoccupancyP2/(sampletime_ms*10.0);
    concentration2 = 1.1*pow(ratio2,3)-3.8*pow(ratio2,2)+520*ratio2+0.62;
    temp2 = voMeasured2 * (5.0 / 1024.0);

/* Eq. linear obtida em http://www.howmuchsnow.com/arduino/airquality/
(Chris Nafis 2012)*/

  dustDensity2 = (0.17 * temp2 - 0.1)* (1000/60);
  dustDensity1 = (0.17 * temp1 - 0.1)* (1000/60);
  

//Data Output
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
  
 
  //Log variables
  gps.f_get_position(&flat, &flon, &age);
  logfile.print(gps.satellites(), 5);
  logfile.print(",");
  logfile.print(gps.hdop(), 5);
  logfile.print(",");  
  logfile.print("GPS Latitude");
  logfile.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
  logfile.print("GPS Longitude");
  logfile.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    //Inserir data/ hora e velocidade (km/h) 
  logfile.print("Air Temperature");
  logfile.print(TempAr);
  logfile.print("Air humidity");
  logfile.print(Umidade);
  logfile.print("Heat Index");
  logfile.print(IndexCalor);
  logfile.print("CO Level");
  logfile.print(CO_value);//prints dos valores de CO
  logfile.print("Large Dust Density (µg/m3)");
  logfile.print(dustDensity1);
  logfile.print("Large Dust Ratio (3-10µm)");
  logfile.print(ratio);
  logfile.print("Large Dust Concentration (3-10 µm)");
  logfile.print(concentration);
  logfile.print("Fine Dust Density (1-2 µm)");
  logfile.print(dustDensity2); // unit: ug/m3
  logfile.print("Fine Dust Ratio (µg/m3)");
  logfile.print(ratio2);
  logfile.print("Fine Dust Concentration (1-2 µm)");
  logfile.println(concentration2);
  
  
#ifdef ECHO_TO_SERIAL
  Serial.print(gps.satellites(), 5);
  Serial.print(",");
  Serial.print(gps.hdop(), 5);
  Serial.print(",");  
  Serial.print("GPS Latitude");
  Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
  Serial.print("GPS Longitude");
  Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
  //Inserir data/ hora e velocidade (km/h) 
  Serial.print("Air Temperature");
  Serial.print(TempAr);
  Serial.print("Air Humidity");
  Serial.print(Umidade);
  Serial.print("Heat Index");
  Serial.print(IndexCalor);
  Serial.print("CO level");
  Serial.print(CO_value);
  Serial.print("Large Dust Density (µg/m3)");
  Serial.print(dustDensity1);
  Serial.print("Large Dust Ratio (3-10µm)");
  Serial.print(ratio);
  Serial.print("Large Dust Concentration (3-10 µm)");
  Serial.print(concentration);
  Serial.print("Fine Dust Density (1-2 µm)");
  Serial.print(dustDensity2); // unit: ug/m3
  Serial.print("Fine Dust Ratio (µg/m3)");
  Serial.print(ratio2);
  Serial.print("Fine Dust Concentration (1-2 µm)");
  Serial.println(concentration2);
  
#endif
  Serial.println("");


// delay
  smartdelay(1000);
}
