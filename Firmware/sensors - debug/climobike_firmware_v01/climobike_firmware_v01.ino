/* This is the Climobike project firmware.

 (insert more project info here)
   https://github.com/smjacques/climobike
*/

//Add Library
#include <SoftwareSerial.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <TinyGPS.h>
#include <SPI.h>
#include <SD.h>
#include <DHT.h>

// Defining Variables

//Humidity and Temperature - DHT22
#define DHTTYPE DHT22
#define dhtPin 5
DHT dht(dhtPin, DHTTYPE);

float Umidade = 0;           //umidade relativa (%)
float TempAr = 0;            //temperatura do ar (degrees F)
float IndexCalor = 0; //indice de calor (degrees F)

/*Dust Sensor (Shinyei PPD42NS/DSM501A)
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

#define ECHO_TO_SERIAL  ////comente esta linha para desligar o ECHO

// Carbon Monoxide (MQ-7)
//digital output (not used)
int CO_MQ7 = A0;

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
      // open a file only if it doesn't exist
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
 
//DHT22
Umidade = dht.readHumidity();
  delay(20);  
TempAr = dht.readTemperature(true);
  delay(20);  
IndexCalor = dht.computeHeatIndex(TempAr,Umidade); //indice  calculado a partir da umidade e temp do ar.

//MQ-7
//reads the analaog value from the CO sensor's AOUT pin
int CO_value = analogRead(CO_MQ7);

// Dust Sensor
  float ratio = 0;
  float concentration = 0;
  float ratio2 = 0;
  float concentration2 = 0;
  float temp1 = 0;
  float temp2 = 0;
  voMeasured1 = analogRead(valP1);
  voMeasured2 = analogRead(valP2);

//Sensor1
  lowpulseoccupancyP1 = lowpulseoccupancyP1 + durationP1;
  ratio = lowpulseoccupancyP1/(sampletime_ms*10.0);     // int percentage 0 to 100
  concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // spec sheet curve
  temp1 = voMeasured1 * (5.0 / 1024.0);

//Sensor2
    lowpulseoccupancyP2 = lowpulseoccupancyP2 + durationP2;
    ratio2 = lowpulseoccupancyP2/(sampletime_ms*10.0);
    concentration2 = 1.1*pow(ratio2,3)-3.8*pow(ratio2,2)+520*ratio2+0.62;
    temp2 = voMeasured2 * (5.0 / 1024.0);

/* linear equation taken from http://www.howmuchsnow.com/arduino/airquality/
Chris Nafis (c) 2012*/

  dustDensity2 = (0.17 * temp2 - 0.1)* (1000/60);
  dustDensity1 = (0.17 * temp1 - 0.1)* (1000/60);


//Data Output
  {
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;

  // For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (gpsSerial.available())
    {
      char c = gpsSerial.read();
      // Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }

  if (newData)
  {
    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);
    Serial.print("LAT=");
    Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    Serial.print(" LON=");
    Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
    Serial.print(" SAT=");
    Serial.print(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites());
    Serial.print(" PREC=");
    Serial.print(gps.hdop() == TinyGPS::GPS_INVALID_HDOP ? 0 : gps.hdop());
    Serial.print("Temperature");
    Serial.print(TempAr);
    Serial.print("Air Humidity (%)");
    Serial.print(Umidade);
    Serial.print("Heat Index");
    Serial.print(IndexCalor);
    Serial.print("Carbon Monoxide");
    Serial.print(CO_value);
    Serial.print("Large Dust Density(3-10 µm)");
    Serial.print(dustDensity1);
    Serial.print("Small Dust Density(1-2 µm)"); // unit: ug/m3
    Serial.print(dustDensity2);
    Serial.print("Large Dust Ratio(3-10 µm)");
    Serial.print(dustDensity1);
    Serial.print("Small Dust Ratio(1-2 µm)");
    Serial.print(dustDensity2);
    Serial.print("Dust Concentration(3-10 µm)");
    Serial.print(concentration);
    Serial.print("Dust Concentration(1-2 µm)");
    Serial.print(concentration2);
    
  }
  
  gps.stats(&chars, &sentences, &failed);
  Serial.print(" CHARS=");
  Serial.print(chars);
  Serial.print(" SENTENCES=");
  Serial.print(sentences);
  Serial.print(" CSUM ERR=");
  Serial.println(failed);
  if (chars == 0)
    Serial.println("** No characters received from GPS: check wiring **");
}


// delay
  smartdelay(1000);
}
