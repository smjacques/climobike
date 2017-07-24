/*
 Shinyei PPD42NS Particle Sensor
 publish via serial and raspberry pi/python
 specification: http://www.sca-shinyei.com/pdf/PPD42NS.pdf
 pin 1 (grey)  -> Arduino GND
 pin 3 (blue) -> Arduino 5V
 pin 2 (green) -> Pin ~3
 pin 4 (white) -> Pin ~6
 pin 5 (red)   -> unused!

Based mainly on: https://github.com/opendata-stuttgart/sensors-software/blob/master/arduino/ppd42ns-serial/ppd42ns-serial.ino
With lines ?? ?? from: 
 
*/

boolean valP1 = HIGH;
boolean valP2 = HIGH;

unsigned long starttime;
unsigned long durationP1;
unsigned long durationP2;

boolean trigP1 = false;
boolean trigP2 = false;
unsigned long trigOnP1;
unsigned long trigOnP2;

unsigned long sampletime_ms = 1600; //intervalo entre medida
unsigned long lowpulseoccupancyP1 = 0;
unsigned long lowpulseoccupancyP2 = 0;

float dustDensity = 0;
float dustDensity2 = 0;

void setup() {
  Serial.begin(9600);
  pinMode(3,INPUT);
  pinMode(6,INPUT);
  starttime = millis();
}

void loop() {
  float ratio = 0;
  float concentration = 0;
  float cumsum = 0;
  float ratio2 = 0;
  float concentration2 = 0;
  float cumsum2 = 0;
  float temp1 = 0;
  float temp2 = 0;
  valP1 = digitalRead(8);
  valP2 = digitalRead(9);
  
  if(valP1 == LOW && trigP1 == false){
    trigP1 = true;
    trigOnP1 = micros();
  }
  
  if (valP1 == HIGH && trigP1 == true){
    durationP1 = micros() - trigOnP1;
    lowpulseoccupancyP1 = lowpulseoccupancyP1 + durationP1;
    trigP1 = false;
  }
  
  if(valP2 == LOW && trigP2 == false){
    trigP2 = true;
    trigOnP2 = micros();
  }
  
  if (valP2 == HIGH && trigP2 == true){
    durationP2 = micros() - trigOnP2;
    lowpulseoccupancyP2 = lowpulseoccupancyP2 + durationP2;
    trigP2 = false;
  }

  if ((millis()-starttime) > sampletime_ms)
  {
    ratio = lowpulseoccupancyP1/(sampletime_ms*10.0);     // int percentage 0 to 100
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // spec sheet curve
    temp1 = valP1 * (5.0 / 1024.0);
    cumsum = cumsum + temp1;// cumulative sum over 60 seconds
/* linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
Chris Nafis (c) 2012*/    
    dustDensity = (0.17 * cumsum - 0.1)* (1000/60);
    
    Serial.print("cumulative: ");
    Serial.println(cumsum);
        Serial.print("\n");
    Serial.print("Densidade (ug/m3)");
    Serial.print(dustDensity);
        Serial.print("\n");
    Serial.print("lowpulseoccupancyP1:");
    Serial.print(lowpulseoccupancyP1);
    Serial.print("\n");
    Serial.print("ratio:");
    Serial.print(ratio);
    Serial.print("\n");
    Serial.print("concentration:");
    Serial.print(concentration);
    Serial.print("\n");
    
    ratio2 = lowpulseoccupancyP2/(sampletime_ms*10.0);
    concentration2 = 1.1*pow(ratio2,3)-3.8*pow(ratio2,2)+520*ratio2+0.62;
    temp2 = valP2 * (5.0 / 1024.0);
    cumsum2 = cumsum + temp2;// cumulative sum over 60 seconds

/* linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
Chris Nafis (c) 2012*/

dustDensity2 = (0.17 * cumsum2 - 0.1)* (1000/60);
  
    
    Serial.print("\n");
    Serial.print("cumulative2: ");
    Serial.println(cumsum2);   
    Serial.print("\n");
    Serial.print(" Densidade de Poeira (ug/m3)");
    Serial.println(dustDensity2); // unit: ug/m3
    Serial.print("\n");
    Serial.print("lowpulseoccupancyP2:");
    Serial.print(lowpulseoccupancyP2);
    Serial.print("\n");
    Serial.print("ratio:");
    Serial.print(ratio2);
    Serial.print("\n");
    Serial.println("concentration:");
    Serial.println(concentration2);
    Serial.print("\n");
    
    
    lowpulseoccupancyP1 = 0;
    lowpulseoccupancyP2 = 0;
    starttime = millis();
  }
}
