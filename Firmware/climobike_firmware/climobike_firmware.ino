#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <TinyGPS.h>
#include <SPI.h>
#include <DHT.h>
#include <SD.h>
#include <SoftwareSerial.h>

//GPS Module
//   Conectar o TX do GPS (transmit) na Digital 3 do Arduino
//   Conectar o RX do GPS (receive) na Digital 4 do Arduino

SoftwareSerial gpsSerial(3,4); // configura a porta serial
TinyGPS gps; // Cria uma "instance" do objeto TinyGPS

//SD card
//  MOSI D11
//  MISO D12
//  CLK  D13
//  CS   D10

#define SDcard 10
File dataFile = SD.open("DATA.TXT", FILE_WRITE);

//Umidade e Temperatura - DHT22

#define DHTTYPE DHT22
#define dhtPin 5
DHT dht(dhtPin, DHTTYPE);

float Umidade = 0;           //umidade relativa (%)
float TempAr = 0;            //temperatura do ar
float IndexCalor = 0;       //indice de calor

//Sensor de poeira(Shinyei PPD42NS/DSM501A)
//  pin 1 -> Arduino GND
//  pin 3 -> Arduino 5V
//  pin 2 -> Pin ~7 (P1 signal out (particulado grande, 3-10 µm)
//  pin 4 -> Pin ~8 (P2 signal out (particulado pequeno, 1-2 µm)
//  pin 5 -> unused!

int valP1 = 7; //
int valP2 = 8; //

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

// Carbon Monoxide (MQ-7)
//digital output (not used)
int CO_MQ7 = A0;


void setup()
{
  gpsSerial.begin(9600);
  Serial.begin(9600);
  dht.begin();
  Wire.begin();
  pinMode(valP1,INPUT);
  pinMode(valP1,INPUT);
  pinMode(CO_MQ7, INPUT);
  pinMode(SDcard, OUTPUT);
  
  // check that the microSD card exists and can be used v
  if (!SD.begin(SDcard)) {
    Serial.println("MicroSD falhou ou ausente");
    // stop the sketch
    return;
  }
  Serial.println("MicroSD pronto!");
  
}


void getgps(TinyGPS &gps)
{
    float latitude, longitude;
  // decode and display time data
  int year;
  byte month, day, hour, minute, second, hundredths;
  gps.crack_datetime(&year,&month,&day,&hour,&minute,&second,&hundredths);
  
  //decode and display position data
  gps.f_get_position(&latitude, &longitude); 
    
    // Print data and time
    Serial.print("Date: "); 
    Serial.print(day, DEC);
    Serial.print("/");
    Serial.print(month, DEC);
    Serial.print("/");
    Serial.print(year, DEC);
    Serial.print(" ");
    Serial.print("Altitude ");
    Serial.print(gps.f_altitude());
    Serial.print("m ");
    Serial.print("Lat: "); 
    Serial.print(latitude,5); 
    Serial.print(" ");
    Serial.print("Long: "); 
    Serial.print(longitude,5);
    Serial.print(" ");  
    
    // correct for your time zone as in Project #45
    hour=hour; //Brazilian time zone (-3:00)
    if (hour>23)
    {
      hour=hour-24;
    }
   if (hour<10)
    {
    Serial.print("0");
    }  
    Serial.print(hour, DEC);
    Serial.print(":");
    if (minute<10) 
    {
    Serial.print("0");
    }
    Serial.print(minute, DEC);
    Serial.print(":");
    if (second<10)
    {
    Serial.print("0");
    }
    Serial.print(second, DEC);
    Serial.print(" ");
    Serial.print(gps.f_speed_kmph());
    Serial.println("km/h");     
    Serial.print("Air Temperature ");
    Serial.print(TempAr);
    Serial.print(" ");
    Serial.print("Air Humidity ");
    Serial.print(Umidade);
    Serial.print(" ");
    Serial.print("Heat Index");
    Serial.print(IndexCalor);
    Serial.print(" ");
    delay(5000); // record a measurement about every 10 seconds
  
  // if the file is ready, write to it
  if (dataFile) 
  {
    dataFile.print("Date: "); 
    dataFile.print(" ");
    dataFile.print(day, DEC);
    dataFile.print("/");
    dataFile.print(month, DEC);
    dataFile.print("/");
    dataFile.print(year, DEC);
    dataFile.println("Altitude ");
    dataFile.print(gps.f_altitude());
    dataFile.print("m ");
    dataFile.print("Lat: "); 
    dataFile.print(latitude,5); 
    dataFile.print(" ");
    dataFile.print("Long: "); 
    dataFile.print(longitude,5);
    dataFile.print(" ");  
   
   // correct for your time zone as in Project #45
    hour=hour-3; //Brazilian time zone (-3:00)
   
    if (hour>23)
    {
      hour=hour-24;
    }
   if (hour<10)
    {
      dataFile.print("0");
    }  
    dataFile.print(hour, DEC);
    dataFile.print(":");
    if (minute<10) 
    {
      dataFile.print("0");
    }
    dataFile.print(minute, DEC);
    dataFile.print(":");
    if (second<10)
    {
      dataFile.print("0");
    }
    dataFile.print(second, DEC);
    dataFile.print(" ");
    dataFile.print(gps.f_speed_kmph());
    dataFile.println("km/h");     
    dataFile.close(); // this is mandatory   
    delay(5000); // record a measurement about every 10 seconds
  }
}
 


void loop() 
{  
  byte a;
  if ( gpsSerial.available() > 0 )  // if there is data coming into the serial line
  {
    a = gpsSerial.read();          // get the byte of data
    if(gps.encode(a))           // if there is valid GPS data...
    {
      getgps(gps);              // grab the data and display it
    }
  }
}

//============================== Environmental Variables Functions =========================

float tempReading() // Function TempReading is declared
{  
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float TempAr = dht.readTemperature();
  if (isnan(TempAr)) 
  {
    Serial.println(F("Failed to read temperature from DHT22"));
  } 
  return (float) TempAr;
}

float humReading() // Function humReading is declared
{
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float Umidade = dht.readHumidity();
  if (isnan(Umidade)) 
  {
    Serial.println(F("Failed to read humidity from DHT22"));
  } 
  return (float) Umidade;
}

float heatindex()
{
    ////indice  calculado a partir da umidade e temp do ar.  
  float heatindex = dht.computeHeatIndex(TempAr,Umidade);
  if (isnan(heatindex)) 
  {
    Serial.println(F("Failed to read Heat Index from DHT22"));
  } 
  return (float) heatindex;
}

/*==============================================================
                            MQ-7
  ==============================================================*/

//int CO_value = analogRead(CO_MQ7); ////apenas leitura analogica do sensor de CO

float carbmonox()
{
    ////indice  calculado a partir da umidade e temp do ar.  
  float CO_value = analogRead(CO_MQ7);
  if (isnan(CO_value)) 
  {
    Serial.println(F("Failed to read Heat Index from MQ-7"));
  } 
  return (float) CO_value;
}


/*==============================================================
                  Sensor de Particulado
  ==============================================================
  
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

/* Eq. linear obtida em (http://www.howmuchsnow.com/arduino/airquality/)
(Chris Nafis 2012)

  dustDensity2 = (0.17 * temp2 - 0.1)* (1000/60);
  dustDensity1 = (0.17 * temp1 - 0.1)* (1000/60);

//========================================================================
*/


