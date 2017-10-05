#include <Adafruit_Sensor.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <Wire.h>
#include <SPI.h>
#include <DHT.h>
#include <SD.h>



int dump_retries = 3;
int year;
int rand_num = 0; // temp rand_numom num
byte month, day, hour, minute, second, hundredths;
String gpsData, dateStamp, timeStamp, csvString = "";
boolean locked = false;
boolean card_ok = false;

float latitude, longitude, altitude, course, kmph;
char new_lat[6], new_lon[6];
unsigned long fix_age;



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
File logfile;
char filename[] = "LOGGER00.CSV";

//Umidade e Temperatura - DHT22

#define DHTTYPE DHT22
#define dhtPin 5
DHT dht(dhtPin, DHTTYPE);

float Umidade = 0;           //umidade relativa (%)
float TempAr = 0;            //temperatura do ar
float heatindex = 0;       //indice de calor

//Sensor de poeira(Shinyei PPD42NS/DSM501A)
//  pin 1 -> Arduino GND
//  pin 3 -> Arduino 5V
//  pin 2 -> Pin ~7 (P1 signal out (particulado grande, 3-10 µm)
//  pin 4 -> Pin ~8 (P2 signal out (particulado pequeno, 1-2 µm)
//  pin 5 -> unused!

unsigned long starttime;

unsigned long triggerOnP1;
unsigned long triggerOffP1;
unsigned long pulseLengthP1;
unsigned long durationP1;
boolean valP1 = HIGH;
boolean triggerP1 = false;

unsigned long triggerOnP2;
unsigned long triggerOffP2;
unsigned long pulseLengthP2;
unsigned long durationP2;
boolean valP2 = HIGH;
boolean triggerP2 = false;

float ratioP1 = 0;
float ratioP2 = 0;
unsigned long sampletime_ms = 30000;
float countP1;
float countP2;

// Carbon Monoxide (MQ-7)
//digital output (not used)
int CO_MQ7 = A0;




void setup()
{
  gpsSerial.begin(9600);
  pinMode(valP1,INPUT);
  pinMode(valP1,INPUT);
  pinMode(CO_MQ7, INPUT);
  pinMode(SDcard, OUTPUT);
  pinMode(11, OUTPUT);     // Pin 11 must be set as an output for the SD communication to work.
  pinMode(13, OUTPUT);    // Pin 13 must be set as an output for the SD communication to work.
  pinMode(12, INPUT);    // Pin 12 must be set as an input for the SD communication to work.
  
  Serial.begin(9600);
  
  // check that the microSD card exists and can be used
if (!SD.begin(SDcard)) {
    Serial.println("Card failed, or not present");
  }
  Serial.println("SD card is ready");
  
    // criar novo arquivo
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // apenas abre um novo arquivo se inexistente
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // abandonar o loop!
    }
  }
  
  if (! logfile) {
    Serial.print("impossvel criar arquivo");
  }
}
 
void getgps(TinyGPS &gps)
{
  float latitude, longitude;
  int year;
  byte month, day, hour, minute, second, hundredths;
 
  //decode and display position data
  gps.f_get_position(&latitude, &longitude);
  gps.crack_datetime(&year,&month,&day,&hour,&minute,&second,&hundredths);
  float falt = gps.f_altitude();
  float fcourse = gps.f_course();
  float fkmph = gps.f_speed_kmph();
  
  
  if (logfile) 
  {
    // Print data and time
    Serial.print("Date: "); 
    Serial.print(day, DEC);
    Serial.print("/");
    Serial.print(month, DEC);
    Serial.print("/");
    Serial.print(year, DEC);
    Serial.print(" ");
    Serial.print("Altitude ");
    Serial.print(falt, 2);
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
    Serial.print("km/h");     
    Serial.print(" ");
    delay(5000); // record a measurement about every 10 seconds
  
    // SD data: if the file is ready, write to it
    logfile.print("Date: "); 
    logfile.print(" ");
    logfile.print(day, DEC);
    logfile.print("/");
    logfile.print(month, DEC);
    logfile.print("/");
    logfile.print(year, DEC);
    logfile.print("Altitude ");
    logfile.print(gps.f_altitude());
    logfile.print("m ");
    logfile.print("Lat: "); 
    logfile.print(latitude,5); 
    logfile.print(" ");
    logfile.print("Long: "); 
    logfile.print(longitude,5);
    logfile.print(" ");  
   
   // correct for your time zone as in Project #45
    hour=hour-3; //Brazilian time zone (-3:00)
   
    if (hour>23)
    {
      hour=hour-24;
    }
   if (hour<10)
    {
      logfile.print("0");
    }  
    logfile.print(hour, DEC);
    logfile.print(":");
    if (minute<10) 
    {
      logfile.print("0");
    }
    logfile.print(minute, DEC);
    logfile.print(":");
    if (second<10)
    {
      logfile.print("0");
    }
    logfile.print(second, DEC);
    logfile.print(" ");
    logfile.print(gps.f_speed_kmph());
    logfile.print("km/h ");
    logfile.close(); // this is mandatory
    delay(5000); // record a measurement about every 10 seconds
  }
}
 
void loop() 
{
  queryGps();
}


