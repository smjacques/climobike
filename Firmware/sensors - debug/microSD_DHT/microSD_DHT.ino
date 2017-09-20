#include <SD.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
 
const int chipSelect = 10; //MicroSD pin
//Constants
// DHT 22  (AM2302) - what pin we're connected to
#define DHTTYPE DHT22
#define dhtPin 5
DHT dht(dhtPin, DHTTYPE);
//Variables
float Umidade = 0;           //umidade relativa (%)
float TempAr = 0;            //temperatura do ar (degrees F)
float IndexCalor = 0; //indice de calor (degrees F)
 
void setup()
{
 
Serial.begin(9600);
while (!Serial) {
; // wait for serial port to connect.
}
Serial.print("Initializing SD card…");
pinMode(10, OUTPUT);

//init SD card
if (!SD.begin(chipSelect))
{
Serial.println("Card failed, or not present");
return;
}
Serial.println("card initialized.");
dht.begin();
}
 
void loop()
{
  
Umidade = dht.readHumidity();
  delay(20);  
TempAr = dht.readTemperature();// if in farenheit use (true)
  delay(20);  
IndexCalor = dht.computeHeatIndex(TempAr,Umidade);

// open the file.
File dataFile = SD.open("temp.txt", FILE_WRITE);
 
// if the file is available, write to it:
if (dataFile)
{
dataFile.println("Umidade do Ar = ");
dataFile.println(Umidade);
dataFile.println("%  ");
dataFile.println("Temperatura = ");
dataFile.println(TempAr);
dataFile.println("C  ");
dataFile.close();
}
// if the file isn’t open
else
{
Serial.println("error opening temp.txt");
}
}
