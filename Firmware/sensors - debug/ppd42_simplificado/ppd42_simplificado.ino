/*
 Shinyei PPD42NS Particle Sensor
 publish via serial and raspberry pi/python
 specification: http://www.sca-shinyei.com/pdf/PPD42NS.pdf
 pin 1 (grey)  -> Arduino GND
 pin 3 (blue) -> Arduino 5V
 pin 2 (green) -> Pin ~3 (P1 signal out (large particulates, 3-10 µm)
 pin 4 (white) -> Pin ~6 (P2 signal out (small particulates, 1-2 µm)
 pin 5 (red)   -> unused!

Based mainly on: https://github.com/opendata-stuttgart/sensors-software/blob/master/arduino/ppd42ns-serial/ppd42ns-serial.ino
With personal modifications as well as from http://arduinodev.woofex.net/2012/12/01/standalone-sharp-dust-sensor/

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

void setup() {
  Serial.begin(9600);
  starttime = millis();
  pinMode(3,INPUT);
  pinMode(6,INPUT);
  starttime = millis();
}

void loop() {
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

#if ECHO_TO_SERIAL
    Serial.println("Densidade (ug/m3),ratio,concentration,Densidade2 (ug/m3),ratio2,concentration2");
    Serial.print(dustDensity1);
    Serial.print(",");
    Serial.print(ratio);
    Serial.print(",");
    Serial.print(concentration);
    Serial.print(",");
    Serial.println(dustDensity2); // unit: ug/m3
    Serial.print(",");
    Serial.print(ratio2);
    Serial.print(",");
    Serial.println(concentration2);
#endif

    lowpulseoccupancyP1 = 0;
    lowpulseoccupancyP2 = 0;
    starttime = millis();
    delay(2000);
}