//======================= Query GPS module and return GPS data =========================
void queryGps()
{
  // Get GPS data //
  while(gpsSerial.available())  // While there is data on the RX pin...
  {
    byte c = gpsSerial.read();   // load the data into a variable...
    if(gps.encode(c))          // if there is a new valid GPS data...
    {
      getgps(gps);  // then grab the data.
      //delay(speed);
      logData();   // log data
      delay(3000); // time between queries
    }
  }
  return;
}

//============================================ Log Data ===================================
void logData()
{
  // Make date and timestamp parts //
  //dateStamp =  String(year) + "-" + String(month) + "-" + String(day);

  // Pretty print minutes and seconds
  /*String min, sec = "";
  if(minute < 10) {
    min = "0"+String(minute);
  } 
  else {
    min = String(minute);
  }
  if(second < 10) {
    sec = "0"+String(second);
  } 
  else {
    sec = String(second);
  }*/


//Environmental Variables Functions
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  
  float TempAr = dht.readTemperature();
  float Umidade = dht.readHumidity();

  //indice de calor calculado a partir da umidade e temp do ar.  
  float heatindex = dht.computeHeatIndex(TempAr,Umidade);

// CO Sensor (MQ-7)
  //apenas leitura analogica do sensor de CO
  float CO_value = analogRead(CO_MQ7);

//Dust Sensor

  valP1 = digitalRead(7);
  valP2 = digitalRead(8);
  
  if(valP1 == LOW && triggerP1 == false){
    triggerP1 = true;
    triggerOnP1 = micros();
  }
  
  if (valP1 == HIGH && triggerP1 == true){
      triggerOffP1 = micros();
      pulseLengthP1 = triggerOffP1 - triggerOnP1;
      durationP1 = durationP1 + pulseLengthP1;
      triggerP1 = false;
  }
  
    if(valP2 == LOW && triggerP2 == false){
    triggerP2 = true;
    triggerOnP2 = micros();
  }
  
    if (valP2 == HIGH && triggerP2 == true){
      triggerOffP2 = micros();
      pulseLengthP2 = triggerOffP2 - triggerOnP2;
      durationP2 = durationP2 + pulseLengthP2;
      triggerP2 = false;
  }
  
    
    if ((millis() - starttime) > sampletime_ms) {
      
      ratioP1 = durationP1/(sampletime_ms*10.0);  // Integer percentage 0=>100
      ratioP2 = durationP2/(sampletime_ms*10.0);
      countP1 = 1.1*pow(ratioP1,3)-3.8*pow(ratioP1,2)+520*ratioP1+0.62;
      countP2 = 1.1*pow(ratioP2,3)-3.8*pow(ratioP2,2)+520*ratioP2+0.62;
      float PM10count = countP2;
      float PM25count = countP1 - countP2;
      
      // first, PM10 count to mass concentration conversion
      double r10 = 2.6*pow(10,-6);
      double pi = 3.14159;
      double vol10 = (4.0/3.0)*pi*pow(r10,3);
      double density = 1.65*pow(10,12);
      double mass10 = density*vol10;
      double K = 3531.5;
      float concLarge = (PM10count)*K*mass10;
      
      // next, PM2.5 count to mass concentration conversion
      double r25 = 0.44*pow(10,-6);
      double vol25 = (4.0/3.0)*pi*pow(r25,3);
      double mass25 = density*vol25;
      float concSmall = (PM25count)*K*mass25;


    Serial.print("Air Temperature ");
    Serial.print(TempAr);
    Serial.print(" ");
    Serial.print("Air Humidity ");
    Serial.print(Umidade);
    Serial.print(" ");
    Serial.print("Heat Index ");
    Serial.print(heatindex);
    Serial.print(" ");
    Serial.print("Concentration Large (µg/m3) ");
    Serial.print(concLarge);
    Serial.print(" ");
    Serial.print("Dust Count1.0 (pt/cf) ");
    Serial.print(PM10count);
    Serial.print(" ");
    Serial.print("Concentration Small (µg/m3) ");
    Serial.print(concSmall); // unit: µg/m3
    Serial.print(" ");
    Serial.print("Dust Count2.5 (pt/cf) ");
    Serial.print(PM25count);
    Serial.print(" ");
    Serial.print("CO () ");
    Serial.println(CO_value);

// SD data: if the file is ready, write to it
    logfile.print("Air Temperature ");
    logfile.print(TempAr);
    logfile.print(" ");
    logfile.print("Air Humidity ");
    logfile.print(Umidade);
    logfile.print(" ");
    logfile.print("Heat Index ");
    logfile.print(heatindex);
    logfile.print(" ");
    logfile.print("Concentration Large (µg/m3) ");
    logfile.print(concLarge);
    logfile.print(",");
    logfile.print("Dust Count 1.0µm (pt/cf) ");
    logfile.print(PM10count);
    logfile.print(",");
    logfile.print("Concentration Small (µg/m3) ");
    logfile.print(concSmall); // unit: µg/m3
    logfile.print(",");
    logfile.print("Dust Count2.5 µm (pt/cf) ");
    logfile.print(PM25count);
    logfile.print(" ");
    logfile.print("CO () ");
    logfile.println(CO_value);
    logfile.close(); // this is mandatory   
    //delay(5000); // record a measurement about every 10 seconds
    }
//gravar dados no SD
  logfile.flush();
  delay(5000);
}


