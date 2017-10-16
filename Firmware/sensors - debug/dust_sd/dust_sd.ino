//DustDuino Serial
//By Matthew Schroyer, MentalMunition.com

// DESCRIPTION
// Outputs particle concentration per cubic foot and
// mass concentration (microgram per cubic meter) to
// serial, from a Shinyei particuate matter sensor.

// CONNECTING THE DUSTDUINO
// P1 channel on sensor is connected to digital 7
// P2 channel on sensor is connected to gitial 8

// THEORY OF OPERATION
// Sketch measures the width of pulses through
// boolean triggers, on each channel.
// Pulse width is converted into a percent integer
// of time on, and equation uses this to determine
// particle concentration.
// Shape, size, and density are assumed for PM10
// and PM2.5 particles. Mass concentration is
// estimated based on these assumptions, along with
// the particle concentration.

#include <SD.h>

const int chipSelect = 10;

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

void setup(){
  Serial.begin(9600);
  
  pinMode(7, INPUT);
  pinMode(8, INPUT);
  
//   Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
/*  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  
  String startString = "PM10 count,PM2.5 count,PM10 conc,PM2.5 conc";
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
    if (dataFile) {
    // Print first row with column headings in comma separated value format.  
    dataFile.println(startString);
    dataFile.close();
    }*/
}

void loop(){
  
  digitalWrite(7, HIGH);
  
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
      
      if(PM25count < 0){
      PM25count = 0;
      }
      
      // first, PM10 count to mass concentration conversion
      double r10 = 2.6*pow(10,-6);
      double pi = 3.14159;
      double vol10 = (4/3)*pi*pow(r10,3);
      double density = 1.65*pow(10,12);
      double mass10 = density*vol10;
      double K = 3531.5;
      float concLarge = (PM10count)*K*mass10;
      
      // next, PM2.5 count to mass concentration conversion
      double r25 = 0.44*pow(10,-6);
      double vol25 = (4/3)*pi*pow(r25,3);
      double mass25 = density*vol25;
      float concSmall = (PM25count)*K*mass25;
      
       // Print first row with column headings in comma separated value format.  
      Serial.print(PM10count);
      Serial.print(", ");
      Serial.print(PM25count);
      Serial.print(", ");
      Serial.print(concLarge);
      Serial.print(", ");
      Serial.println(concSmall);
      delay(2000);
    }
    
}
