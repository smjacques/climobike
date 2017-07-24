#include <dht.h>

/*
*/


#include "DHT.h"
#include "Adafruit_Sensor.h"


#define DHTTYPE DHT22
#define ECHO_TO_SERIAL 1         //Envia registro de dados para serial se 1, nada se 0
#define LOG_INTERVAL 10000     //Intervado de medidas (6 minutes = 360000)


#define dhtPin 2           // output sensor DHT

DHT dht(dhtPin, DHTTYPE);
float Umidade = 0;           //umidade relativa (%)
float TempAr = 0;            //temperatura do ar (degrees F)
float IndexCalor = 0;          //indice de calor (degrees F)

void setup() {
  
  //Iniciar conexcao serial
  Serial.begin(9600); //apenas para teste
  dht.begin(); //Estabelece conexao com o sensor DHT
}


void loop() {
    Umidade = dht.readHumidity();
  delay(20);
  
  TempAr = dht.readTemperature(true);
  delay(20);
  
  IndexCalor = dht.computeHeatIndex(TempAr,Umidade); //indice  calculado a partir da umidade e temp do ar.
  
  #if ECHO_TO_SERIAL
  Serial.print(TempAr);
  Serial.print(",");
  Serial.print(Umidade);
  Serial.print(",");
  Serial.print(IndexCalor);
  Serial.print(",");
#endif
}
